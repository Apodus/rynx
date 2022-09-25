
#include <rynx/editor/tools/joint_tool.hpp>
#include <rynx/application/visualisation/debug_visualisation.hpp>
#include <rynx/menu/Component.hpp>
#include <rynx/menu/Div.hpp>
#include <rynx/menu/Button.hpp>

rynx::editor::tools::joint_tool::joint_tool([[maybe_unused]] rynx::scheduler::context& ctx) {
	define_action_no_tool_activate(
		rynx::type_index::id<rynx::components::phys::joint>(),
		"recompute length",
		[this](rynx::scheduler::context* ctx)
		{
			auto& ecs = ctx->get_resource<rynx::ecs>();
			auto* joint = reinterpret_cast<rynx::components::phys::joint*>(this->address_of_operand());
			if(ecs.exists(joint->a.id) & ecs.exists(joint->b.id))
				joint->length = rynx::components::phys::compute_current_joint_length(*joint, ecs);
		}
	);
}

bool rynx::editor::tools::joint_tool::try_generate_menu(
	rynx::reflection::field field_type,
	rynx::editor::component_recursion_info_t info,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	rynx::reflection::type type = info.reflections->get(field_type);
	
	if (type.m_type_name == rynx::traits::type_name<rynx::components::phys::joint>()) {
		for (auto&& field : type.m_fields) {
			auto field_type_ = info.reflections->get(field);
			info.gen_type_editor(field_type_, info, reflection_stack);
		}
		return true;
	}
	
	using joint_connector_t = rynx::components::phys::joint::connector_type;
	if (type.m_type_name == rynx::traits::type_name<joint_connector_t>())
	{
		auto field_div = rynx::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.03f, 0.0f));
		field_div->velocity_position(200.0f); // TODO

		auto jointConnectionButton = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(1.0f, 1.0f, 0.0f));
		jointConnectionButton->velocity_position(200.0f); // TODO
		jointConnectionButton->on_click([field_type, type, info, self = jointConnectionButton.get()]() {
			char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
			auto* value = reinterpret_cast<joint_connector_t*>(data + info.cumulative_offset);
			if (*value == joint_connector_t::Rod) {
				self->text().text(field_type.m_field_name + ": RubberBand");
				*value = joint_connector_t::RubberBand;
			}
			else if (*value == joint_connector_t::RubberBand) {
				self->text().text(field_type.m_field_name + ": Spring");
				*value = joint_connector_t::Spring;
			}
			else if (*value == joint_connector_t::Spring) {
				self->text().text(field_type.m_field_name + ": Rod");
				*value = joint_connector_t::Rod;
			}
		});

		char* data = reinterpret_cast<char*>((*info.ecs)[info.entity_id].get(info.component_type_id));
		auto* value = reinterpret_cast<joint_connector_t*>(data + info.cumulative_offset);
		if (*value == joint_connector_t::Rod) {
			jointConnectionButton->text().text(field_type.m_field_name + ": Rod");
		}
		else if (*value == joint_connector_t::RubberBand) {
			jointConnectionButton->text().text(field_type.m_field_name + ": Rubberband");
		}
		else if (*value == joint_connector_t::Spring) {
			jointConnectionButton->text().text(field_type.m_field_name + ": Spring");
		}

		field_div->align()
			.target(info.component_sheet->last_child())
			.bottom_outside()
			.left_inside();

		field_div->addChild(jointConnectionButton);
		info.component_sheet->addChild(field_div);
		return true;
	}
	return false;
}

void rynx::editor::tools::joint_tool::on_entity_component_value_changed(
	rynx::scheduler::context* /* ctx */,
	rynx::type_index::type_id_t type_id,
	rynx::ecs& ecs,
	rynx::id id)
{
	bool positionChanged = type_id == rynx::type_index::id<rynx::components::transform::position>();
	bool jointChanged = type_id == rynx::type_index::id<rynx::components::phys::joint>();

	if (positionChanged) {
		ecs.query().for_each([this, id, &ecs](rynx::components::phys::joint& j) {
			if (j.a.id == id || j.b.id == id) {
				if (ecs.exists(j.a.id) & ecs.exists(j.b.id)) {
					j.length = rynx::components::phys::compute_current_joint_length(j, ecs);
				}
			}
		});
	}

	if (jointChanged) {
		auto& j = ecs[id].get<rynx::components::phys::joint>();
		j.length = rynx::components::phys::compute_current_joint_length(j, ecs);
	}
}

void rynx::editor::tools::joint_tool::update_nofocus(rynx::scheduler::context& ctx) {
	auto& ecs = ctx.get_resource<rynx::ecs>();
	auto& dbug = ctx.get_resource<rynx::application::DebugVisualization>();

	ecs.query().for_each([&dbug, &ecs](const rynx::components::phys::joint& joint, rynx::components::transform::position p) {
		if (ecs.exists(joint.a.id)) {
			auto pos_a = ecs[joint.a.id].get<rynx::components::transform::position>();
			auto offset_a = rynx::math::rotatedXY(joint.a.pos, pos_a.angle);
			dbug.addDebugLine(pos_a.value + offset_a, p.value, { 0.5f, 1.0f, 0.2f, 1.0f });
		}
		if (ecs.exists(joint.b.id)) {
			auto pos_b = ecs[joint.b.id].get<rynx::components::transform::position>();
			auto offset_b = rynx::math::rotatedXY(joint.b.pos, pos_b.angle);
			dbug.addDebugLine(pos_b.value + offset_b, p.value, { 0.5f, 1.0f, 0.2f, 1.0f });
		}
	});

	auto id = selected_id();
	if (ecs.exists(id)) {
		auto joint_entity_pos = ecs[id].get<rynx::components::transform::position>();
		auto* joint = ecs[id].try_get<rynx::components::phys::joint>();
		if (joint) {
			if (ecs.exists(joint->a.id)) {
				auto pos_a = ecs[joint->a.id].get<rynx::components::transform::position>();
				auto offset_a = rynx::math::rotatedXY(joint->a.pos, pos_a.angle);
				dbug.addDebugLine(pos_a.value + offset_a, joint_entity_pos.value, { 0.5f, 1.0f, 0.2f, 1.0f });
			}
			if (ecs.exists(joint->b.id)) {
				auto pos_b = ecs[joint->b.id].get<rynx::components::transform::position>();
				auto offset_b = rynx::math::rotatedXY(joint->b.pos, pos_b.angle);
				dbug.addDebugLine(pos_b.value + offset_b, joint_entity_pos.value, { 1.0f, 0.5f, 0.2f, 1.0f });
			}

			if (ecs.exists(joint->a.id) & ecs.exists(joint->b.id)) {
				auto pos_a = ecs[joint->a.id].get<rynx::components::transform::position>();
				auto pos_b = ecs[joint->b.id].get<rynx::components::transform::position>();
				auto offset_a = rynx::math::rotatedXY(joint->a.pos, pos_a.angle);
				auto offset_b = rynx::math::rotatedXY(joint->b.pos, pos_b.angle);
				dbug.addDebugLine(pos_a.value + offset_a, pos_b.value + offset_b, { 0.2f, 1.0f, 1.0f, 1.0f });
			}
		}
	}
}

namespace {
	struct verify_has_component {
		template<typename T>
		void check(rynx::editor::itool::error_emitter& emitter, rynx::ecs& ecs, rynx::id entity) {
			if (!ecs[entity].has<T>()) {
				emitter.component_missing<T>(entity, "missing from joint target");
			}
		}
	};
}

void rynx::editor::tools::joint_tool::verify(rynx::scheduler::context& ctx, error_emitter& emitter) {
	auto& ecs = ctx.get_resource<rynx::ecs>();
	ecs.query().for_each(
		[this, &emitter, &ecs](
			rynx::id id,
			const rynx::components::phys::joint& joint)
		{
			if (!ecs.exists(joint.a.id) || !ecs.exists(joint.b.id)) {
				emitter.emit(id, "joint component not connected to entities");
			}
			else {
				auto is_valid_joint_target = [&](rynx::id entity) {
					verify_has_component().check<rynx::components::phys::body>(emitter, ecs, entity);
					verify_has_component().check<rynx::components::transform::motion>(emitter, ecs, entity);
					verify_has_component().check<rynx::components::transform::position>(emitter, ecs, entity);
				};
			
				is_valid_joint_target(joint.a.id);
				is_valid_joint_target(joint.b.id);
			}
		}
	);
}

void rynx::editor::tools::joint_tool::update(rynx::scheduler::context&) {
}
