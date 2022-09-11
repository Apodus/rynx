#include <rynx/editor/tools/subscene_tool.hpp>

rynx::editor::tools::subscene_tool::subscene_tool(rynx::scheduler::context& ctx) {

}

void rynx::editor::tools::subscene_tool::update(rynx::scheduler::context& ctx) {

}

void rynx::editor::tools::subscene_tool::verify(rynx::scheduler::context&, error_emitter&) {}

void rynx::editor::tools::subscene_tool::on_tool_selected() {
}

void rynx::editor::tools::subscene_tool::on_tool_unselected() {
}

bool rynx::editor::tools::subscene_tool::operates_on(const rynx::string&) {
	return false;
}

rynx::string rynx::editor::tools::subscene_tool::get_tool_name() {
	return "subscene editor";
}

rynx::string rynx::editor::tools::subscene_tool::get_button_texture() {
	return "subscene_tool";
}

void rynx::editor::tools::subscene_tool::on_entity_component_added(
	rynx::scheduler::context* ctx,
	rynx::string componentTypeName,
	rynx::ecs& ecs,
	rynx::id id)
{

}

void rynx::editor::tools::subscene_tool::on_entity_component_removed(
	rynx::scheduler::context* ctx,
	rynx::string componentTypeName,
	rynx::ecs& ecs,
	rynx::id id)
{

}

void rynx::editor::tools::subscene_tool::on_entity_component_value_changed(
	rynx::scheduler::context* ctx,
	rynx::type_index::type_id_t type_id,
	rynx::ecs& ecs,
	rynx::id id)
{
	if (type_id != rynx::type_index::id<rynx::components::transform::position>())
		return;

	// children need to translate to new position/orientation
	auto entity = ecs[id];
	if (entity.has<rynx::components::scene::children>()) {
		const auto& pos = entity.get<rynx::components::transform::position>();
		
		float sin_v = rynx::math::sin(pos.angle);
		float cos_v = rynx::math::cos(pos.angle);
		for (auto child : entity.get<rynx::components::scene::children>().entities) {
			const auto& child_local_pos = ecs[child].get<rynx::components::scene::local_position>().pos;
			auto& child_world_pos = ecs[child].get<rynx::components::transform::position>();
			auto rotated_local_pos = rynx::math::rotatedXY(child_local_pos.value, sin_v, cos_v);
			child_world_pos.value = rotated_local_pos + pos.value;
			child_world_pos.angle = pos.angle + child_local_pos.angle;
		}
	}
}
