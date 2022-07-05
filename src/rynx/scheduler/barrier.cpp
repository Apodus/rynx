
#include <rynx/scheduler/barrier.hpp>
#include <rynx/scheduler/context.hpp>

#include <iostream>

void rynx::scheduler::operation_barriers::dump() const {
	for (auto&& bar : m_requires) {
		std::cout << "\tbarrier name: '" << bar.name.c_str() << "', counter: " << bar.counter->load() << std::endl;
	}
}

rynx::scheduler::scoped_barrier_after::scoped_barrier_after(scheduler::context& context, barrier bar) : m_context(&context) {
	m_context->m_activeTaskBarriers.emplace_back(std::move(bar));
}

rynx::scheduler::scoped_barrier_after::~scoped_barrier_after() {
	m_context->m_activeTaskBarriers.pop_back();
}

rynx::scheduler::scoped_barrier_before::~scoped_barrier_before() {
	while (m_dependenciesAdded > 0) {
		m_context->m_activeTaskBarriers_Dependencies.pop_back();
		--m_dependenciesAdded;
	}
}

rynx::scheduler::scoped_barrier_before& rynx::scheduler::scoped_barrier_before::addDependency(rynx::scheduler::barrier bar) {
	m_context->m_activeTaskBarriers_Dependencies.emplace_back(std::move(bar));
	++m_dependenciesAdded;
	return *this;
}