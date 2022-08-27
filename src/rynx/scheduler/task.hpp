
#pragma once

#include <rynx/tech/parallel/accumulator.hpp>
#include <rynx/ecs/ecs.hpp>
#include <rynx/std/string.hpp>
#include <rynx/scheduler/barrier.hpp>
#include <rynx/system/assert.hpp>

namespace rynx {
	namespace scheduler {
		class task_scheduler;
		class context;
		class task;
		class task_token;

		class SchedulerDLL task_token {
		private:
			// TODO get rid of this
			template<typename RynxTask, typename F> static task_token silly_delayed_evaluate(rynx::string&& name, RynxTask& task, F&& f) {
				auto followUpTask = task.m_context->add_task(std::move(name), std::forward<F>(f));
				followUpTask.depends_on(task);
				return followUpTask;
			}
		public:
			task_token(task&& task);
			task_token(task_token&&) = default;
			~task_token();

			task* operator -> ();
			task& operator * ();
			
			// chaining tasks with nice api
			task_token& operator | (task_token& other);

			task_token& depends_on(barrier bar);
			task_token& required_for(barrier bar);
			task_token& depends_on(task& other);
			task_token& required_for(task& other);
			task_token& depends_on(task_token& other) { return depends_on(*other); }
			task_token& required_for(task_token& other) { return required_for(*other); }

			template<typename F>
			task_token then(rynx::string name, F&& f) {
				return this->silly_delayed_evaluate(std::move(name), *m_pTask.get(), std::forward<F>(f));
				/*
				auto followUpTask = m_pTask->m_context->add_task(std::move(name), std::forward<F>(f));
				followUpTask.depends_on(*this);
				return followUpTask;
				*/
			}

			template<typename F> task_token then(F&& f) { return then("->", std::forward<F>(f)); }

		private:
			rynx::unique_ptr<task> m_pTask;
		};

		class SchedulerDLL task {
		private:
			friend class rynx::scheduler::context;
			friend class rynx::scheduler::task_token;

			template<typename F> struct resource_deducer {};
			template<typename RetVal, typename Class, typename...Args> struct resource_deducer<RetVal(Class::*)(Args...) const> {
				template<typename T>
				struct unpack_resource {
					void operator()(task& host) {
						uint64_t typeId = rynx::type_index::id<std::remove_cvref_t<T>>();
						if constexpr (std::is_const_v<std::remove_reference_t<T>>) {
							host.resources().require_read(typeId);
						}
						else {
							host.resources().require_write(typeId);
						}
					}
				};

				template<typename... Ts>
				struct unpack_resource<rynx::ecs::view<Ts...>> {
					void operator()(task& host) {
						(unpack_resource<Ts>()(host), ...);
						unpack_resource<const rynx::ecs&>()(host); // also mark us as reading the ecs. this prevents any view tasks to run simultaneously with a mutable ecs& task.
					}
				};

				template<typename... Ts>
				struct unpack_resource<rynx::ecs::edit_view<Ts...>> {
					void operator()(task& host) {
						(unpack_resource<Ts>()(host), ...);
						unpack_resource<rynx::ecs&>()(host); // also mark us as writing to the entire ecs. this is because adding/removing components can move any types of data around the ecs categories.
					}
				};

				// don't mark tasks as required resources. they are unique for each task, not shared.
				template<> struct unpack_resource<rynx::scheduler::task&> { void operator()(task&) {} };
				template<> struct unpack_resource<const rynx::scheduler::task&> { void operator()(task&) {} };

				void operator()(task& host) {
					(unpack_resource<Args>()(host), ...); // apply the resource constraints to the task.
				}

				template<typename T> inline auto& getTaskArgParam([[maybe_unused]] context* ctx, [[maybe_unused]] rynx::scheduler::task* currentTask) {
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
			
		public:
			// TODO: use some memory pool thing for task_resources.
			task() : m_name("EmptyTask"), m_context(nullptr) {}
			task(context& context, rynx::string name)
				: m_name(std::move(name))
				, m_context(&context)
				, m_barriers(rynx::make_shared<rynx::scheduler::operation_barriers>())
				, m_resources(rynx::make_shared<task_resources>(&context))
			{}
			
			template<typename F> task(context& context, rynx::string taskName, F&& op)
				: m_name(std::move(taskName))
				, m_context(&context)
				, m_barriers(rynx::make_shared<rynx::scheduler::operation_barriers>())
				, m_resources(rynx::make_shared<task_resources>(&context)) {
				set(std::forward<F>(op));
			}

			task(task&& other) = default;
			task& operator =(task&& other) = default;

			task clone() const; // copy is required in parallel_for case.
		
		private:
			task(const task& other); // copy is required in parallel_for case.

		public:
			operation_barriers& barriers() { return *m_barriers; }
			const operation_barriers& barriers() const { return *m_barriers; }

			operation_resources& resources() { return *m_resources; }
			const operation_resources& resources() const { return *m_resources; }

			// TODO: Rename this function.
			task& completion_blocked_by(task& other) {
				m_barriers->completion_blocked_by(other.m_barriers);
				return *this;
			}

			task& share_resources(task& other) {
				rynx_assert(other.m_resources_shared.empty(), "can't share more than one task's resources!");
				other.m_resources_shared = m_resources_shared;
				other.m_resources_shared.emplace_back(m_resources);
				return *this;
			}

			task& copy_barriers_from(task& other) {
				other.completion_blocked_by(*this);
				return *this;
			}

			task& copy_resources(task& other) {
				for(auto&& res : m_resources_shared) {
					other.m_resources->insert(*res);
				}
				other.m_resources->insert(*m_resources);
				return *this;
			}

			// task is not complete until barrier completes.
			// '&' denotes "both must be complete for dependent tasks to start".
			task& operator & (task_token& other);
			
			// '|' denotes pipe, a | b   == b comes after a.
			task& operator | (task_token& other);

			// TODO: Rename all task creation functions.
			template<typename F> task_token make_task(rynx::string name, F&& op) {
				task_token t = m_context->add_task(name, std::forward<F>(op));
				t->m_enable_logging = m_enable_logging;
				return t;
			}
			template<typename F> task_token make_task(F&& op) {
				return make_task("task", std::forward<F>(op));
			}

			template<typename F> task_token extend_task_independent(rynx::string name, F&& op) {
				task_token followUpTask = m_context->add_task(std::move(name), std::forward<F>(op));
				completion_blocked_by(*followUpTask);
				followUpTask->m_enable_logging = m_enable_logging;
				return followUpTask;
			}

			template<typename F> task_token extend_task_independent(F&& op) { return extend_task_independent(m_name + "_e", std::forward<F>(op)); }
			
			template<typename F> task_token make_extension_task_execute_sequential(rynx::string name, F&& op) {
				task_token followUpTask = m_context->add_task(std::move(name), std::forward<F>(op));
				completion_blocked_by(*followUpTask);
				copy_resources(*followUpTask); // uses same resources but must reserve them individually.
				followUpTask->m_enable_logging = m_enable_logging;
				return followUpTask;
			}
			
			template<typename F> task_token make_extension_task_execute_parallel(rynx::string name, F&& op) {
				task_token followUpTask = m_context->add_task(std::move(name), std::forward<F>(op));
				completion_blocked_by(*followUpTask);
				share_resources(*followUpTask); // use and extend parent reservation on resources.
				followUpTask->m_enable_logging = m_enable_logging;
				return followUpTask;
			}

			template<typename F> task_token extend_task_execute_parallel(rynx::string name, F&& op) { return make_extension_task_execute_parallel(std::move(name), std::forward<F>(op)); }
			template<typename F> task_token extend_task_execute_parallel(F&& op) { return extend_task_execute_parallel(m_name + "_es", std::forward<F>(op)); }
			template<typename F> task_token extend_task_execute_sequential(rynx::string name, F&& op) { return make_extension_task_execute_sequential(std::move(name), std::forward<F>(op)); }
			template<typename F> task_token extend_task_execute_sequential(F&& op) { return extend_task_execute_sequential(m_name + "_es", std::forward<F>(op)); }

			void run();

			operator bool() const { return static_cast<bool>(m_op); }
			const rynx::string& name() const { return m_name; }
			void clear() {
				m_op = nullptr;
				m_barriers.reset();
				m_for_each.clear();
				m_name.clear();
				m_resources.reset();
				m_resources_shared.clear();
			}

			void enable_log() {
				m_enable_logging = true;
			}

		private:

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
					barriers().depends_on(bar);

				for (auto&& bar : m_context->m_activeTaskBarriers)
					barriers().required_for(bar);
			}

			struct parallel_for_each_data {
				parallel_for_each_data(int64_t begin, int64_t end) noexcept : index(begin), end(end) {
					work_remaining = end - begin;
				}
				
				bool work_available() const noexcept {
					return index < end;
				}

				bool all_work_completed() const noexcept {
					return work_remaining <= 0;
				}

				int64_t get_work() noexcept {
					return index.fetch_add(work_size);
				}

				alignas(std::hardware_destructive_interference_size) std::atomic<int64_t> index; // used when reserving new chunk of work. is updated when starting to work.
				alignas(std::hardware_destructive_interference_size) std::atomic<int64_t> work_remaining; // used when checking if task is done. is updated when work chunk is completed.
				int64_t end;
				int64_t work_size = 1;
			};

		public:
			// NOTE: parallel_for_each is implemented as atomic index which means that the first time that a worker exits
			//       this task, is when all of the workload is being worked on. So there is no way that one task begins work on
			//       parallel for, does some work, exits the task, and looks for other work. This would leave the task in a state
			//       where its resources would no longer be reserved, but work would still need to be done.
			//       That would make life much more complicated. That is the reason why pre-divided work sizes
			//       approach to parallel fors was abandoned.
			
			struct parallel_for_operation {
				parallel_for_operation(rynx::scheduler::task& parent) : m_parent(&parent), m_ops(rynx::make_shared<std::vector<rynx::function<void()>>>()) {
					m_executor = rynx::make_unique<rynx::scheduler::task_token>(parent.extend_task_execute_parallel(this->m_parent->name() + "_pf", [ops = this->m_ops]() mutable {
						for (auto&& op : *ops) {
							op();
						}
					}));
					m_executor->required_for(m_barrier);
				}

				parallel_for_operation(parallel_for_operation&& other) = default;
				parallel_for_operation& operator = (parallel_for_operation&& other) = default;

				~parallel_for_operation() {
					if (m_executor) {
						m_executor.reset();
						if (m_parent->m_enable_logging) {
							logmsg("start parfor: %s", this->m_parent->name().c_str());
						}

						if(notify_workers)
							m_parent->notify_work_available();

						if (self_participate) {
							for (auto&& op : *m_ops) {
								op();
							}
						}
					}
				}


				rynx::scheduler::barrier barrier() const {
					return m_barrier;
				}

				rynx::observer_ptr<rynx::scheduler::task_token> task() const {
					return m_executor;
				}

				parallel_for_operation& range(int64_t begin_, int64_t end_, int64_t work_size_ = 256) {
					for_each_data = rynx::make_shared<parallel_for_each_data>(begin_, end_);
					for_each_data->work_size = work_size_;
					return *this;
				}

				parallel_for_operation& deferred_work() {
					self_participate = false;
					return *this;
				}

				parallel_for_operation& skip_worker_wakeup() {
					notify_workers = false;
					return *this;
				}

				template<typename F>
				parallel_for_operation& execute(F&& op) {
					rynx_assert(for_each_data != nullptr, "no operation range given. call range(..) first.");
					
					auto work_on_task = [for_each_data = this->for_each_data, op]() mutable  {
						const int64_t work_segment_size = for_each_data->work_size;
						const int64_t work_end = for_each_data->end;
						for (;;) {
							const int64_t work_segment_begin = for_each_data->get_work();
							if (work_segment_begin >= work_end) {
								return;
							}
							const int64_t work_segment_end = work_segment_begin + work_segment_size >= work_end ? work_end : work_segment_begin + work_segment_size;
							for (int64_t i = work_segment_begin; i < work_segment_end; ++i) {
								op(i);
							}
							for_each_data->work_remaining -= work_segment_end - work_segment_begin;
						}
					};

					(*m_executor)->m_for_each.emplace_back(for_each_data);
					m_ops->emplace_back(std::move(work_on_task));
					for_each_data.reset();
					return *this;
				}

			private:
				rynx::unique_ptr<rynx::scheduler::task_token> m_executor;
				rynx::scheduler::barrier m_barrier;

				rynx::scheduler::task* m_parent;
				rynx::shared_ptr<parallel_for_each_data> for_each_data;
				rynx::shared_ptr<std::vector<rynx::function<void()>>> m_ops;
				bool self_participate = true;

				// if you are creating multiple parallel for tasks with deferred_work, then might be better to
				// skip notify workers during task creation and just notify once after all tasks are created.
				bool notify_workers = true;
			};

			parallel_for_operation parallel() {
				return parallel_for_operation(*this);
			}

			explicit operator rynx::scheduler::context* () {
				return m_context;
			}

			explicit operator const rynx::scheduler::context* () const {
				return m_context;
			}

			bool is_for_each() const {
				return !m_for_each.empty();
			}

			bool for_each_all_work_completed() const {
				bool all_work_done = true;
				for (auto&& work_range : m_for_each)
					all_work_done &= work_range->all_work_completed();
				return all_work_done;
			}

			bool for_each_no_work_available() const {
				bool work_available = false;
				for (auto&& work_range : m_for_each)
					work_available |= work_range->work_available();
				return !work_available;
			}

		private:
			void notify_work_available() const;
			
			struct SchedulerDLL task_resources : public operation_resources {
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

			rynx::string m_name;
			rynx::function<void(rynx::scheduler::task*)> m_op;
			rynx::shared_ptr<operation_barriers> m_barriers;

			rynx::shared_ptr<task_resources> m_resources;
			std::vector<rynx::shared_ptr<task_resources>> m_resources_shared;
			std::vector<rynx::shared_ptr<parallel_for_each_data>> m_for_each;

			context* m_context = nullptr;
			bool m_enable_logging = false;
		};
	}
}
