
#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/scheduler/barrier.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/worker_thread.hpp>

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

void rynx::scheduler::task_scheduler::worker_activated() {
	++m_active_workers;
}

void rynx::scheduler::task_scheduler::worker_deactivated() {
	--m_active_workers;
}

void rynx::scheduler::task_scheduler::wake_up_sleeping_workers() {
	rynx_profile("Profiler", "Wake up sleeping workers");
	for (int i = 0; i < numThreads; ++i) {
		if (m_threads[i]->wake_up()) {
			return; // wake up one is always enough. if they find work, they will wake up the next worker.
		}
	}
}

rynx::scheduler::task_scheduler::task_scheduler() : m_threads({ nullptr }), m_deadlock_detector(this) {
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
rynx::observer_ptr<rynx::scheduler::context> rynx::scheduler::task_scheduler::make_context() {
	scheduler::context::context_id id = ++m_contextIdGen;
	rynx_assert(m_contexts.find(id) == m_contexts.end(), "creating a context but a context by this name exists already");
	auto context_ptr = std::make_unique<scheduler::context>(id, this);
	context_ptr->set_resource(context_ptr.get());
	auto context_observer = rynx::as_observer(context_ptr);
	m_contexts.emplace(id, std::move(context_ptr));
	return context_observer;
}

bool rynx::scheduler::task_scheduler::checkComplete() {
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
			return true;
		}
	}
	return false;
}


void rynx::scheduler::task_scheduler::dump() {
	rynx::this_thread::rynx_thread_raii poser_thread;
	for (auto& ctx : m_contexts) {
		ctx.second->dump();
	}

	for (auto& worker : m_threads) {
		std::cerr << "worker: " << worker->getTask().name() << std::endl;
	}
}