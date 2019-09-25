
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <iostream>

rynx::scheduler::task rynx::scheduler::context::findWork() {
	rynx_profile("Profiler", "Find work self");
	std::lock_guard<std::mutex> lock(m_taskMutex);
	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
		auto& task = it->second;
		if (task.barriers().canStart() && resourcesAvailableFor(task)) {
			rynx::scheduler::task t(std::move(it->second));
			m_tasks.erase(it);
			t.reserve_resources();
			return t;
		}
	}
	return task();
}

void rynx::scheduler::context::dump() {
	std::lock_guard<std::mutex> lock(m_taskMutex);
	std::cout << m_tasks.size() << " tasks remaining" << std::endl;
	for (auto&& task : m_tasks) {
		(void)task;
		logmsg("%s barriers: %d, resources: %d", task.second.name().c_str(), task.second.barriers().canStart(), resourcesAvailableFor(task.second));
	}
}
