#include <rynx/editor/tools/subscene_tool.hpp>
#include <rynx/menu/Div.hpp>
#include <rynx/menu/Text.hpp>
#include <rynx/menu/Button.hpp>
#include <rynx/menu/FileSelector.hpp>

void rynx::editor::tools::subscene_tool::gen_menu_field_scenelink(
	const rynx::reflection::field& member,
	rynx::editor::component_recursion_info_t info,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	float menuVelocityFast = 20.0f;
	auto scenes = info.ctx->get_resource_ptr<const rynx::scenes>();
	auto vfs = info.ctx->get_resource_ptr<rynx::filesystem::vfs>();
	auto scene_list = scenes->list_scenes();
	auto frame_tex = info.frame_tex;
	
	// need text element with current selected scene name
	// need "pick" button to change the selected scene
	//    pick button onclick-> push popup with scene selection

	auto field_container = rynx::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.03f, 0.0f));
	auto variable_name_label = rynx::make_shared<rynx::menu::Text>(rynx::vec3f(0.4f, 1.0f, 0.0f));

	variable_name_label->text(rynx::string(info.indent, '-') + member.m_field_name);
	variable_name_label->text_align_left();

	auto variable_value_field = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.4f, 1.0f, 0.0f));
	auto pick_scene = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.2f, 1.0f, 0.0f));
	pick_scene->velocity_position(menuVelocityFast);
	pick_scene->text().text("Pick");
	pick_scene->align().target(field_container.get()).top_inside().right_inside().offset_x(0.1f);
	pick_scene->on_click([this, variable_value_field, vfs, scenes, frame_tex, info, member]() mutable {
		this->m_editor_state->m_editor->disable_tools();
		auto fileSelectDialog = rynx::make_shared<rynx::menu::FileSelector>(*vfs, frame_tex, rynx::vec3f(0.3f, 0.6f, 0.0f));
		fileSelectDialog->configure().m_allowNewFile = false;
		fileSelectDialog->configure().m_allowNewDir = false;
		fileSelectDialog->file_type(".rynxscene");
		fileSelectDialog->filepath_to_display_name([vfs, scenes](rynx::string path) {
			return scenes->filepath_to_info(path).name;
		});
		fileSelectDialog->display("/scenes/",
			// on file selected
			[this, variable_value_field, scenes, info, member](rynx::string fileName) mutable {
				bool entity_has_children = info.ctx->get_resource<rynx::ecs>()[info.entity_id].has<rynx::components::scene::children>();
				if (entity_has_children)
					return;

				const auto& scene_info = scenes->filepath_to_info(fileName);
				info.access<rynx::scene_id>(member) = scene_info.id;
				variable_value_field->text()
					.text(scene_info.name);

				execute([this, info] {
					this->m_editor_state->m_editor->enable_tools();
					this->m_editor_state->m_editor->pop_popup();
					this->m_editor_state->m_editor->for_each_tool([info](rynx::editor::itool* tool) {
						tool->on_entity_component_value_changed(info.ctx, info.component_type_id, info.ctx->get_resource<rynx::ecs>(), info.entity_id);
					});
				});
			},
			// on directory selected
				[](rynx::string) {}
			);

		execute([this, fileSelectDialog]() { this->m_editor_state->m_editor->push_popup(fileSelectDialog); });
	});

	field_container->velocity_position(menuVelocityFast);
	variable_name_label->velocity_position(menuVelocityFast);
	variable_value_field->velocity_position(menuVelocityFast);

	rynx::scene_id& value = info.access<rynx::scene_id>(member);
	if (!value) {
		variable_value_field->text()
			.text("no scene selected")
			.text_align_center()
			.text_input_disable();
	}
	else {
		auto& scenes = info.ctx->get_resource<rynx::scenes>();
		if (scenes.id_exists(value)) {
			const auto& scene_info = scenes.id_to_info(value);
			variable_value_field->text()
				.text(scene_info.name)
				.text_align_center()
				.text_input_disable();
		}
		else {
			variable_value_field->text().text("<invalid>").text_align_center().text_input_disable();
		}
	}

	variable_value_field->align().target(variable_name_label.get()).right_outside().top_inside();
	variable_name_label->align().left_inside().top_inside();

	field_container->addChild(variable_name_label);
	field_container->addChild(variable_value_field);
	field_container->addChild(pick_scene);

	field_container->align()
		.target(info.component_sheet->last_child())
		.bottom_outside()
		.left_inside();

	info.component_sheet->addChild(field_container);
}

rynx::editor::tools::subscene_tool::subscene_tool(rynx::scheduler::context& ctx) {
}

void rynx::editor::tools::subscene_tool::update(rynx::scheduler::context& ctx) {

}

void rynx::editor::tools::subscene_tool::verify(rynx::scheduler::context&, error_emitter&) {}

void rynx::editor::tools::subscene_tool::on_tool_selected() {
}

void rynx::editor::tools::subscene_tool::on_tool_unselected() {
}

bool rynx::editor::tools::subscene_tool::operates_on(rynx::type_id_t type_id) {
	return type_id == rynx::type_index::id<rynx::scene_id>();
}

rynx::string rynx::editor::tools::subscene_tool::get_tool_name() {
	return "subscene tool";
}

rynx::string rynx::editor::tools::subscene_tool::get_button_texture() {
	return "subscene_tool";
}

void rynx::editor::tools::subscene_tool::on_entity_component_added(
	rynx::scheduler::context* ctx,
	rynx::type_index::type_id_t componentType,
	rynx::ecs& ecs,
	rynx::id id)
{

}

void rynx::editor::tools::subscene_tool::on_entity_component_removed(
	rynx::scheduler::context* ctx,
	rynx::type_index::type_id_t componentType,
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

		// recursively update tools with the edited child entities.
		for (auto child : entity.get<rynx::components::scene::children>().entities) {
			this->for_each_tool([this, child, &ecs](rynx::editor::itool* tool) {
				tool->on_entity_component_value_changed(
					m_editor_state->m_editor->get_context(),
					rynx::type_index::id<rynx::components::transform::position>(),
					ecs,
					child
				);
			});
		}
	}
}
