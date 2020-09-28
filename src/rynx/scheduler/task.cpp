
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/tech/profiling.hpp>

rynx::scheduler::task* rynx::scheduler::task_token::operator -> () { return m_pTask.get(); }
rynx::scheduler::task_token::~task_token() {
	if(m_pTask)
		m_pTask->m_context->schedule_task(std::move(*m_pTask));
}

rynx::scheduler::task_token::task_token(task&& task) {
	m_pTask = std::make_unique<rynx::scheduler::task>(std::move(task));
}

rynx::scheduler::task_token& rynx::scheduler::task_token::depends_on(task& other) {
	barrier bar(other.m_name);
	m_pTask->barriers().depends_on(bar);
	other.barriers().required_for(bar);
	return *this;
}

rynx::scheduler::task_token& rynx::scheduler::task_token::required_for(task& other) {
	barrier bar(m_pTask->m_name);
	other.barriers().depends_on(bar);
	m_pTask->barriers().required_for(bar);
	return *this;
}

rynx::scheduler::task_token& rynx::scheduler::task_token::depends_on(barrier bar) {
	m_pTask->barriers().depends_on(bar);
	return *this;
}

rynx::scheduler::task_token& rynx::scheduler::task_token::required_for(barrier bar) {
	m_pTask->barriers().required_for(bar);
	return *this;
}

rynx::scheduler::task& rynx::scheduler::task_token::operator * () {
	return *m_pTask;
}

rynx::scheduler::task_token& rynx::scheduler::task_token::operator | (task_token& other) {
	other.depends_on(*this);
	return other;
}

void rynx::scheduler::task::run() {
	rynx_profile("Scheduler", m_name);
	rynx_assert(static_cast<bool>(m_op), "no op in task that is being run!");
	rynx_assert(m_barriers->can_start(), "task is being run while still blocked by barriers!");

	{
		if (m_enable_logging && !is_for_each()) {
			logmsg("start %s", m_name.c_str());
		}
		
		rynx_profile("Scheduler", "work");
		m_op(this);
		
		if (m_enable_logging && !is_for_each()) {
			logmsg("end %s", m_name.c_str());
		}
	}

	{
		if (!is_for_each()) {
			m_context->task_finished();
		}
	}

	{
		rynx_profile("Scheduler", "barriers update");
		m_barriers->on_complete();
	}

	{
		rynx_profile("Scheduler", "release resources");
		m_resources.reset();
		m_resources_shared.clear();
	}
}

rynx::scheduler::task::task_resources::~task_resources() {
	if (!empty()) {
		m_context->release_resources(*this);
	}
}

rynx::scheduler::task& rynx::scheduler::task::operator & (task_token& other) {
	completion_blocked_by(*other);
	return *this;
}

rynx::scheduler::task& rynx::scheduler::task::operator | (task_token& other) {
	other.depends_on(*this);
	return *other;
}

rynx::scheduler::task::task(const task& other) {
	m_name = other.m_name;
	m_op = other.m_op;
	m_barriers = std::make_shared<operation_barriers>(*other.m_barriers);

	m_resources = other.m_resources;
	m_resources_shared = other.m_resources_shared;
	m_for_each = other.m_for_each;

	m_context = other.m_context;
	m_enable_logging = other.m_enable_logging;
}

void rynx::scheduler::task::notify_work_available() const {
	m_context->m_scheduler->wake_up_sleeping_workers();
}