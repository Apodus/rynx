
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/tech/profiling.hpp>

rynx::scheduler::task* rynx::scheduler::task_token::operator -> () { return &m_task; }
rynx::scheduler::task_token::~task_token() {
	m_task.m_context->schedule_task(std::move(m_task));
}


void rynx::scheduler::task::reserve_resources() const { m_context->reserve_resources(*m_resources); }

void rynx::scheduler::task::run() {
	rynx_profile("Scheduler", m_name);
	rynx_assert(m_op, "no op in task that is being run!");
	rynx_assert(m_barriers.can_start(), "task is being run while still blocked by barriers!");

	{
		// logmsg("start %s", m_name.c_str());
		rynx_profile("Scheduler", "work");
		m_op(this);
		// logmsg("end %s", m_name.c_str());
	}

	{
		rynx_profile("Scheduler", "barriers update");
		m_barriers.on_complete();
	}

	{
		rynx_profile("Scheduler", "release resources");
		m_resources.reset();
		m_resources_shared.reset();
	}
}


rynx::scheduler::task::task_resources::~task_resources() {
	m_context->release_resources(*this);
}


rynx::scheduler::task& rynx::scheduler::task::depends_on(task_token& other) {
	return depends_on(*other);
}

rynx::scheduler::task& rynx::scheduler::task::required_for(task_token& other) {
	return required_for(*other);
}

rynx::scheduler::task& rynx::scheduler::task::operator & (barrier bar) {
	extend_task([]() {})->depends_on(bar);
	return *this;
}

rynx::scheduler::task& rynx::scheduler::task::operator & (task_token& other) {
	completion_blocked_by(*other);
	return *this;
}

rynx::scheduler::task& rynx::scheduler::task::operator | (task_token& other) {
	other->depends_on(*this);
	return *other;
}

void rynx::scheduler::task::notify_work_available() const {
	m_context->m_scheduler->wake_up_sleeping_workers();
}