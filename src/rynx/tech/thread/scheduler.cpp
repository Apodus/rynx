
#include <rynx/tech/thread/scheduler.hpp>

rynx::thread::task_scheduler::task* rynx::thread::task_scheduler::task_token::operator -> () { return &m_context->m_tasks.find(m_taskId)->second; }

rynx::thread::task_scheduler::task rynx::thread::task_scheduler::context::findWork() {
	std::lock_guard<std::mutex> lock(m_taskMutex);
	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
		auto& task = it->second;
		if (task.barriers().canStart() && resourcesAvailableFor(task)) {
			rynx::thread::task_scheduler::task t(std::move(it->second));
			m_tasks.erase(it);
			return t;
		}
	}
	return task();
}