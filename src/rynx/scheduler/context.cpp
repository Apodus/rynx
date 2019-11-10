
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <iostream>

rynx::scheduler::task rynx::scheduler::context::findWork() {
	rynx_profile("Profiler", "Find work self");
	std::lock_guard<std::mutex> lock(m_taskMutex);
	
	// keep track of available for each tasks, but prefer an independent task if they are available.
	size_t for_each_task_index = ~size_t(0);
	for (size_t i = 0; i < m_tasks.size(); ++i) {
		auto& task = m_tasks[i];
		if (task.barriers().can_start() && resourcesAvailableFor(task)) {
			if (!task.is_for_each()) {
				rynx::scheduler::task t(std::move(task));
				m_tasks[i] = std::move(m_tasks.back());
				m_tasks.pop_back();
				t.reserve_resources();
				return t;
			}
			else {
				// is for each task
				if (task.is_for_each_done()) {
					task.barriers().on_complete();
					m_tasks[i] = std::move(m_tasks.back());
					m_tasks.pop_back();
				}
				else {
					for_each_task_index = i;
				}
			}
		}
	}
	
	if (for_each_task_index != ~size_t(0)) {
		task& task = m_tasks[for_each_task_index];
		rynx::scheduler::task copy = task;
		task.completion_blocked_by(copy);
		return copy;
	}
	
	return task();
}

void rynx::scheduler::context::erase_completed_parallel_for_tasks() {
	rynx_profile("Profiler", "Erase completed parallel for tasks");
	std::lock_guard<std::mutex> lock(m_taskMutex);
	for (size_t i = 0; i< m_tasks.size(); ++i) {
		rynx::scheduler::task& task = m_tasks[i];
		if (task.is_for_each()) {
			if (task.is_for_each_done()) {
				task.barriers().on_complete();
				
				m_tasks[i] = std::move(m_tasks.back());
				m_tasks.pop_back();
			}
		}
	}
}

void rynx::scheduler::context::dump() {
	std::lock_guard<std::mutex> lock(m_taskMutex);
	std::cout << m_tasks.size() << " tasks remaining" << std::endl;
	for (auto&& task : m_tasks) {
		(void)task;
		logmsg("%s barriers: %d, resources: %d", task.name().c_str(), task.barriers().can_start(), resourcesAvailableFor(task));
	}
}
