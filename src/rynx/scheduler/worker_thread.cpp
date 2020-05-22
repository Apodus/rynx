
#include <rynx/scheduler/worker_thread.hpp>
#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/thread/this_thread.hpp>

#include <thread>

rynx::scheduler::task_thread::task_thread(task_scheduler* pTaskMaster, int myIndex) {
	m_scheduler = pTaskMaster;
	m_sleeping = true;
	m_alive = true;
	m_thread = std::thread([this, myIndex]() {
		threadEntry(myIndex);
	});
}


void rynx::scheduler::task_thread::threadEntry(int myThreadIndex) {
	rynx::this_thread::rynx_thread_raii rynx_thread_utilities_required_token;
	while (m_alive) {
		if (!m_task)
			wait();

		while (m_scheduler->find_work_for_thread_index(myThreadIndex)) {
			m_scheduler->wake_up_sleeping_workers();
			m_task.run();
			m_task.clear();
		}
		
		m_sleeping.store(true);
		m_scheduler->checkComplete();
	}
	logmsg("task thread exit");
}
