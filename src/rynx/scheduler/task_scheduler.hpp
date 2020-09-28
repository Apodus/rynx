
#pragma once

#include <rynx/tech/unordered_map.hpp>
#include <rynx/system/assert.hpp>

#include <rynx/scheduler/barrier.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
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

			bool find_work_for_thread_index(int threadIndex); // this is only allowed to be called from within the worker. TODO: architecture.
			void wake_up_sleeping_workers();

		public:

			task_scheduler();
			~task_scheduler();

			// TODO: this should probably return some wrapper object, instead of a pointer. now user might think that he needs to delete it.
			scheduler::context* make_context();

			void checkComplete();

			int32_t worker_count() const {
				return numThreads;
			}

			// called once per frame.
			void wait_until_complete() {
				m_waitForComplete.wait();
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
				rynx_assert(m_frameComplete.load() == 1, "mismatch with scheduler starts and waits");
				m_frameComplete.store(0);
				wake_up_sleeping_workers();
			}

			void dump();
		};
	}
}
