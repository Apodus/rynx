
#include <rynx/application/logic.hpp>
#include <rynx/scheduler/barrier.hpp>

rynx::application::logic::iruleset::iruleset() {
	m_barrier = std::make_unique<rynx::scheduler::barrier>("SystemBarrier");
}

void rynx::application::logic::iruleset::process(rynx::scheduler::context& context) {
	rynx::scheduler::scoped_barrier_after systemBarrier(context, *m_barrier);
	rynx::scheduler::scoped_barrier_before systemBarrier_dependencies(context);
	for (auto&& bar : m_dependOn) {
		systemBarrier_dependencies.addDependency(*bar);
	}

	onFrameProcess(context);
}

rynx::scheduler::barrier rynx::application::logic::iruleset::barrier() const { return *m_barrier; }

void rynx::application::logic::iruleset::requiredFor(iruleset& other) {
	other.m_dependOn.emplace_back(std::make_unique<rynx::scheduler::barrier>(*m_barrier));
}