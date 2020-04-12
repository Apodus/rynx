
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <iostream>

rynx::scheduler::task rynx::scheduler::context::findWork() {
	rynx_profile("Profiler", "Find work self");
	std::lock_guard<std::mutex> lock(m_taskMutex);
	
	for (size_t i = 0; i < m_tasks.size(); ++i) {
		auto& task = m_tasks[i];
		// logmsg("looking for work: %s, barriers: %d, resources: %d", task.name().c_str(), task.barriers().can_start(), resourcesAvailableFor(task));

		if (task.barriers().can_start() && resourcesAvailableFor(task)) {
			rynx::scheduler::task t(std::move(task));
			m_tasks[i] = std::move(m_tasks.back());
			m_tasks.pop_back();
			t.reserve_resources();
			return t;
		}
	}
	
	while (!m_tasks_parallel_for.empty()) {
		// if random task mode is enabled, set task index = random.
		// otherwise, set task_index = 0
		uint64_t task_index = m_random(m_tasks_parallel_for.size()) * (m_currentParallelForTaskStrategy == ParallelForTaskAssignmentStrategy::RandomTaskForEachWorkers);
		
		task& task = m_tasks_parallel_for[task_index];

		if (task.for_each_no_work_available()) {
			if (task.for_each_all_work_completed()) {
				task.barriers().on_complete();
				m_tasks_parallel_for[task_index] = std::move(m_tasks_parallel_for.back());
				m_tasks_parallel_for.pop_back();
				continue;
			}
		}

		rynx::scheduler::task copy = task;
		task.completion_blocked_by(copy);
		return copy;
	}
	
	return task();
}

void rynx::scheduler::context::set_parallel_for_task_assignment_strategy_as_random() {
	m_currentParallelForTaskStrategy = ParallelForTaskAssignmentStrategy::RandomTaskForEachWorkers;
}

void rynx::scheduler::context::set_parallel_for_task_assignment_strategy_as_first_available() {
	m_currentParallelForTaskStrategy = ParallelForTaskAssignmentStrategy::SameTaskForEachWorkers;
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
	for (size_t i = 0; i < m_tasks_parallel_for.size(); ++i) {
		rynx::scheduler::task& task = m_tasks_parallel_for[i];
		if (!task.m_for_each->work_available()) {
			if (task.m_for_each->all_work_completed())
			{
				task.barriers().on_complete();
				m_tasks_parallel_for[i] = std::move(m_tasks_parallel_for.back());
				m_tasks_parallel_for.pop_back();
				--i;
			}
		}
	}
}

void rynx::scheduler::context::schedule_task(task task) {
	std::lock_guard<std::mutex> lock(m_taskMutex);
	if (task.is_for_each())
		m_tasks_parallel_for.emplace_back(std::move(task));
	else
		m_tasks.emplace_back(std::move(task));
}

void rynx::scheduler::context::dump() {
	std::lock_guard<std::mutex> lock(m_taskMutex);
	std::cout << m_tasks.size() << " tasks remaining" << std::endl;
	for (auto&& task : m_tasks) {
		(void)task;
		std::cout << task.name() << " barriers: " << task.barriers().can_start() << ", resources: " << resourcesAvailableFor(task) << std::endl;
		task.barriers().dump();
		// logmsg("%s barriers: %d, resources: %d", task.name().c_str(), task.barriers().can_start(), resourcesAvailableFor(task));
	}
}
