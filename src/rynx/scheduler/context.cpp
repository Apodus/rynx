
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <iostream>

rynx::scheduler::task rynx::scheduler::context::findWork() {
	rynx_profile("Profiler", "Find work self");
	
#if PARALLEL_QUEUE_TASKS

	bool need_to_recheck_tasks = false;

	auto try_get_free_task = [this, &need_to_recheck_tasks]() -> rynx::scheduler::task
	{
		task t;
		if (m_tasks_parallel_for.deque(t)) {
			if (t.is_for_each()) {
				if (!t.m_for_each->work_available() & t.m_for_each->all_work_completed()) {
					// TODO: Check if any blocker barrier actually reaches zero OR any resource counter goes to zero.
					//       only then recheck for tasks.
					if (t.barriers().blocks_other_tasks()) {
						t.barriers().on_complete();
					}
					need_to_recheck_tasks = true;
					--m_task_counter;
					t = task();

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

	if(m_taskMutex.try_lock())
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

	if (need_to_recheck_tasks) {
		return findWork();
	}

#else
	std::lock_guard<std::mutex> lock(m_taskMutex);

	for (size_t i = 0; i < m_tasks.size(); ++i) {
		auto& task = m_tasks[i];
		// logmsg("looking for work: %s, barriers: %d, resources: %d", task.name().c_str(), task.barriers().can_start(), resourcesAvailableFor(task));

		if (task.barriers().can_start() & resourcesAvailableFor(task)) {
			rynx::scheduler::task t(std::move(task));
			m_tasks[i] = std::move(m_tasks.back());
			m_tasks.pop_back();
			reserve_resources(t.resources());
			return t;
		}
	}

	while (!m_tasks_parallel_for.empty()) {
		// if random task mode is enabled, set task index = random.
		// otherwise, set task_index = 0
		uint64_t task_index = m_random(m_tasks_parallel_for.size()) * (m_currentParallelForTaskStrategy == ParallelForTaskAssignmentStrategy::RandomTaskForEachWorkers);
		task& task = m_tasks_parallel_for[task_index];
		rynx::scheduler::task copy = task;
		task.completion_blocked_by(copy);
		return copy;
	}
#endif

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

void rynx::scheduler::context::erase_completed_parallel_for_tasks() {
#if PARALLEL_QUEUE_TASKS
#else
	rynx_profile("Profiler", "Erase completed parallel for tasks");
	std::lock_guard<std::mutex> lock(m_taskMutex);
	for (size_t i = 0; i < m_tasks_parallel_for.size(); ++i) {
		rynx::scheduler::task& task = m_tasks_parallel_for[i];
		if (!task.m_for_each->work_available() & task.m_for_each->all_work_completed()) {
			task.barriers().on_complete();
			m_tasks_parallel_for[i] = std::move(m_tasks_parallel_for.back());
			m_tasks_parallel_for.pop_back();
			--i;
			--m_task_counter;
		}
	}
#endif
}

void rynx::scheduler::context::schedule_task(task task) {
	++m_task_counter;
	++m_tasks_per_frame;
#if PARALLEL_QUEUE_TASKS
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
#else
	std::lock_guard<std::mutex> lock(m_taskMutex);
	if (task.is_for_each())
		m_tasks_parallel_for.emplace_back(std::move(task));
	else
		m_tasks.emplace_back(std::move(task));
#endif
}

void rynx::scheduler::context::dump() {
#if false && PARALLEL_QUEUE_TASKS
	// TODO :(
#else
	std::lock_guard<std::mutex> lock(m_taskMutex);
	std::cout << m_tasks.size() << " tasks remaining" << std::endl;
	for (auto&& task : m_tasks) {
		(void)task;
		std::cout << " barriers: " << task.barriers().can_start() << ", resources: " << resourcesAvailableFor(task) << " -- " << task.name() << std::endl;
		task.barriers().dump();
		// logmsg("%s barriers: %d, resources: %d", task.name().c_str(), task.barriers().can_start(), resourcesAvailableFor(task));
	}
#endif
}
