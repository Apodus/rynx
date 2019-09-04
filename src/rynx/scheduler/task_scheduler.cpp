
#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/scheduler/barrier.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/worker_thread.hpp>

rynx::scheduler::task* rynx::scheduler::task_token::operator -> () { return &m_context->m_tasks.find(m_taskId)->second; }

rynx::scheduler::task rynx::scheduler::context::findWork() {
	std::lock_guard<std::mutex> lock(m_taskMutex);
	for (auto it = m_tasks.begin(); it != m_tasks.end(); ++it) {
		auto& task = it->second;
		if (task.barriers().canStart() && resourcesAvailableFor(task)) {
			rynx::scheduler::task t(std::move(it->second));
			m_tasks.erase(it);
			return t;
		}
	}
	return task();
}



int rynx::scheduler::task_scheduler::getFreeThreadIndex() {
	for (int i = 0; i < numThreads; ++i) {
		if (m_threads[i]->isDone()) {
			return i;
		}
	}
	return -1;
}

void rynx::scheduler::task_scheduler::startTask(int threadIndex, rynx::scheduler::task& task) {
	task.reserve_resources();
	m_threads[threadIndex]->setTask(std::move(task));
	m_threads[threadIndex]->start();
}

bool rynx::scheduler::task_scheduler::findWorkForThreadIndex(int threadIndex) {
	// TODO: We should have separate mutexes for each thread index. Locking all is bad sense.
	std::lock_guard<std::mutex> lock(m_worker_mutex);
	if (m_threads[threadIndex]->isDone()) {
		for (auto& context : m_contexts) {
			rynx::scheduler::task t = context.second->findWork();
			if (t) {
				startTask(threadIndex, t);
				return true;
			}
		}
	}
	return false;
}

void rynx::scheduler::task_scheduler::findWorkForAllThreads() {
	for (;;) {
		int freeThreadIndex = getFreeThreadIndex();
		if (freeThreadIndex >= 0) {
			if (!findWorkForThreadIndex(freeThreadIndex)) {
				return;
			}
		}
		else {
			// logmsg("task execution was requested but no task threads are free :(");
			return;
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
	return m_contexts.emplace(id, std::make_unique<scheduler::context>(id)).first->second.get();
}


void rynx::scheduler::task_scheduler::checkComplete() {
	bool contextsHaveNoQueuedTasks = true;
	for (auto& context : m_contexts) {
		contextsHaveNoQueuedTasks &= context.second->isFinished();
	}

	if (contextsHaveNoQueuedTasks) {
		bool allWorkersAreFinished = true;
		for (auto&& worker : m_threads) {
			allWorkersAreFinished &= worker->isDone();
		}

		if (allWorkersAreFinished) {
			if (m_frameComplete.exchange(1) == 0) {
				m_waitForComplete.signal();
			}
		}
	}
}