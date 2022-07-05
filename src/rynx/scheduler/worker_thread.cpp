
#include <rynx/scheduler/worker_thread.hpp>
#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/thread/this_thread.hpp>

#include <thread>

rynx::scheduler::task_thread::task_thread(task_scheduler* pTaskMaster, int myIndex) {
	m_scheduler = pTaskMaster;
	m_sleeping = true;
	m_alive = true;
	m_thread = rynx::make_opaque_unique_ptr<std::thread>(std::thread([this, myIndex]() {
		threadEntry(myIndex);
	}));
}

rynx::scheduler::task_thread::~task_thread() {}

void rynx::scheduler::task_thread::set_task(task task) { m_task = std::move(task); }
const rynx::scheduler::task& rynx::scheduler::task_thread::getTask() { return m_task; }

void rynx::scheduler::task_thread::shutdown() {
	m_alive = false;
	while (!is_sleeping()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	m_semaphore.signal();
	m_thread->join();
}

bool rynx::scheduler::task_thread::wake_up() {
	if (m_sleeping.exchange(false)) {
		m_semaphore.signal();
		return true;
	}
	return false;
}

void rynx::scheduler::task_thread::wait() {
	m_semaphore.wait();
}

bool rynx::scheduler::task_thread::is_sleeping() const {
	return m_sleeping;
}


void rynx::scheduler::task_thread::threadEntry(int myThreadIndex) {
	rynx::this_thread::rynx_thread_raii rynx_thread_utilities_required_token;
	while (m_alive) {
		wait();

		m_scheduler->worker_activated();
		while (m_scheduler->find_work_for_thread_index(myThreadIndex)) {
			m_scheduler->wake_up_sleeping_workers();
			m_task.run();
			m_task.clear();
		}
		
		m_sleeping.store(true);
		m_scheduler->checkComplete();
		m_scheduler->worker_deactivated();
	}
	logmsg("task thread exit");
}
