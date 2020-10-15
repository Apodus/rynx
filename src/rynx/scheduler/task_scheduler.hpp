
#pragma once

#include <rynx/tech/unordered_map.hpp>
#include <rynx/system/assert.hpp>

#include <rynx/scheduler/barrier.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/deadlock_detector.hpp>
#include <rynx/thread/semaphore.hpp>

#include <array>
#include <atomic>
#include <string>
#include <memory>
#include <functional>

namespace rynx {
	namespace scheduler {
		class context;
		class task_thread;
		class task;

		class task_scheduler {

			friend class scheduler::context;
			friend class scheduler::task_thread;
			friend class scheduler::task;

			// TODO: fix the number of threads here later if necessary.
			static constexpr int numThreads = 4;

		private:
			std::atomic<scheduler::context::context_id> m_contextIdGen = 0;
			rynx::unordered_map<scheduler::context::context_id, std::unique_ptr<scheduler::context>> m_contexts;
			std::array<rynx::scheduler::task_thread*, numThreads> m_threads;
			semaphore m_waitForComplete;
			std::mutex m_task_starting_mutex;
			
			std::atomic<int32_t> m_frameComplete = 1; // initially the scheduler is in a "frame completed" state.
			uint64_t m_activeFrame = 0;
			std::atomic<int32_t> m_active_workers = 0; // starting a new frame is not allowed until all workers are inactive.
			
			dead_lock_detector m_deadlock_detector;


			bool find_work_for_thread_index(int threadIndex); // this is only allowed to be called from within the worker. TODO: architecture.
			void wake_up_sleeping_workers();

			void worker_activated();
			void worker_deactivated();

		public:

			task_scheduler();
			~task_scheduler();

			// No move, no copy.
			task_scheduler(task_scheduler&& other) = delete;
			task_scheduler(const task_scheduler& other) = delete;
			task_scheduler& operator =(task_scheduler&& other) = delete;
			task_scheduler& operator =(const task_scheduler& other) = delete;

			// TODO: this should probably return some wrapper object, instead of a pointer. now user might think that he needs to delete it.
			scheduler::context* make_context();

			bool checkComplete();
			uint64_t tick_counter() const { return m_activeFrame; }
			operator uint64_t() const { return m_activeFrame; }

			int32_t worker_count() const {
				return numThreads;
			}

			// called once per frame.
			void wait_until_complete() {
				m_waitForComplete.wait();
				rynx_assert(checkComplete(), "wait interrupted ahead of time!");
				
				// now all workers are stopped, and we can update our type indices.
				for (auto& ctx : m_contexts) {
					ctx.second->m_typeIndex.sync();
					ctx.second->m_resources.sync();
					auto* ecs_resource = ctx.second->m_resources.try_get<rynx::ecs>();
					if (ecs_resource) {
						ecs_resource->sync_type_index();
					}
				}

				// logmsg("frame complete\n");
			}

			// called once per frame.
			void start_frame() {
				// todo: get rid of busyloop?
				while (m_active_workers.load() != 0) {}

				rynx_assert(m_frameComplete.load() == 1, "mismatch with scheduler starts and waits");
				m_frameComplete.store(0);

				wake_up_sleeping_workers();

				++m_activeFrame;
			}

			void dump();
		};
	}
}
