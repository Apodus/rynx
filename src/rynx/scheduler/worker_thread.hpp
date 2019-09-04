
#pragma once

#include <rynx/thread/semaphore.hpp>
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
			std::atomic<bool> m_done = true;
			bool m_running = false;

			void threadEntry(int myThreadIndex);

		public:

			task_thread(task_scheduler* pTaskMaster, int myIndex);
			~task_thread() {}

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
	}
}