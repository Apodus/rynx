
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <iostream>

rynx::scheduler::task rynx::scheduler::context::findWork() {
	rynx_profile("Profiler", "Find work self");
	std::lock_guard<std::mutex> lock(m_taskMutex);
	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
		auto& task = it->second;
		if (task.barriers().can_start() && resourcesAvailableFor(task)) {
			if (!it->second.is_for_each()) {
				rynx::scheduler::task t(std::move(it->second));
				m_tasks.erase(it);
				t.reserve_resources();
				return t;
			}
			else {
				// is for each task
				if (it->second.is_for_each_done()) {
					// NOTE: this depends on special properties of rynx unordered_map iterators not being invalidated on erase.
					it->second.barriers().on_complete();
					m_tasks.erase(it);
				}
				else {
					rynx::scheduler::task copy = it->second;
					it->second.completion_blocked_by(copy);
					return copy;
				}
			}
		}
	}
	return task();
}

void rynx::scheduler::context::erase_completed_parallel_for_tasks() {
	rynx_profile("Profiler", "Erase completed parallel for tasks");
	std::lock_guard<std::mutex> lock(m_taskMutex);
	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
		if (it->second.is_for_each()) {
			if (it->second.is_for_each_done()) {
				// NOTE: this depends on special properties of rynx unordered_map iterators not being invalidated on erase.
				it->second.barriers().on_complete();
				m_tasks.erase(it);
			}
		}
	}
}

void rynx::scheduler::context::dump() {
	std::lock_guard<std::mutex> lock(m_taskMutex);
	std::cout << m_tasks.size() << " tasks remaining" << std::endl;
	for (auto&& task : m_tasks) {
		(void)task;
		logmsg("%s barriers: %d, resources: %d", task.second.name().c_str(), task.second.barriers().can_start(), resourcesAvailableFor(task.second));
	}
}
