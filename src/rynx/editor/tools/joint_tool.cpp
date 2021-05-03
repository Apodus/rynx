
#include <rynx/editor/tools/joint_tool.hpp>
#include <rynx/application/visualisation/debug_visualisation.hpp>
#include <rynx/menu/Component.hpp>
#include <rynx/menu/Div.hpp>
#include <rynx/menu/Button.hpp>

rynx::editor::tools::joint_tool::joint_tool(rynx::scheduler::context& ctx) {
	define_action_no_tool_activate(
		rynx::traits::type_name<rynx::components::phys::joint>(),
		"recompute length",
		[this](rynx::scheduler::context* ctx)
		{
			auto& ecs = ctx->get_resource<rynx::ecs>();
			auto* joint = reinterpret_cast<rynx::components::phys::joint*>(this->address_of_operand());
			if(ecs.exists(joint->id_a) & ecs.exists(joint->id_b))
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
	using joint_connector_t = rynx::components::phys::joint::connector_type;
	if (type.m_type_name == rynx::traits::type_name<joint_connector_t>())
	{
		auto field_div = std::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.03f, 0.0f));
		field_div->velocity_position(200.0f); // TODO

		auto jointConnectionButton = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(1.0f, 1.0f, 0.0f));
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

void rynx::editor::tools::joint_tool::update(rynx::scheduler::context& ctx) {
	
	auto& ecs = ctx.get_resource<rynx::ecs>();
	auto& dbug = ctx.get_resource<rynx::application::DebugVisualization>();
	
	auto id = selected_id();
	if (ecs.exists(id)) {
		auto* joint = ecs[id].try_get<rynx::components::phys::joint>();
		if (joint) {
			if (ecs.exists(joint->id_a) & ecs.exists(joint->id_b)) {
				auto& pos_a = ecs[joint->id_a].get<rynx::components::position>();
				auto& pos_b = ecs[joint->id_b].get<rynx::components::position>();
				auto offset_a = rynx::math::rotatedXY(joint->point_a, pos_a.angle);
				auto offset_b = rynx::math::rotatedXY(joint->point_b, pos_b.angle);
				dbug.addDebugLine(pos_a.value + offset_a, pos_b.value + offset_b, {0.2f, 1.0f, 1.0f, 1.0f});
			}
		}
	}
}
