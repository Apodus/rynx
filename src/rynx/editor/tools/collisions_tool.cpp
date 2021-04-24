
#include <rynx/editor/tools/collisions_tool.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/tech/collision_detection.hpp>

rynx::editor::tools::collisions_tool::collisions_tool(rynx::scheduler::context& ctx) : m_ecs(ctx.get_resource<rynx::ecs>()) {
	define_action_no_tool_activate("remove collisions", [this](rynx::scheduler::context* ctx) {
		auto& ecs = ctx->get_resource<rynx::ecs>();
		auto& collision_detection_system = ctx->get_resource<rynx::collision_detection>();
		auto id = selected_id();
		collision_detection_system.editor_api().remove_collision_from_entity(ecs, id);
		
		// update the entity component list
		this->m_editor_state->m_on_entity_selected(id);
	});

	define_action_no_tool_activate("set category dynamic", [this](rynx::scheduler::context* ctx) {
		auto& collision_detection_system = ctx->get_resource<rynx::collision_detection>();
		auto& ecs = ctx->get_resource<rynx::ecs>();
		auto id = selected_id();
		collision_detection_system.editor_api().set_collision_category_for_entity(
			ecs,
			id,
			rynx::collision_detection::category_id(0) // TODO: Magic number :(
		);
	});

	define_action_no_tool_activate("set category world", [this](rynx::scheduler::context* ctx) {
		auto& collision_detection_system = ctx->get_resource<rynx::collision_detection>();
		auto& ecs = ctx->get_resource<rynx::ecs>();
		auto id = selected_id();
		collision_detection_system.editor_api().set_collision_category_for_entity(
			ecs,
			id,
			rynx::collision_detection::category_id(1) // TODO: Magic number :(
		);
	});
}

void rynx::editor::tools::collisions_tool::update(rynx::scheduler::context& ctx) {

}
