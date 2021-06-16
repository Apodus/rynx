
#include <rynx/system/typeid.hpp>
#include <rynx/editor/tools/collisions_tool.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/tech/collision_detection.hpp>

rynx::editor::tools::collisions_tool::collisions_tool(rynx::scheduler::context& ctx) : m_ecs(ctx.get_resource<rynx::ecs>()) {
	define_action_no_tool_activate(rynx::traits::type_name<rynx::components::collisions>(), "remove collisions", [this](rynx::scheduler::context* ctx) {
		auto& ecs = ctx->get_resource<rynx::ecs>();
		auto& collision_detection_system = ctx->get_resource<rynx::collision_detection>();
		auto id = selected_id();
		collision_detection_system.editor_api().remove_collision_from_entity(ecs, id);
		
		// update the entity component list
		this->m_editor_state->m_on_entity_selected(id);
	});

	define_action_no_tool_activate(rynx::traits::type_name<rynx::components::collisions>(), "set category dynamic", [this](rynx::scheduler::context* ctx) {
		auto& collision_detection_system = ctx->get_resource<rynx::collision_detection>();
		auto& ecs = ctx->get_resource<rynx::ecs>();
		auto id = selected_id();
		collision_detection_system.editor_api().set_collision_category_for_entity(
			ecs,
			id,
			rynx::collision_detection::category_id(0) // TODO: Magic number :(
		);
	});

	define_action_no_tool_activate(rynx::traits::type_name<rynx::components::collisions>(), "set category world", [this](rynx::scheduler::context* ctx) {
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

void rynx::editor::tools::collisions_tool::on_entity_component_removed(
	rynx::scheduler::context* ctx,
	std::string componentTypeName,
	rynx::ecs& ecs,
	rynx::id id)
{
	if (componentTypeName == rynx::traits::type_name<rynx::components::boundary>()) {
		logmsg("noticed removal of boundary");
		if (ecs[id].has<rynx::components::collisions>()) {
			logmsg("updating collision kind");
			auto& detection = ctx->get_resource<rynx::collision_detection>();
			detection.editor_api().update_collider_kind_for_entity(ecs, id);
		}
	}
}

namespace {
	struct verify_has_component {
		template<typename T>
		void check(rynx::editor::itool::error_emitter& emitter, rynx::ecs& ecs, rynx::id entity) {
			if (!ecs[entity].has<T>()) {
				emitter.component_missing<T>(entity, "missing from collision");
			}
		}
	};
}

void rynx::editor::tools::collisions_tool::verify(rynx::scheduler::context& ctx, error_emitter& emitter) {
	auto& ecs = ctx.get_resource<rynx::ecs>();
	ecs.query().in<rynx::components::collisions>().for_each([&emitter, &ecs](rynx::id entity) {
		verify_has_component().check<rynx::components::physical_body>(emitter, ecs, entity);
		verify_has_component().check<rynx::components::motion>(emitter, ecs, entity);
	});
}

void rynx::editor::tools::collisions_tool::update(rynx::scheduler::context&) {
}
