
#include <rynx/application/logic.hpp>
#include <rynx/scheduler/barrier.hpp>
#include <rynx/scheduler/context.hpp>

rynx::application::logic::iruleset::iruleset() {
	m_barrier = std::make_unique<rynx::scheduler::barrier>("SystemBarrier");
}

void rynx::application::logic::iruleset::process(rynx::scheduler::context& context, float dt) {
	rynx::scheduler::scoped_barrier_after systemBarrier(context, *m_barrier);
	rynx::scheduler::scoped_barrier_before systemBarrier_dependencies(context);
	for (auto&& bar : m_dependOn) {
		systemBarrier_dependencies.addDependency(*bar);
	}

	onFrameProcess(context, dt);
}

rynx::scheduler::barrier rynx::application::logic::iruleset::barrier() const { return *m_barrier; }

void rynx::application::logic::iruleset::required_for(iruleset& other) {
	other.m_dependOn.emplace_back(std::make_unique<rynx::scheduler::barrier>(*m_barrier));
}


rynx::application::logic& rynx::application::logic::add_ruleset(std::unique_ptr<iruleset> ruleset) {
	m_rules.emplace_back(std::move(ruleset));
	return *this;
}

void rynx::application::logic::generate_tasks(rynx::scheduler::context& context, float dt) {
	for (auto& ruleset : m_rules) {
		if (ruleset->state_id().is_enabled()) {
			ruleset->process(context, dt);
		}
	}
}

void rynx::application::logic::entities_erased(rynx::scheduler::context& context, const std::vector<rynx::ecs::id>& ids) {
	for (auto& ruleset : m_rules) {
		ruleset->on_entities_erased(context, ids);
	}
}

void rynx::application::logic::clear() {
	for (auto& ruleset : m_rules) {
		ruleset->clear();
	}
}