
#pragma once

#include <rynx/tech/parallel_accumulator.hpp>
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
		class task_token;

		class task {
		private:
			friend class rynx::scheduler::context;
			friend class rynx::scheduler::task_token;

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

				template<typename T> inline auto& getTaskArgParam(context* [[maybe_unused]] ctx, rynx::scheduler::task* [[maybe_unused]] currentTask) {
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

		public:
			// TODO: use some memory pool thing for task_resources.
			task() : m_name("EmptyTask"), m_context(nullptr) {}
			task(context& context, std::string name)
				: m_name(std::move(name))
				, m_context(&context)
				, m_resources(std::make_shared<task_resources>(&context))
			{}
			
			template<typename F> task(context& context, std::string taskName, F&& op)
				: m_name(std::move(taskName))
				, m_context(&context)
				, m_resources(std::make_shared<task_resources>(&context)) {
				set(std::forward<F>(op));
			}

			task& operator =(task&& other) = default;
			
			// TODO: PRIVATE!!
		public:
			// NOTE:
			// copying tasks does not increment barrier counters because we don't know if user actually wants that or not.
			// that also makes the copying api very surprising, and that is why it is hidden from end users as private.
			task(const task& other) = default; // copy is required in parallel_for case.
		public:
			
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

			task& required_for(task& other) {
				other.depends_on(*this);
				return *this;
			}

			task& depends_on(barrier bar) {
				barriers().depends_on(bar);
				return *this;
			}

			task& required_for(barrier bar) {
				barriers().required_for(bar);
				return *this;
			}

			task& depends_on(task_token& other);
			task& required_for(task_token& other);

			// TODO: Rename this function.
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

			task& copy_barriers_of(task& other) {
				other.completion_blocked_by(*this);
				return *this;
			}

			task& copy_resources(task& other) {
				if (m_resources_shared) {
					other.m_resources->insert(*m_resources_shared);
				}
				other.m_resources->insert(*m_resources);
				return *this;
			}

			// task is not complete until barrier completes.
			// '&' denotes "both must be complete for dependent tasks to start".
			task& operator & (barrier bar);
			task& operator & (task_token& other);
			
			// '|' denotes pipe, a | b   == b comes after a.
			task& operator | (task_token& other);

			// TODO: Rename all task creation functions.
			template<typename F> task_token make_task(F&& op) {
				return task_token(m_context->make_task("task", std::forward<F>(op)));
			}

			template<typename F> task_token extend_task(std::string name, F&& op) {
				auto followUpTask = m_context->make_task(std::move(name), std::forward<F>(op));
				completion_blocked_by(followUpTask);
				return task_token(std::move(followUpTask));
			}

			template<typename F> task_token extend_task(F&& op) { return extend_task(m_name + "_e", std::forward<F>(op)); }
			
			template<typename F> task_token make_extension_task_execute_sequential(std::string name, F&& op) {
				auto followUpTask = m_context->make_task(std::move(name), std::forward<F>(op));
				completion_blocked_by(followUpTask);
				copy_resources(followUpTask);
				return std::move(followUpTask);
			}
			
			template<typename F> task_token make_extension_task_execute_parallel(std::string name, F&& op) {
				auto followUpTask = m_context->make_task(std::move(name), std::forward<F>(op));
				completion_blocked_by(followUpTask);
				share_resources(followUpTask);
				return std::move(followUpTask);
			}

			template<typename F> task_token extend_task_execute_parallel(std::string name, F&& op) { return make_extension_task_execute_parallel(std::move(name), std::forward<F>(op)); }
			template<typename F> task_token extend_task_execute_parallel(F&& op) { return extend_task_execute_parallel(m_name + "_es", std::forward<F>(op)); }
			template<typename F> task_token extend_task_execute_sequential(std::string name, F&& op) { return make_extension_task_execute_sequential(std::move(name), std::forward<F>(op)); }
			template<typename F> task_token extend_task_execute_sequential(F&& op) { return extend_task_execute_sequential(m_name + "_es", std::forward<F>(op)); }

			template<typename F>
			task_token then(std::string name, F&& f) {
				auto followUpTask = m_context->make_task(std::move(name), std::forward<F>(f));
				followUpTask.depends_on(*this);
				return m_context->add_task(std::move(followUpTask));
			}

			template<typename F> task_token then(F&& f) { return then(m_name + "->", std::forward<F>(f)); }

			task(task&& other) = default;

			template<typename F>
			void set(F&& f) {
				resource_deducer<decltype(&std::remove_reference_t<F>::operator())>()(*this);
				context* ctx = m_context;

				// this is marked as mutable in order to support f being mutable. since it is captured here in a lambda,
				// if this were not mutable, f would not be allowed to change itself either. 
				m_op = [ctx, f = std::move(f)](rynx::scheduler::task* myTask) mutable {
					auto resources = resource_deducer<decltype(&std::remove_reference_t<F>::operator())>().fetchParams(ctx, myTask);
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

		private:
			struct parallel_for_each_data {
				parallel_for_each_data(int64_t begin, int64_t end) : index(begin), end(end), task_is_cleaned_up(0) {}
				std::atomic<int64_t> index;
				std::atomic<int> task_is_cleaned_up;
				const int64_t end;
			};

		public:
			// NOTE: parallel_for_each is implemented as atomic index which means that the first time that a worker exits
			//       this task, is when all of the workload is being worked on. So there is no way that one task begins work on
			//       parallel for, does some work, exits the task, and looks for other work. This would leave the task in a state
			//       where its resources would no longer be reserved, but work would still need to be done.
			//       That would make life much more complicated. That is the reason why pre-divided work sizes
			//       approach to parallel fors was abandoned.
			struct parallel_operations {
				parallel_operations(task& parent) : m_parent(parent) {}

				struct parallel_for_operation {
					parallel_for_operation(rynx::scheduler::task& parent) : m_parent(parent) {}

					rynx::scheduler::task& m_parent;
					int64_t begin = 0;
					int64_t end = 0;
					int64_t work_size = 1;
					bool self_participate = true;

					parallel_for_operation& range_begin(int64_t begin_) {
						begin = begin_;
						return *this;
					}

					parallel_for_operation& range_end(int64_t end_) {
						end = end_;
						return *this;
					}


					parallel_for_operation& work_per_iteration(int64_t work_size_) {
						work_size = work_size_;
						return *this;
					}

					parallel_for_operation& deferred_work() {
						self_participate = false;
						return *this;
					}

					template<typename F>
					barrier execute(F&& op) {
						barrier bar;

						std::shared_ptr<parallel_for_each_data> for_each_data = std::make_shared<parallel_for_each_data>(begin, end);

						{
							task_token work = m_parent.make_extension_task_execute_parallel("parfor", [task_context = m_parent.m_context, work_size = work_size, end = end, for_each_data, op]() mutable {
								for (;;) {
									int64_t my_index = for_each_data->index.fetch_add(work_size);
									if (my_index >= end) {
										if (for_each_data->task_is_cleaned_up.exchange(1) == 0) {
											task_context->erase_completed_parallel_for_tasks();
										}
										return;
									}
									int64_t limit = my_index + work_size >= end ? end : my_index + work_size;
									for (int64_t i = my_index; i < limit; ++i) {
										op(i);
									}
								}
							});
							work->m_for_each = for_each_data;
							work->required_for(bar);
						}

						m_parent.m_context->m_scheduler->wake_up_sleeping_workers();

						if (self_participate) {
							for (;;) {
								int64_t my_index = for_each_data->index.fetch_add(work_size);
								if (my_index >= end) {
									if (for_each_data->task_is_cleaned_up.exchange(1) == 0) {
										m_parent.m_context->erase_completed_parallel_for_tasks();
									}
									return bar;
								}
								int64_t limit = my_index + work_size >= end ? end : my_index + work_size;
								for (int64_t i = my_index; i < limit; ++i) {
									op(i);
								}
							}
						}
						return bar;
					}
				};

				parallel_for_operation for_each(int64_t begin = 0, int64_t end = 0, int64_t work_size = 256) {
					parallel_for_operation op(m_parent);
					op.begin = begin;
					op.end = end;
					op.work_size = work_size;
					return op;
				}

				task& m_parent;
			};

			// to allow passing task as a parameter when parallel operations context is expected.
			operator parallel_operations() const {
				return parallel_operations(*this);
			}

			parallel_operations parallel() {
				return parallel_operations(*this);
			}

		private:
			bool is_for_each() const {
				return static_cast<bool>(m_for_each);
			}

			bool is_for_each_done() const {
				return m_for_each->index.load() >= m_for_each->end;
			}
			
			std::string m_name;
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
			std::shared_ptr<parallel_for_each_data> m_for_each;

			context* m_context = nullptr;
		};

		class task_token {
		public:
			task_token(task&& task) : m_task(std::move(task)) {}
			~task_token();

			task* operator -> ();
			task& operator * () { return m_task; }
			
			// chaining tasks with nice api
			task_token& operator | (task_token& other) {
				other->depends_on(m_task);
				return other;
			}

		private:
			task m_task;
		};
	}
}
