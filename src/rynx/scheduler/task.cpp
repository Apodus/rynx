
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/context.hpp>

#ifdef WILDSHADE_PROFILING
#include <Core/Profiling.h>
#endif

void rynx::scheduler::task::reserve_resources() const { m_context->reserve_resources(*this); }

void rynx::scheduler::task::run() {
	rynx_profile(Game, m_name.c_str());
	rynx_assert(m_op, "no op in task that is being run!");
	rynx_assert(m_barriers.canStart(), "task is being run while still blocked by barriers!");

	{
		rynx_profile(Game, "op");
		m_op(this);
	}

	{
		rynx_profile(Game, "barriers update");
		m_barriers.onComplete();
	}

	{
		rynx_profile(Game, "release resources");
		m_context->release_resources(*this);
	}
}
