
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <iostream>

rynx::scheduler::task rynx::scheduler::context::findWork() {
	rynx_profile("Profiler", "Find work self");
	
	bool task_was_completed = false;
	auto try_get_free_task = [this, &task_was_completed]() -> rynx::scheduler::task
	{
		task t;
		if (m_tasks_parallel_for.deque(t)) {
			if (t.is_for_each()) {
				if (t.for_each_no_work_available()) {					
					if (t.for_each_all_work_completed()) {
						task_was_completed = true;
						if (t.m_enable_logging) {
							logmsg("end parfor %s", t.name().c_str());
						}

						if (t.barriers().blocks_other_tasks()) {
							t.barriers().on_complete();
						}
						--m_task_counter;
						t = task();
					}
					else {
						m_tasks_parallel_for.enque(std::move(t));
					}
				}
				else {
					rynx::scheduler::task copy = t;
					t.completion_blocked_by(copy);
					m_tasks_parallel_for.enque(std::move(t));
					return copy;
				}
			}
			else {
				return t;
			}
		}
		return {};
	};

	auto try_get_protected_task = [this]() -> rynx::scheduler::task
	{
		for (size_t i = 0; i < m_tasks.size(); ++i) {
			auto& task = m_tasks[i];
			if (task.barriers().can_start() & resourcesAvailableFor(task)) {
				rynx::scheduler::task t(std::move(task));
				m_tasks[i] = std::move(m_tasks.back());
				m_tasks.pop_back();
				reserve_resources(t.resources());
				return t;
			}
		}
		return {};
	};


	do {
		task_was_completed = false;
		if (m_taskMutex.try_lock())
		{
			{
				task t = try_get_protected_task();
				m_taskMutex.unlock();
				if (t) {
					return t;
				}
			}

			{
				task t = try_get_free_task();
				if (t) {
					return t;
				}
			}
		}
		else
		{
			{
				task t = try_get_free_task();
				if (t) {
					return t;
				}
			}

			{
				m_taskMutex.lock();
				task t = try_get_protected_task();
				m_taskMutex.unlock();
				if (t) {
					return t;
				}
			}
		}
	} while (!m_tasks_parallel_for.empty() | task_was_completed);

	return {};
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

void rynx::scheduler::context::schedule_task(task task) {
	++m_task_counter;
	++m_tasks_per_frame;

	if (task.is_for_each() ||
		(task.resources().empty() && task.barriers().can_start())
		)
	{
		m_tasks_parallel_for.enque(std::move(task));
	}
	else
	{
		std::scoped_lock lock(m_taskMutex);
		m_tasks.emplace_back(std::move(task));
	}
}

void rynx::scheduler::context::dump() {
	std::lock_guard<std::mutex> lock(m_taskMutex);
	std::cout << m_tasks.size() << " tasks remaining" << std::endl;
	for (auto&& task : m_tasks) {
		(void)task;
		std::cout << " barriers: " << task.barriers().can_start() << ", resources: " << resourcesAvailableFor(task) << " -- " << task.name() << std::endl;
		task.barriers().dump();
		// logmsg("%s barriers: %d, resources: %d", task.name().c_str(), task.barriers().can_start(), resourcesAvailableFor(task));
	}

	while (!m_tasks_parallel_for.empty()) {
		task t;
		m_tasks_parallel_for.deque(t);
		if (t.is_for_each()) {
			std::cout << "for each (" << t.name() << ")\n\tno work available: " << t.for_each_no_work_available() << "\n\tall work done: " << t.for_each_all_work_completed() << "\n";
		}
		else {
			std::cout << "free task: " << t.name() << "\n";
		}
	}
}
