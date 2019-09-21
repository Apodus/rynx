
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/barrier.hpp>


#include <iostream>

void rynx::scheduler::context::dump() {
	std::lock_guard<std::mutex> lock(m_taskMutex);
	std::cout << m_tasks.size() << " tasks remaining" << std::endl;
	for (auto&& task : m_tasks) {
		(void)task;
		logmsg("%s barriers: %d, resources: %d", task.second.name().c_str(), task.second.barriers().canStart(), resourcesAvailableFor(task.second));
	}
}