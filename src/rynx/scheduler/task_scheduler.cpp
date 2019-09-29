
#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/scheduler/barrier.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/worker_thread.hpp>

rynx::scheduler::task* rynx::scheduler::task_token::operator -> () { return &m_context->m_tasks.find(m_taskId)->second; }

// TODO: it would probably be better to just find work, and return the task. don't mix threads and worker state to this function. let them do that internally.
bool rynx::scheduler::task_scheduler::find_work_for_thread_index(int threadIndex) {
	rynx_profile("Profiler", "Find work");
	for (auto& context : m_contexts) {
		rynx::scheduler::task t = context.second->findWork();
		if (t) {
			m_threads[threadIndex]->set_task(std::move(t));
			return true;
		}
	}
	return false;
}

void rynx::scheduler::task_scheduler::wake_up_sleeping_workers() {
	rynx_profile("Profiler", "Wake up sleeping workers");
	for (int i = 0; i < numThreads; ++i) {
		if (m_threads[i]->is_sleeping()) {
			m_threads[i]->wake_up();
		}
	}
}

rynx::scheduler::task_scheduler::task_scheduler() : m_threads({ nullptr }) {
	for (int i = 0; i < numThreads; ++i) {
		m_threads[i] = new rynx::scheduler::task_thread(this, i);
	}
}

rynx::scheduler::task_scheduler::~task_scheduler() {
	for (int i = 0; i < numThreads; ++i) {
		m_threads[i]->shutdown();
		delete m_threads[i];
	}
}

// TODO: this should probably return some wrapper object, instead of a pointer. now user might think that he needs to delete it.
rynx::scheduler::context* rynx::scheduler::task_scheduler::make_context() {
	scheduler::context::context_id id = ++m_contextIdGen;
	rynx_assert(m_contexts.find(id) == m_contexts.end(), "creating a context but a context by this name exists already");
	return m_contexts.emplace(id, std::make_unique<scheduler::context>(id, this)).first->second.get();
}

void rynx::scheduler::task_scheduler::checkComplete() {
	bool contextsHaveNoQueuedTasks = true;
	for (auto& context : m_contexts) {
		contextsHaveNoQueuedTasks &= context.second->isFinished();
	}

	if (contextsHaveNoQueuedTasks) {
		bool allWorkersAreFinished = true;
		for (auto&& worker : m_threads) {
			allWorkersAreFinished &= worker->is_sleeping();
		}

		if (allWorkersAreFinished) {
			if (m_frameComplete.exchange(1) == 0) {
				m_waitForComplete.signal();
			}
		}
	}
}
