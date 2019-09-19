
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
			friend class rynx::scheduler::task_scheduler;

			template<typename F> struct resource_deducer {};
			template<typename RetVal, typename Class, typename...Args> struct resource_deducer<RetVal(Class::*)(Args...) const> {
				template<typename T>
				struct unpack_resource {
					void operator()(task& host) {
						uint64_t typeId = host.m_context->m_typeIndex.id<std::remove_cvref_t<T>>();
						if constexpr (std::is_const_v<std::remove_reference_t<T>>) {
							host.resources().requireRead(typeId);
						}
						else {
							host.resources().requireWrite(typeId);
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

			task& dependsOn(task& other) {
				barrier bar("anon");
				barriers().require(bar);
				other.barriers().block(bar);
				return *this;
			}

			task& requiredFor(task& other) {
				barrier bar("anon");
				other.barriers().require(bar);
				barriers().block(bar);
				return *this;
			}

			task& before(barrier bar) {
				barriers().block(bar);
				return *this;
			}

			task& after(barrier bar) {
				barriers().require(bar);
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
			template<typename F> task& extend_task_shared_resources(std::string name, F&& op) {
				auto followUpTask = m_context->make_task(std::move(name), std::forward<F>(op));
				completion_blocked_by(followUpTask);
				share_resources(followUpTask);
				m_context->add_task(std::move(followUpTask));
				return *this;
			}
			template<typename F> task& extend_task_shared_resources(F&& op) { return extend_task_shared_resources(m_name + "_es", std::forward<F>(op)); }


			template<typename F>
			task_token then(std::string name, F&& f) {
				auto followUpTask = m_context->make_task(std::move(name), std::forward<F>(f));
				followUpTask.dependsOn(*this);
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
					this->after(bar);

				for (auto&& bar : m_context->m_activeTaskBarriers)
					this->before(bar);
			}

			void run();

			operator bool() const { return static_cast<bool>(m_op); }
			void clear() { m_op = nullptr; }

			const std::string& name() const { return m_name; }

		private:
			std::string m_name;
			uint64_t m_id;
			std::function<void(rynx::scheduler::task*)> m_op;
			operation_barriers m_barriers;
			
			struct task_resources : public operation_resources {
				task_resources(context* ctx) : m_context(ctx) {}
				~task_resources();

			private:
				context* m_context = nullptr;
			};

			std::shared_ptr<task_resources> m_resources;
			std::shared_ptr<task_resources> m_resources_shared;

			context* m_context = nullptr;
		};
	}
}