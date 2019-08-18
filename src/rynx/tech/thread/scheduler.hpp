
#pragma once

// required for scheduler data ownership idea.
#include <rynx/tech/ecs.hpp>
#include <rynx/tech/object_storage.hpp>

#include <rynx/tech/thread/Semaphore.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/system/assert.hpp>

#include <array>
#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#include <iostream>

namespace rynx {
	namespace thread {

		struct barrier {
			barrier(std::string name) : name(std::move(name)), counter(std::make_shared<std::atomic<int32_t>>(0)) {}
			barrier(const barrier& other) = default;
			barrier(barrier&& other) = default;
			barrier& operator = (const barrier& other) = default;
			barrier& operator = (barrier&& other) = default;

			operator bool() const { return *counter == 0; }

			std::string name;
			std::shared_ptr<std::atomic<int32_t>> counter; // counter object is shared by all copies of the same barrier.
		};

		class operation_barriers {
		public:
			operation_barriers& require(barrier bar) {
				m_requires.emplace_back(std::move(bar));
				return *this;
			}

			operation_barriers& block(barrier bar) {
				++*bar.counter;
				m_blocks.emplace_back(std::move(bar));
				return *this;
			}

			bool canStart() {
				while (!m_requires.empty()) {
					if (m_requires.front()) {
						m_requires[0] = std::move(m_requires.back()); // requirement met. erase it so we don't check it again.
						m_requires.pop_back();
					}
					else {
						return false;
					}
				}
				return true;
			}

			void onComplete() {
				for (auto&& bar : m_blocks) {
					--*bar.counter;
				}
			}

			// in cases where we are running a task, and notice that there is some additional work with different resources
			// that must be done before we can say the first task is complete, we can say completion_blocked_by(other)
			void completion_blocked_by(operation_barriers& other) {
				for (auto& bar : m_blocks) {
					other.block(bar);
				}
			}

		private:
			std::vector<barrier> m_requires; // barriers that must be completed before starting this task.
			std::vector<barrier> m_blocks; // barriers that are not complete without this task.
		};

		class operation_resources {
			std::vector<uint64_t> m_writeAccess;
			std::vector<uint64_t> m_readAccess;
		
		public:
			operation_resources& requireWrite(uint64_t resourceId) {
				m_writeAccess.emplace_back(resourceId);
				return *this;
			}

			operation_resources& requireRead(uint64_t resourceId) {
				m_readAccess.emplace_back(resourceId);
				return *this;
			}

			const std::vector<uint64_t>& readRequirements() const { return m_readAccess; }
			const std::vector<uint64_t>& writeRequirements() const { return m_writeAccess; }
		};

		class task_scheduler {
			
			// TODO: fix the number of threads here later if necessary.
			static constexpr int numThreads = 4;
			
		public:
			class task;

			// NOTE: Task_tokens do not work at all if used while scheduler is running. Might be better to just store task pointers and return those?
			class context;
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
				friend class rynx::thread::task_scheduler;

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
							unpack_resource<const rynx::ecs&>(); // also mark us as reading the ecs. this prevents any view tasks to run simultaneously with a mutable ecs& task.
						}
					};

					template<typename... Args>
					struct unpack_resource<rynx::ecs::edit_view<Args...>> {
						void operator()(task& host) {
							(unpack_resource<Args>()(host), ...);
							unpack_resource<rynx::ecs&>(); // also mark us as writing to the entire ecs. this is because adding/removing components can move any types of data around the ecs chunks.
						}
					};

					void operator()(task& host) {
						(unpack_resource<Args>()(host), ...); // apply the resource constraints to the task.
					}

					template<typename T> inline auto& getTaskArgParam(context* ctx, rynx::thread::task_scheduler::task* currentTask) {
						(void)currentTask; // trust me mr compiler. they are both useful parameters.
						(void)ctx;
						if constexpr (!std::is_same_v<std::remove_cvref_t<T>, rynx::thread::task_scheduler::task>) {
							return ctx->getTaskArgParam<T>();
						}
						else {
							return *currentTask;
						}
					}

					auto fetchParams(context* ctx, rynx::thread::task_scheduler::task* currentTask) {
						return std::forward_as_tuple(getTaskArgParam<Args>(ctx, currentTask)...);
					}
				};

				void reserveResources() const { m_context->reserveResources(*this); }

				task& id(uint64_t taskId) { m_id = taskId; return *this; }

			public:
				task() : m_name("EmptyTask"), m_context(nullptr) {}
				task(context& context, uint64_t taskId, std::string name) : m_name(std::move(name)), m_id(taskId), m_context(&context) {}
				template<typename F> task(context& context, uint64_t taskId, std::string taskName, F&& op) : m_name(std::move(taskName)), m_id(taskId), m_context(&context) { set(std::forward<F>(op)); }

				uint64_t id() const { return m_id; }

				operation_barriers& barriers() { return m_barriers; }
				const operation_barriers& barriers() const { return m_barriers; }

				operation_resources& resources() { return m_resources; }
				const operation_resources& resources() const { return m_resources; }

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

				template<typename F>
				task& extend_task(F&& op) {
					auto followUpTask = m_context->makeTask(m_name + "_e", std::forward<F>(op));
					completion_blocked_by(followUpTask);
					m_context->addTask(std::move(followUpTask));
					return *this;
				}

				template<typename F>
				task_token then(std::string name, F&& f) {
					auto followUpTask = m_context->makeTask(std::move(name), std::forward<F>(f));
					followUpTask.dependsOn(*this);
					return m_context->addTask(std::move(followUpTask));
				}

				task(task&& other) = default;
				task& operator =(task&& other) = default;

				template<typename F>
				void set(F&& f) {
					resource_deducer<decltype(&F::operator())>()(*this);
					context* ctx = m_context;
					
					m_op = [ctx, f = std::move(f)](rynx::thread::task_scheduler::task* myTask) {
						auto resources = resource_deducer<decltype(&F::operator())>().fetchParams(ctx, myTask);
						std::apply(f, resources);
					};

					// apply current barrier states.
					for (auto&& bar : m_context->m_activeTaskBarriers_Dependencies)
						this->after(bar);

					for (auto&& bar : m_context->m_activeTaskBarriers)
						this->before(bar);
				}

				void run() {
					rynx_assert(m_op, "no op in task that is being run!");
					rynx_assert(m_barriers.canStart(), "task is being run while still blocked by barriers!");

					m_op(this);
					m_barriers.onComplete();
					m_context->releaseResources(*this);
				}

				operator bool() const { return static_cast<bool>(m_op); }
				void clear() { m_op = nullptr; }

			private:
				std::string m_name;
				uint64_t m_id;
				std::function<void(rynx::thread::task_scheduler::task*)> m_op;
				operation_barriers m_barriers;
				operation_resources m_resources;
				context* m_context = nullptr;
			};

			class scoped_barrier_after;
			class scoped_barrier_before;

			// NOTE: adding new tasks is allowed while scheduler is running.
			class context {
				friend struct rynx::thread::task_scheduler::task_token;
				friend class rynx::thread::task_scheduler::task;
				friend class rynx::thread::task_scheduler;

				friend class scoped_barrier_after;
				friend class scoped_barrier_before;

				rynx::type_index m_typeIndex;
				std::atomic<uint64_t> m_nextTaskId = 0;

				struct resource_state {
					std::atomic<int> readers = 0;
					std::atomic<int> writers = 0;
				};

				template<typename T>
				struct resource_getter {
					T& operator()(context* context_) {
						return context_->getResource<std::remove_reference_t<T>>();
					}
				};

				template<typename...Args>
				struct resource_getter<rynx::ecs::view<Args...>> {
					rynx::ecs& operator()(context* context_) {
						return context_->getResource<rynx::ecs>();
					}
				};

				template<typename...Args>
				struct resource_getter<rynx::ecs::edit_view<Args...>> {
					rynx::ecs& operator()(context* context_) {
						return context_->getResource<rynx::ecs>();
					}
				};

				template<typename T>
				auto& getTaskArgParam() {
					return resource_getter<T>()(this);
				}

				rynx::unordered_map<uint64_t, task> m_tasks;
				std::vector<barrier> m_activeTaskBarriers; // barriers that depend on any task that is created while they are here.
				std::vector<barrier> m_activeTaskBarriers_Dependencies; // new tasks are not allowed to run until these barriers are complete.
				std::unordered_map<uint64_t, resource_state> m_resource_counters;

				rynx::object_storage m_resources;
				std::mutex m_taskMutex;

				// these are not thread safe when they add new elements to the map.
				void releaseResources(const task& task) {
					for (uint64_t readResource : task.resources().readRequirements()) {
						--m_resource_counters[readResource].readers;
					}
					for (uint64_t writeResource : task.resources().writeRequirements()) {
						--m_resource_counters[writeResource].writers;
					}
				}

				void reserveResources(const task& task) {
					for (uint64_t readResource : task.resources().readRequirements()) {
						++m_resource_counters[readResource].readers;
					}
					for (uint64_t writeResource : task.resources().writeRequirements()) {
						++m_resource_counters[writeResource].writers;
					}
				}

				task findWork();
				uint64_t nextTaskId() { return m_nextTaskId.fetch_add(1); }

			public:

				bool resourcesAvailableFor(const task& t) const {
					bool okToStart = true;

					for (uint64_t readResource : t.resources().readRequirements()) {
						auto it = m_resource_counters.find(readResource);
						if (it != m_resource_counters.end()) {
							okToStart &= (it->second.writers == 0);
						}
					}
					for (uint64_t writeResource : t.resources().writeRequirements()) {
						auto it = m_resource_counters.find(writeResource);
						if (it != m_resource_counters.end()) {
							auto& resourceState = it->second;
							okToStart &= ((resourceState.readers == 0 && resourceState.writers == 0));
						}
					}
					return okToStart;
				}

				context() {
					setResource<context>(this);
				}

				bool isFinished() const { return m_tasks.empty(); }

				template<typename T> context& setResource(T* t) {
					m_resources.set_and_release(t);
					return *this;
				}

				template<typename T> T& getResource() {
					return m_resources.get<T>();
				}

				template<typename F> task makeTask(std::string taskName, F&& op) { return task(*this, nextTaskId(), std::move(taskName), std::forward<F>(op)); }
				task makeTask(std::string taskName) { return task(*this, nextTaskId(), std::move(taskName)); }

				task_token addTask(task task) {
					std::lock_guard<std::mutex> lock(m_taskMutex);
					uint64_t taskId = task.id();
					m_tasks.emplace(taskId, std::move(task));
					return task_token(this, taskId);
				}

				template<typename F>
				task_token addTask(std::string taskName, F&& taskOp) {
					return addTask(task(*this, nextTaskId(), std::move(taskName), std::forward<F>(taskOp)));
				}

				void dump() {
					std::lock_guard<std::mutex> lock(m_taskMutex);
					std::cout << m_tasks.size() << " tasks remaining" << std::endl;
					for (auto&& task : m_tasks) {
						std::cout << task.second.m_name << " " << task.second.m_barriers.canStart() << " " << resourcesAvailableFor(task.second) << std::endl;
					}
				}
			};

			// this barrier is attached to end of any new task added to scheduler while in scope.
			// this barrier depends on tasks created.
			class scoped_barrier_after {
			public:
				scoped_barrier_after(context& context, barrier bar) : m_context(&context) {
					m_context->m_activeTaskBarriers.emplace_back(std::move(bar));
				}
				~scoped_barrier_after() {
					m_context->m_activeTaskBarriers.pop_back();
				}

			private:
				context* m_context;
			};

			// this barrier is attached to front of any new task added to scheduler while in scope.
			// created tasks depend on this barrier.
			class scoped_barrier_before {
			public:
				scoped_barrier_before(context& context) : m_context(&context) {}

				~scoped_barrier_before() {
					while (m_dependenciesAdded > 0) {
						m_context->m_activeTaskBarriers_Dependencies.pop_back();
						--m_dependenciesAdded;
					}
				}

				scoped_barrier_before& addDependency(rynx::thread::barrier bar) {
					m_context->m_activeTaskBarriers_Dependencies.emplace_back(std::move(bar));
					++m_dependenciesAdded;
					return *this;
				}

			private:
				context* m_context;
				int m_dependenciesAdded = 0;
			};

		private:
			class task_thread {
				std::thread m_thread;
				semaphore m_semaphore;
				task_scheduler* m_scheduler;
				task m_task;
				std::atomic<bool> m_done = true;
				bool m_running = false;

				void threadEntry(int myThreadIndex) {
					while (m_running) {
						if (!m_task)
							wait();
						
						if (m_task) {
							m_task.run();
							m_task.clear();

							// if we can't find work, then we are done and others are allowed to find work for us.
							m_done = true;
							m_scheduler->findWorkForThreadIndex(myThreadIndex);

							if (!m_done) {
								// if we did find work, we should also check if we can find work for our friends.
								m_scheduler->findWorkForAllThreads();
							}
							else {
								m_scheduler->checkComplete();
							}
						}
					}
					std::cout << "task thread exiting" << std::endl;
				}

			public:

				task_thread(task_scheduler* pTaskMaster, int myIndex) {
					m_scheduler = pTaskMaster;
					m_done = true;
					m_running = true;
					m_thread = std::thread([this, myIndex]() {
						threadEntry(myIndex);
					});
				}

				~task_thread() {
					std::cout << "task thread deleter" << std::endl;
				}

				void setTask(task task) { m_task = std::move(task); }
				const task& getTask() { return m_task; }

				void shutdown() {
					m_running = false;
					while (!isDone()) {
						std::this_thread::sleep_for(std::chrono::milliseconds(50));
					}
					m_semaphore.signal();
					m_thread.join();
				}

				void start() {
					m_done = false;
					m_semaphore.signal();
				}

				void wait() {
					m_semaphore.wait();
				}

				bool isDone() const {
					return m_done;
				}
			};

			rynx::unordered_map<std::string, std::unique_ptr<context>> m_contexts;
			std::array<task_thread*, numThreads> m_threads;
			std::mutex m_worker_mutex;
			semaphore m_waitForComplete;
			std::atomic<int32_t> m_frameComplete = 1;

			int getFreeThreadIndex() {
				for (int i = 0; i < numThreads; ++i) {
					if (m_threads[i]->isDone()) {
						return i;
					}
				}
				return -1;
			}

			void startTask(int threadIndex, task& task) {
				task.reserveResources();
				m_threads[threadIndex]->setTask(std::move(task));
				m_threads[threadIndex]->start();
			}

			bool findWorkForThreadIndex(int threadIndex) {
				// TODO: We should have separate mutexes for each thread index. Locking all is bad sense.
				std::lock_guard<std::mutex> lock(m_worker_mutex);
				if (m_threads[threadIndex]->isDone()) {
					for (auto& context : m_contexts) {
						task t = context.second->findWork();
						if (t) {
							startTask(threadIndex, t);
							return true;
						}
					}
				}
				return false;
			}

			void findWorkForAllThreads() {
				for (;;) {
					int freeThreadIndex = getFreeThreadIndex();
					if (freeThreadIndex >= 0) {
						if (!findWorkForThreadIndex(freeThreadIndex)) {
							return;
						}
					}
					else {
						// logmsg("task execution was requested but no task threads are free :(");
						return;
					}
				}
			}

		public:

			task_scheduler() : m_threads({nullptr}) {
				for (int i = 0; i < numThreads; ++i) {
					m_threads[i] = new task_thread(this, i);
				}
			}

			~task_scheduler() {
				for (int i = 0; i < numThreads; ++i) {
					m_threads[i]->shutdown();
					delete m_threads[i];
				}
			}

			// TODO: this should probably return some wrapper object, instead of a pointer. now user might think that he needs to delete it.
			context* make_context(std::string context_name) {
				rynx_assert(m_contexts.find(context_name) == m_contexts.end(), "creating a context but a context by this name exists already");
				return m_contexts.emplace(context_name, std::make_unique<context>()).first->second.get();
			}

			void checkComplete() {
				bool contextsHaveNoQueuedTasks = true;
				for (auto& context : m_contexts) {
					contextsHaveNoQueuedTasks &= context.second->isFinished();
				}

				if (contextsHaveNoQueuedTasks) {
					bool allWorkersAreFinished = true;
					for (auto&& worker : m_threads) {
						allWorkersAreFinished &= worker->isDone();
					}

					if (allWorkersAreFinished) {
						// in case multiple workers are detecting completion simultaneously.
						if (m_frameComplete.exchange(1) == 0) {
							m_waitForComplete.signal();
						}
					}
				}
			}

			// called once per frame.
			void wait_until_complete() {
				m_waitForComplete.wait();
			}

			// called once per frame.
			void start_frame() {
				rynx_assert(m_frameComplete == 1, "mismatch with scheduler starts and waits");
				m_frameComplete = 0;
				findWorkForAllThreads();
			}

			void dump() {
				for (auto& ctx : m_contexts) {
					ctx.second->dump();
				}
			}
		};
	}

	// shorthands for user.
	using scheduler = thread::task_scheduler;
	using scheduler_task = thread::task_scheduler::task;
	using scheduler_context = thread::task_scheduler::context; 
}