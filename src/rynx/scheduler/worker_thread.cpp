
#include <rynx/scheduler/worker_thread.hpp>
#include <rynx/scheduler/task_scheduler.hpp>

#ifdef WILDSHADE_PROFILING
#include <Core/Profiling.h>
#endif

rynx::scheduler::task_thread::task_thread(task_scheduler* pTaskMaster, int myIndex) {
	m_scheduler = pTaskMaster;
	m_done = true;
	m_running = true;
	m_thread = std::thread([this, myIndex]() {
#ifdef WILDSHADE_PROFILING
		core::Thread::Block bs; // wildshade engine memory tracking or some such. is required.
#endif
		threadEntry(myIndex);
	});
}


void rynx::scheduler::task_thread::threadEntry(int myThreadIndex) {
	while (m_running) {
		if (!m_task)
			wait();

		if (m_task) {
			m_task.run();
			m_task.clear();

			// if we can't find work, then we are done and others are allowed to find work for us.
			m_done = true;
			m_scheduler->findWorkForThreadIndex(myThreadIndex);

			if (!m_done) {
				// if we did find work, we should also check if we can find work for our friends.
				m_scheduler->findWorkForAllThreads();
			}
			else {
				m_scheduler->checkComplete();
			}
		}
	}
	logmsg("task thread exit");
}