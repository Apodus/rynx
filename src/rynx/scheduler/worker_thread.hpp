
#pragma once

#include <rynx/thread/semaphore.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>

#include <thread>
#include <atomic>

namespace rynx {

	namespace scheduler {
		class task_scheduler;

		class task_thread {
			std::thread m_thread;
			semaphore m_semaphore;
			task_scheduler* m_scheduler;
			task m_task;
			std::atomic<bool> m_sleeping = true;
			bool m_alive = false;

			void threadEntry(int myThreadIndex);

		public:

			task_thread(task_scheduler* pTaskMaster, int myIndex);
			~task_thread() {}

			void set_task(task task) { m_task = std::move(task); }
			const task& getTask() { return m_task; }

			void shutdown() {
				m_alive = false;
				while (!is_sleeping()) {
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
				}
				m_semaphore.signal();
				m_thread.join();
			}

			bool wake_up() {
				if (m_sleeping.exchange(false)) {
					m_semaphore.signal();
					return true;
				}
				return false;
			}

			void wait() {
				m_semaphore.wait();
			}

			bool is_sleeping() const {
				return m_sleeping;
			}
		};
	}
}
