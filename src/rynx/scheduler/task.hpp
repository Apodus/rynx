
#pragma once

#include <rynx/tech/ecs.hpp>
#include <rynx/scheduler/barrier.hpp>
#include <rynx/system/assert.hpp>

#include <string>
#include <functional>

namespace rynx {
	namespace scheduler {
		class task_scheduler;
		class context;
		class task;

		struct task_token {
			task_token(context* context, size_t taskId) : m_taskId(taskId), m_context(context) {}
			task* operator -> ();
			task& operator * () { return *operator->(); }

		private:
			size_t m_taskId;
			context* m_context;
		};

		class task {
		private:
			friend class rynx::scheduler::context;

			template<typename F> struct resource_deducer {};
			template<typename RetVal, typename Class, typename...Args> struct resource_deducer<RetVal(Class::*)(Args...) const> {
				template<typename T>
				struct unpack_resource {
					void operator()(task& host) {
						uint64_t typeId = host.m_context->m_typeIndex.id<std::remove_cvref_t<T>>();
						if constexpr (std::is_const_v<std::remove_reference_t<T>>) {
							host.resources().require_read(typeId);
						}
						else {
							host.resources().require_write(typeId);
						}
					}
				};

				template<typename... Args>
				struct unpack_resource<rynx::ecs::view<Args...>> {
					void operator()(task& host) {
						(unpack_resource<Args>()(host), ...);
						unpack_resource<const rynx::ecs&>()(host); // also mark us as reading the ecs. this prevents any view tasks to run simultaneously with a mutable ecs& task.
					}
				};

				template<typename... Args>
				struct unpack_resource<rynx::ecs::edit_view<Args...>> {
					void operator()(task& host) {
						(unpack_resource<Args>()(host), ...);
						unpack_resource<rynx::ecs&>()(host); // also mark us as writing to the entire ecs. this is because adding/removing components can move any types of data around the ecs categories.
					}
				};

				// don't mark tasks as required resources. they are unique for each task, not shared.
				template<> struct unpack_resource<rynx::scheduler::task&> { void operator()(task&) {} };
				template<> struct unpack_resource<const rynx::scheduler::task&> { void operator()(task&) {} };

				void operator()(task& host) {
					(unpack_resource<Args>()(host), ...); // apply the resource constraints to the task.
				}

				template<typename T> inline auto& getTaskArgParam(context* ctx, rynx::scheduler::task* currentTask) {
					(void)currentTask; // trust me mr compiler. they are both useful parameters.
					(void)ctx;
					if constexpr (!std::is_same_v<std::remove_cvref_t<T>, rynx::scheduler::task>) {
						return ctx->getTaskArgParam<T>();
					}
					else {
						return *currentTask;
					}
				}

				auto fetchParams(context* ctx, rynx::scheduler::task* currentTask) {
					return std::forward_as_tuple(getTaskArgParam<Args>(ctx, currentTask)...);
				}
			};
			template<typename RetVal, typename Class, typename...Args> struct resource_deducer<RetVal(Class::*)(Args...)> : public resource_deducer<RetVal(Class::*)(Args...) const> {};

			void reserve_resources() const;

			task& id(uint64_t taskId) { m_id = taskId; return *this; }

		public:
			// TODO: use some memory pool thing for task_resources.
			task() : m_name("EmptyTask"), m_context(nullptr) {}
			task(context& context, uint64_t taskId, std::string name)
				: m_name(std::move(name))
				, m_id(taskId)
				, m_context(&context)
				, m_resources(std::make_shared<task_resources>(&context))
			{}
			
			template<typename F> task(context& context, uint64_t taskId, std::string taskName, F&& op)
				: m_name(std::move(taskName))
				, m_id(taskId)
				, m_context(&context)
				, m_resources(std::make_shared<task_resources>(&context)) {
				set(std::forward<F>(op));
			}

			uint64_t id() const { return m_id; }

			operation_barriers& barriers() { return m_barriers; }
			const operation_barriers& barriers() const { return m_barriers; }

			operation_resources& resources() { return *m_resources; }
			const operation_resources& resources() const { return *m_resources; }

			task& depends_on(task& other) {
				barrier bar("anon");
				barriers().depends_on(bar);
				other.barriers().required_for(bar);
				return *this;
			}

			task& depends_on(barrier bar) {
				barriers().depends_on(bar);
				return *this;
			}

			task& required_for(task& other) {
				barrier bar("anon");
				other.barriers().depends_on(bar);
				barriers().required_for(bar);
				return *this;
			}

			task& required_for(barrier bar) {
				barriers().required_for(bar);
				return *this;
			}

			task& completion_blocked_by(task& other) {
				m_barriers.completion_blocked_by(other.barriers());
				return *this;
			}

			task& share_resources(task& other) {
				rynx_assert(!other.m_resources_shared, "can't share more than one task's resources!");
				if (m_resources_shared) {
					other.m_resources_shared = m_resources_shared;
				}
				else {
					other.m_resources_shared = m_resources;
				}
				return *this;
			}

			task& copy_resources(task& other) {
				if (m_resources_shared) {
					other.m_resources->insert(*m_resources_shared);
				}
				other.m_resources->insert(*m_resources);
				return *this;
			}

			template<typename F> task& extend_task(std::string name, F&& op) {
				auto followUpTask = m_context->make_task(std::move(name), std::forward<F>(op));
				completion_blocked_by(followUpTask);
				m_context->add_task(std::move(followUpTask));
				return *this;
			}
			template<typename F> task& extend_task(F&& op) { return extend_task(m_name + "_e", std::forward<F>(op)); }
			
			// NOTE: This is the only task creation function where you are allowed to capture
			//       resources in your lambda. You are allowed to capture the resources used
			//       by the task extended. These functions will run simultaneously regardless
			//       oh having same write targets as shared resources. You are responsible for
			//       ensuring that there are no race conditions when writing data.

			template<typename F> task make_extension_task_execute_sequential(std::string name, F&& op) {
				auto followUpTask = m_context->make_task(std::move(name), std::forward<F>(op));
				completion_blocked_by(followUpTask);
				copy_resources(followUpTask);
				return followUpTask;
			}
			template<typename F> task make_extension_task_execute_parallel(std::string name, F&& op) {
				auto followUpTask = m_context->make_task(std::move(name), std::forward<F>(op));
				completion_blocked_by(followUpTask);
				share_resources(followUpTask);
				return followUpTask;
			}

			template<typename F> task& extend_task_execute_parallel(std::string name, F&& op) {
				m_context->add_task(make_extension_task_execute_parallel(std::move(name), std::forward<F>(op)));
				return *this;
			}
			template<typename F> task& extend_task_execute_parallel(F&& op) { return extend_task_execute_parallel(m_name + "_es", std::forward<F>(op)); }

			template<typename F> task& extend_task_execute_sequential(std::string name, F&& op) {
				m_context->add_task(make_extension_task_execute_sequential(std::move(name), std::forward<F>(op)));
				return *this;
			}
			template<typename F> task& extend_task_execute_sequential(F&& op) { return extend_task_execute_sequential(m_name + "_es", std::forward<F>(op)); }


			template<typename F>
			task_token then(std::string name, F&& f) {
				auto followUpTask = m_context->make_task(std::move(name), std::forward<F>(f));
				followUpTask.depends_on(*this);
				return m_context->add_task(std::move(followUpTask));
			}

			task(task&& other) = default;
			task& operator =(task&& other) = default;

			template<typename F>
			void set(F&& f) {
				resource_deducer<decltype(&F::operator())>()(*this);
				context* ctx = m_context;

				m_op = [ctx, f = std::move(f)](rynx::scheduler::task* myTask) {
					auto resources = resource_deducer<decltype(&F::operator())>().fetchParams(ctx, myTask);
					std::apply(f, resources);
				};

				// apply current barrier states.
				for (auto&& bar : m_context->m_activeTaskBarriers_Dependencies)
					this->depends_on(bar);

				for (auto&& bar : m_context->m_activeTaskBarriers)
					this->required_for(bar);
			}

			void run();

			operator bool() const { return static_cast<bool>(m_op); }
			void clear() { m_op = nullptr; }

			const std::string& name() const { return m_name; }

			struct parallel_operations {
				parallel_operations(task& parent) : m_parent(parent) {}

				template<typename F> barrier for_each_prediv(int64_t begin, int64_t end, F&& op, int64_t divisions = 32) {
					int64_t size = end - begin;
					if (size == 0)
						return {};
					while (divisions > size)
						divisions >>= 1;

					barrier bar;

					int64_t block_size = size / divisions;
					int64_t block_start = begin;
					int64_t block_end = begin;
					for (int64_t i = 0; i < divisions - 1; ++i) {
						block_end += block_size;
						task work = m_parent.make_extension_task_execute_parallel("parfor", [block_start, block_end, op]() {
							for (int64_t begin = block_start; begin < block_end; ++begin) {
								op(begin);
							}
						});
						work.required_for(bar);
						m_parent.m_context->add_task(std::move(work));
						block_start += block_size;
					}
					m_parent.m_context->m_scheduler->wake_up_sleeping_workers();

					for (; block_start < end; ++block_start) {
						op(block_start);
					}

					return bar;
				}

				template<typename F> barrier for_each_atomic_index(int64_t begin, int64_t end, F&& op, int64_t num_worker_threads = 8, int64_t work_size = 32) {
					barrier bar;
					std::shared_ptr<std::atomic<int64_t>> index = std::make_shared<std::atomic<int64_t>>(begin);
					for (int64_t i = 0; i < num_worker_threads - 1; ++i) {
						task work = m_parent.make_extension_task_execute_parallel("parfor", [index, work_size, end, op]() {
							for (;;) {
								int64_t my_index = index->fetch_add(work_size);
								for (int64_t i = 0; i < work_size; ++i) {
									if (my_index + i >= end)
										return;
									op(my_index + i);
								}
							}
						});
						work.required_for(bar);
						m_parent.m_context->add_task(std::move(work));
					}
					m_parent.m_context->m_scheduler->wake_up_sleeping_workers();

					for (;;) {
						int64_t my_index = index->fetch_add(work_size);
						for (int64_t i = 0; i < work_size; ++i) {
							if (my_index + i >= end)
								return bar;
							op(my_index + i);
						}
					}
				}

				task& m_parent;
			};

			parallel_operations parallel() {
				return parallel_operations(*this);
			}

		private:
			std::string m_name;
			uint64_t m_id;
			std::function<void(rynx::scheduler::task*)> m_op;
			operation_barriers m_barriers;
			
			struct task_resources : public operation_resources {
				task_resources(context* ctx) : m_context(ctx) {}
				~task_resources();

				task_resources(const task_resources& other) = default;
				task_resources& operator = (const task_resources& other) = default;

				// moving not allowed
				task_resources(task_resources&& other) = delete;
				task_resources& operator = (task_resources&& other) = delete;

				task_resources& insert(const task_resources& other) {
					for (const auto resource_id : other.read_requirements()) {
						require_read(resource_id);
					}
					for (const auto resource_id : other.write_requirements()) {
						require_write(resource_id);
					}
					return *this;
				}

			private:
				context* m_context = nullptr;
			};

			std::shared_ptr<task_resources> m_resources;
			std::shared_ptr<task_resources> m_resources_shared;

			context* m_context = nullptr;
		};
	}
}
