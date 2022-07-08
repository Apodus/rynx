
#pragma once

#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/std/string.hpp>
#include <rynx/system/assert.hpp>

#include <rynx/scheduler/barrier.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/deadlock_detector.hpp>
#include <rynx/thread/semaphore.hpp>

#include <vector>

namespace rynx {
	namespace scheduler {
		class context;
		class task_thread;
		class task;

		class task_scheduler {
		public:
			friend class scheduler::context;
			friend class scheduler::task_thread;
			friend class scheduler::task;

		private:
			std::atomic<scheduler::context::context_id> m_contextIdGen = 0;
			rynx::unordered_map<scheduler::context::context_id, rynx::unique_ptr<scheduler::context>> m_contexts;
			std::vector<rynx::scheduler::task_thread*> m_threads;
			
			uint64_t m_activeFrame = 0;
			std::atomic<int32_t> m_frameComplete = 1; // initially the scheduler is in a "frame completed" state.
			std::atomic<int32_t> m_active_workers = 0; // starting a new frame is not allowed until all workers are inactive.
			semaphore m_waitForComplete;
			
			dead_lock_detector m_deadlock_detector;


			bool find_work_for_thread_index(int threadIndex); // this is only allowed to be called from within the worker. TODO: architecture.
			void wake_up_sleeping_workers();

			void worker_activated();
			void worker_deactivated();

		public:

			task_scheduler(uint64_t numWorkers = 16);
			~task_scheduler();

			// No move, no copy.
			task_scheduler(task_scheduler&& other) = delete;
			task_scheduler(const task_scheduler& other) = delete;
			task_scheduler& operator =(task_scheduler&& other) = delete;
			task_scheduler& operator =(const task_scheduler& other) = delete;

			rynx::observer_ptr<scheduler::context> make_context();

			bool checkComplete();
			uint64_t tick_counter() const { return m_activeFrame; }
			operator uint64_t() const { return m_activeFrame; }

			uint64_t worker_count() const {
				return m_threads.size();
			}

			// called once per frame.
			void wait_until_complete() {
				m_waitForComplete.wait();
				rynx_assert(checkComplete(), "wait interrupted ahead of time!");
			}

			// called once per frame.
			void start_frame() {
				// NOTE: Frame start/end should be property of a scheduling context.
				// NOTE: Workers can be loaded by other scheduling contexts when another one starts, this assertion should be removed.
				rynx_assert(m_active_workers.load() == 0, "workers are still active when starting frame");
				rynx_assert(m_frameComplete.load() == 1, "mismatch with scheduler starts and waits");
				m_frameComplete.store(0);
				wake_up_sleeping_workers();
				++m_activeFrame;
			}

			void dump();
		};
	}
}
