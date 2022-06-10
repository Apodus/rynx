
#include <rynx/application/logic.hpp>
#include <rynx/scheduler/barrier.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/tech/serialization.hpp>

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

rynx::serialization::vector_writer rynx::application::logic::iruleset::apply_to_all::serialize(rynx::scheduler::context& context) {
	return m_host->m_parent->serialize(context);
}

void rynx::application::logic::iruleset::apply_to_all::deserialize(rynx::scheduler::context& context, rynx::serialization::vector_reader& reader) {
	return m_host->m_parent->deserialize(context, reader);
}

void rynx::application::logic::iruleset::apply_to_all::clear(rynx::scheduler::context& context) {
	m_host->m_parent->clear(context);
}

rynx::application::logic& rynx::application::logic::add_ruleset(std::unique_ptr<iruleset> ruleset) {
	m_rules.emplace_back(std::move(ruleset));
	m_rules.back()->m_parent = this;
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

void rynx::application::logic::clear(rynx::scheduler::context& context) {
	for (auto& ruleset : m_rules) {
		ruleset->clear(context);
	}
}

void rynx::application::logic::serialize(rynx::scheduler::context& context, rynx::serialization::vector_writer& out) {
	rynx::unordered_map<std::string, std::vector<char>> serializations;
	for (auto&& ruleset : m_rules) {
		rynx::serialization::vector_writer ruleset_out;
		ruleset->serialize(context, ruleset_out);
		if (!ruleset_out.data().empty()) {
			serializations.emplace(ruleset->m_ruleset_unique_name, std::move(ruleset_out.data()));
		}
	}
	rynx::serialize(serializations, out);
}

rynx::serialization::vector_writer rynx::application::logic::serialize(rynx::scheduler::context& context) {
	rynx::serialization::vector_writer out;
	serialize(context, out);
	return out;
}

void rynx::application::logic::deserialize(rynx::scheduler::context& context, rynx::serialization::vector_reader& in) {
	rynx::unordered_map<std::string, std::vector<char>> serializations;
	rynx::deserialize(serializations, in);
	for (auto&& ruleset : m_rules) {
		auto it = serializations.find(ruleset->m_ruleset_unique_name);
		if (it != serializations.end()) {
			rynx::serialization::vector_reader ruleset_in(it->second);
			ruleset->deserialize(context, ruleset_in);
		}
	}
}