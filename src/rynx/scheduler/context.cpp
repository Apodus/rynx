
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
				if (task.for_each_no_work_available()) {
					if (task.for_each_all_work_completed())
					{
						task.barriers().on_complete();
						m_tasks[i] = std::move(m_tasks.back());
						m_tasks.pop_back();
						
						// need to check earlier tasks again,
						// in case for each task unblocked some of them.
						i = 0;
						for_each_task_index = ~size_t(0);
					}
				}
				else {
					// always work on the first available parallel for op. this way we maximize the speed at which tasks get unblocked.
					if (for_each_task_index == ~size_t(0)) {
						for_each_task_index = i;
					}
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

rynx::scheduler::task rynx::scheduler::context::make_task(std::string taskName) {
	return task(*this, std::move(taskName));
}

rynx::scheduler::task_token rynx::scheduler::context::add_task(task task) {
	return task_token(std::move(task));
}

bool rynx::scheduler::context::resourcesAvailableFor(const task& t) const {
	int resources_in_use = 0;
	for (uint64_t readResource : t.resources().read_requirements()) {
		resources_in_use += m_resource_counters[readResource].writers.load();
	}
	for (uint64_t writeResource : t.resources().write_requirements()) {
		resources_in_use += m_resource_counters[writeResource].writers.load();
		resources_in_use += m_resource_counters[writeResource].readers.load();
	}
	return resources_in_use == 0;
}

void rynx::scheduler::context::erase_completed_parallel_for_tasks() {
	rynx_profile("Profiler", "Erase completed parallel for tasks");
	std::lock_guard<std::mutex> lock(m_taskMutex);
	for (size_t i = 0; i< m_tasks.size(); ++i) {
		rynx::scheduler::task& task = m_tasks[i];
		if (task.is_for_each()) {
			if (!task.m_for_each->work_available()) {
				if (task.m_for_each->all_work_completed())
				{
					task.barriers().on_complete();

					m_tasks[i] = std::move(m_tasks.back());
					m_tasks.pop_back();
					--i;
				}
			}
		}
	}
}

void rynx::scheduler::context::schedule_task(task task) {
	std::lock_guard<std::mutex> lock(m_taskMutex);
	m_tasks.emplace_back(std::move(task));
}

void rynx::scheduler::context::dump() {
	std::lock_guard<std::mutex> lock(m_taskMutex);
	std::cout << m_tasks.size() << " tasks remaining" << std::endl;
	for (auto&& task : m_tasks) {
		(void)task;
		std::cout << task.name() << " barriers: " << task.barriers().can_start() << ", resources: " << resourcesAvailableFor(task) << std::endl;
		// logmsg("%s barriers: %d, resources: %d", task.name().c_str(), task.barriers().can_start(), resourcesAvailableFor(task));
	}
}
