
#pragma once

#include <rynx/tech/unordered_map.hpp>
#include <rynx/system/assert.hpp>

#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/barrier.hpp>
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
			static constexpr int numThreads = 8;

		private:
			std::atomic<scheduler::context::context_id> m_contextIdGen = 0;
			rynx::unordered_map<scheduler::context::context_id, std::unique_ptr<scheduler::context>> m_contexts;
			std::array<rynx::scheduler::task_thread*, numThreads> m_threads;
			std::mutex m_worker_mutex;
			semaphore m_waitForComplete;
			std::atomic<int32_t> m_frameComplete = 1; // initially the scheduler is in a "frame completed" state.

			int getFreeThreadIndex();
			void startTask(int threadIndex, rynx::scheduler::task& task);
			bool findWorkForThreadIndex(int threadIndex);
			void findWorkForAllThreads();

		public:

			task_scheduler();
			~task_scheduler();

			// TODO: this should probably return some wrapper object, instead of a pointer. now user might think that he needs to delete it.
			scheduler::context* make_context();

			void checkComplete();

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
}