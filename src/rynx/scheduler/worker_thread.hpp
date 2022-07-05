
#pragma once

#include <rynx/thread/semaphore.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/tech/std/memory.hpp>
#include <atomic>

namespace std {
	class thread;
}

namespace rynx {
	namespace scheduler {
		class task_scheduler;

		class task_thread {
			rynx::opaque_unique_ptr<std::thread> m_thread;
			semaphore m_semaphore;
			task_scheduler* m_scheduler;
			task m_task;
			std::atomic<bool> m_sleeping = true;
			bool m_alive = false;

			void threadEntry(int myThreadIndex);

		public:

			task_thread(task_scheduler* pTaskMaster, int myIndex);
			~task_thread();

			void set_task(task task);
			const task& getTask();

			void shutdown();
			bool wake_up();
			void wait();
			bool is_sleeping() const;
		};
	}
}
