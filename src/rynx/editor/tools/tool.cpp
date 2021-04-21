
#include <rynx/editor/tools/tool.hpp>
#include <algorithm>

void rynx::editor::itool::source_data(std::function<void* ()> func) {
	m_get_data = std::move(func);
}

void* rynx::editor::itool::address_of_operand() {
	if (m_get_data)
		return m_get_data();
	return nullptr;
}

std::vector<std::string> rynx::editor::itool::get_editor_actions_list() {
	std::vector<std::string> result;
	for (auto&& entry : m_actions)
		result.push_back(entry.first);
	std::ranges::sort(result);
	return result;
}

bool rynx::editor::itool::editor_action_execute(std::string actionName, rynx::scheduler::context* ctx) {
	auto& action = m_actions.find(actionName)->second;
	action.m_action(ctx);
	return action.activate_tool;
}

rynx::id rynx::editor::itool::selected_id() const {
	if (m_editor_state->m_selected_ids.size() == 1) {
		return m_editor_state->m_selected_ids.front();
	}
	return rynx::id{ 0 };
}

void rynx::editor::itool::define_action(std::string name, std::function<void(rynx::scheduler::context*)> op) {
	m_actions.emplace(name, action{ true, name, std::move(op) });
}

void rynx::editor::itool::define_action_no_tool_activate(std::string name, std::function<void(rynx::scheduler::context*)> op) {
	m_actions.emplace(name, action{ false, name, std::move(op) });
}