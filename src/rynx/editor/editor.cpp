
#include <rynx/editor/editor.hpp>
#include <rynx/graphics/mesh/collection.hpp>
#include <rynx/tech/filesystem/filesystem.hpp>
#include <rynx/menu/FileSelector.hpp>
#include <rynx/application/visualisation/debug_visualisation.hpp>
#include <rynx/editor/tools/selection_tool.hpp>
#include <rynx/graphics/camera/camera.hpp>
#include <rynx/ecs/scenes.hpp>
#include <rynx/ecs/scene_serialization.hpp>

#include <sstream>

namespace
{
	static constexpr float menuVelocityFast = 20.0f;
	static constexpr float menuVelocityMedium = 5.0f;
	static constexpr float menuVelocitySlow = 2.0f;
}

rynx::string humanize(rynx::string s) {
	auto replace_all = [&s](rynx::string what, rynx::string with) {
		while (s.find(what) != s.npos) {
			s.replace(s.find(what), what.length(), with);
		}
	};

	replace_all("class", "");
	replace_all("struct", "");
	replace_all(" ", "");
	replace_all("rynx::math", "r::m");
	replace_all("rynx::", "r::");
	replace_all("components::", "comp::");
	replace_all("vec3<float>", "vec3f");
	replace_all("vec4<float>", "vec4f");
	return s;
}

void register_post_deserialize_operations(rynx::ecs& ecs) {
	ecs.register_post_deserialize_init_function([](rynx::ecs& ecs, rynx::scheduler::context& ctx) {
		ecs.query().for_each([](rynx::components::phys::boundary& boundary, rynx::components::transform::position pos) {
			if (boundary.segments_local.size() != boundary.segments_world.size()) {
				boundary.segments_world = boundary.segments_local;
				boundary.update_world_positions(pos.value, pos.angle);
			}
		});
	});
}

void rynx::editor::field_float(
	const rynx::reflection::field& member,
	rynx::editor::component_recursion_info_t info,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	int32_t mem_offset = info.cumulative_offset + member.m_memory_offset;
	float value = ecs_value_editor().access<float>(*info.ecs, info.entity_id, info.component_type_id, mem_offset);

	auto field_container = rynx::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.03f, 0.0f));
	field_container->velocity_position(menuVelocityFast);
	
	auto variable_name_label = rynx::make_shared<rynx::menu::Text>(rynx::vec3f(0.4f, 1.0f, 0.0f));
	variable_name_label->velocity_position(menuVelocityFast);
	variable_name_label->text(rynx::string(info.indent, '-') + member.m_field_name);
	variable_name_label->text_align_left();

	auto variable_value_field = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.4f, 1.0f, 0.0f));
	variable_value_field->velocity_position(menuVelocityFast);

	struct field_config {
		float min_value = std::numeric_limits<float>::lowest();
		float max_value = std::numeric_limits<float>::max();
		bool slider_dynamic = true;

		float constrain(float v) const {
			if (v < min_value) return min_value;
			if (v > max_value) return max_value;
			return v;
		}
	};

	field_config config;

	auto handle_annotations = [&variable_name_label, &info, &member, &config](
		// const rynx::reflection::type& type,
		const rynx::reflection::field& field)
	{
		bool skip_next = false;
		for (auto&& annotation : field.m_annotations) {
			if (annotation.starts_with("applies_to")) {
				std::stringstream ss(annotation.c_str());
				rynx::string v;
				ss >> v;

				bool self_found = false;
				while (ss >> v) {
					self_found |= (v == member.m_field_name);
				}

				skip_next = !self_found;
			}

			if (annotation == "applies_to_all") {
				skip_next = false;
			}

			if (skip_next) {
				continue;
			}

			if (annotation.starts_with("rename")) {
				std::stringstream ss(annotation.c_str());
				rynx::string v;
				ss >> v >> v;
				rynx::string source_name = v;
				ss >> v;
				if (source_name == member.m_field_name) {
					variable_name_label->text(rynx::string(info.indent, '-') + v);
				}
			}
			else if (annotation == ">=0") {
				config.min_value = 0;
			}
			else if (annotation.starts_with("except")) {
				std::stringstream ss(annotation.c_str());
				rynx::string v;
				ss >> v;

				while (ss >> v) {
					if (v == member.m_field_name) {
						skip_next = true;
					}
				}
			}
			else if (annotation.starts_with("range")) {
				std::stringstream ss(annotation.c_str());
				rynx::string v;
				ss >> v;
				ss >> v;
				config.min_value = rynx::stof(v);
				ss >> v;
				config.max_value = rynx::stof(v);
				config.slider_dynamic = false;
			}
		}
	};

	for (auto it = reflection_stack.rbegin(); it != reflection_stack.rend(); ++it)
		handle_annotations(it->second);
	handle_annotations(member);

	variable_value_field->text()
		.text(rynx::to_string(value))
		.text_align_right()
		.text_input_enable();

	rynx::shared_ptr<rynx::menu::SlideBarVertical> value_slider;

	if (config.slider_dynamic) {
		value_slider = rynx::make_shared<rynx::menu::SlideBarVertical>(info.frame_tex, info.frame_tex, rynx::vec3f(0.2f, 1.0f, 0.0f), -1.0f, +1.0f);
		value_slider->velocity_position(menuVelocityFast);
		value_slider->setValue(0);
		value_slider->on_active_tick([info, mem_offset, config, self = value_slider.get(), text_element = variable_value_field.get()](float /* input_v */, float dt) {
			float& v = ecs_value_editor().access<float>(*info.ecs, info.entity_id, info.component_type_id, mem_offset);
			float input_v = self->getValueCubic_AroundCenter();
			float tmp = v + dt * input_v;
			constexpr float value_modify_velocity = 3.0f;
			if (input_v * v > 0) {
				tmp *= (1.0f + std::fabs(input_v) * value_modify_velocity * dt);
			}
			else {
				tmp *= 1.0f / (1.0f + std::fabs(input_v) * value_modify_velocity * dt);
			}
			v = config.constrain(tmp);
			text_element->text().text(rynx::to_string(v));
		});

		value_slider->on_drag_end([self = value_slider.get()](float /* v */) {
			self->setValue(0);
		});
	}
	else {
		value_slider = rynx::make_shared<rynx::menu::SlideBarVertical>(
			info.frame_tex,
			info.frame_tex,
			rynx::vec3f(0.2f, 1.0f, 0.0f),
			config.min_value,
			config.max_value);

		value_slider->velocity_position(menuVelocityFast);
		value_slider->setValue(ecs_value_editor().access<float>(*info.ecs, info.entity_id, info.component_type_id, mem_offset));
		value_slider->on_value_changed([info, mem_offset, text_element = variable_value_field.get()](float v) {
			float& field_value = ecs_value_editor().access<float>(*info.ecs, info.entity_id, info.component_type_id, mem_offset);
			field_value = v;
			text_element->text().text(rynx::to_string(v));
		});

		value_slider->on_update([info, mem_offset, self = value_slider.get()]() {
			if (!self->has_dedicated_mouse_input()) {
				float field_value = ecs_value_editor().access<float>(*info.ecs, info.entity_id, info.component_type_id, mem_offset);
				self->setValue(field_value);
			}
		});
	}

	variable_value_field->text().on_value_changed([info, mem_offset, config, slider_ptr = value_slider.get()](const rynx::string& s) {
		float new_value = 0.0f;
		try { new_value = config.constrain(std::stof(s.c_str())); }
		catch (...) { return; }

		ecs_value_editor().access<float>(*info.ecs, info.entity_id, info.component_type_id, mem_offset) = new_value;
		if (!config.slider_dynamic) {
			slider_ptr->setValue(new_value);
		}
	});

	variable_value_field->on_update([info, mem_offset, self = variable_value_field.get()]() {
		if (!self->text().has_dedicated_keyboard_input()) {
			float field_value = ecs_value_editor().access<float>(*info.ecs, info.entity_id, info.component_type_id, mem_offset);
			self->text().text(rynx::to_string(field_value));
		}
	});

	value_slider->align().right_inside().top_inside().offset_x(-0.15f);
	variable_value_field->align().target(value_slider.get()).left_outside().top_inside();
	variable_name_label->align().left_inside().top_inside();

	field_container->addChild(variable_name_label);
	field_container->addChild(value_slider);
	field_container->addChild(variable_value_field);

	field_container->align()
		.target(info.component_sheet->last_child())
		.bottom_outside()
		.left_inside();

	info.component_sheet->addChild(field_container);
}

void rynx::editor::field_bool(
	const rynx::reflection::field& member,
	rynx::editor::component_recursion_info_t info,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	int32_t mem_offset = info.cumulative_offset + member.m_memory_offset;
	bool value = ecs_value_editor().access<bool>(*info.ecs, info.entity_id, info.component_type_id, mem_offset);

	auto field_container = rynx::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.03f, 0.0f));
	auto variable_name_label = rynx::make_shared<rynx::menu::Text>(rynx::vec3f(0.4f, 1.0f, 0.0f));

	variable_name_label->text(rynx::string(info.indent, '-') + member.m_field_name);
	variable_name_label->text_align_left();

	auto variable_value_field = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.4f, 1.0f, 0.0f));

	field_container->velocity_position(menuVelocityFast);
	variable_name_label->velocity_position(menuVelocityFast);
	variable_value_field->velocity_position(menuVelocityFast);

	variable_value_field->text()
		.text(value ? "^gYes" : "^rNo")
		.text_align_center()
		.text_input_disable();

	variable_value_field->on_click([info, mem_offset, self = variable_value_field.get()]() {
		bool& value = ecs_value_editor().access<bool>(*info.ecs, info.entity_id, info.component_type_id, mem_offset);
		value = !value;
		self->text().text(value ? "^gYes" : "^rNo");
	});

	variable_value_field->align().right_inside().top_inside();
	variable_name_label->align().left_inside().top_inside();

	field_container->addChild(variable_name_label);
	field_container->addChild(variable_value_field);

	field_container->align()
		.target(info.component_sheet->last_child())
		.bottom_outside()
		.left_inside();

	info.component_sheet->addChild(field_container);
}


void rynx::editor_rules::generate_menu_for_reflection(
	const rynx::reflection::type& type_reflection,
	rynx::editor::component_recursion_info_t info,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	for (auto&& member : type_reflection.m_fields) {
		{
			bool tool_created_menu = false;
			for (auto&& tool : m_tools) {
				rynx::editor::component_recursion_info_t field_info = info;
				field_info.cumulative_offset += member.m_memory_offset;
				++field_info.indent;
				tool_created_menu |= tool->try_generate_menu(member, field_info, reflection_stack);
			}
			if (tool_created_menu) {
				continue;
			}
		}

		if (member.m_type_name == "float") {
			rynx::editor::component_recursion_info_t field_info = info;
			++field_info.indent;
			field_float(member, field_info, reflection_stack);
		}
		else if (member.m_type_name == "bool") {
			rynx::editor::component_recursion_info_t field_info = info;
			++field_info.indent;
			field_bool(member, field_info, reflection_stack);
		}
		else if (info.reflections->has(member.m_type_name)) {
			auto label = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.6f, 0.03f, 0.0f));
			label->text()
				.text(rynx::string(info.indent + 1, '-') + member.m_field_name + " (" + humanize(member.m_type_name) + ")")
				.text_align_left();

			label->align()
				.target(info.component_sheet->last_child())
				.bottom_outside()
				.left_inside();

			label->velocity_position(menuVelocityFast);
			info.component_sheet->addChild(label);

			// Create tools buttons
			for (auto&& tool : m_tools) {
				if (tool->operates_on(m_reflections.find(member.m_type_name)->m_type_index_value)) {
					std::vector<rynx::string> actionNames = tool->get_editor_actions_list();
					if (!actionNames.empty()) {
						for (auto&& actionName : actionNames) {
							auto edit_button = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.6f, 0.03f, 0.0f));
							edit_button->velocity_position(menuVelocityFast);
							edit_button->align().target(info.component_sheet->last_child()).bottom_outside().left_inside();
							edit_button->text().text("^g" + tool->get_tool_name() + ": " + actionName);
							edit_button->text().text_align_left();
							edit_button->on_click([this, info, member, actionName, tool_ptr = tool.get()]() {
								tool_ptr->source_data([=]() {
									int32_t mem_offset = info.cumulative_offset + member.m_memory_offset;
									void* value_ptr = rynx::editor::ecs_value_editor().compute_address(*info.ecs, info.entity_id, info.component_type_id, mem_offset);
									return value_ptr;
								});

								if (tool_ptr->editor_action_execute(actionName, m_context)) {
									this->switch_to_tool(*tool_ptr);
								}
							});

							info.component_sheet->addChild(edit_button);
						}
					}
					else {
						auto edit_button = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.6f, 0.03f, 0.0f));
						edit_button->velocity_position(menuVelocityFast);
						edit_button->align().target(info.component_sheet->last_child()).bottom_outside().left_inside();
						edit_button->text().text("^g" + tool->get_tool_name());
						edit_button->text().text_align_left();
						edit_button->on_click([this, info, member, tool_ptr = tool.get()]() {
							tool_ptr->source_data([=]() {
								int32_t mem_offset = info.cumulative_offset + member.m_memory_offset;
								void* value_ptr = rynx::editor::ecs_value_editor().compute_address(*info.ecs, info.entity_id, info.component_type_id, mem_offset);
								return value_ptr;
							});

							this->switch_to_tool(*tool_ptr);
						});

						info.component_sheet->addChild(edit_button);
					}
				}
			}

			rynx::editor::component_recursion_info_t field_info;
			field_info = info;
			field_info.cumulative_offset += member.m_memory_offset;
			++field_info.indent;

			reflection_stack.emplace_back(type_reflection, member);
			const auto& member_type_reflection = info.reflections->get(member);
			field_info.gen_type_editor(member_type_reflection, field_info, reflection_stack);
			reflection_stack.pop_back();
		}
		else {
			auto label = rynx::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.6f, 0.03f, 0.0f));
			label->text()
				.text(rynx::string(info.indent + 1, '-') + member.m_field_name + " (" + humanize(member.m_type_name) + ")")
				.text_align_left();

			label->align()
				.target(info.component_sheet->last_child())
				.bottom_outside()
				.left_inside();

			label->velocity_position(menuVelocityFast);
			info.component_sheet->addChild(label);
		}
	}
}



void rynx::editor_rules::on_entity_selected(rynx::id id) {
	rynx::ecs& ecs = m_context->get_resource<rynx::ecs>();
	if (!ecs.exists(id)) {
		m_components_list->clear_children();
		return;
	}

	m_state.m_selected_ids.clear();
	m_state.m_selected_ids.emplace_back(id);

	rynx::graphics::GPUTextures& textures = m_context->get_resource<rynx::graphics::GPUTextures>();

	auto reflections_vec = ecs[id].reflections(m_reflections);
	m_components_list->clear_children();

	for (auto&& reflection_entry : reflections_vec) {
		auto component_sheet = rynx::make_shared<rynx::menu::Div>(rynx::vec3f(0.0f, 0.0f, 0.0f));
		component_sheet->set_background(frame_tex);
		component_sheet->set_dynamic_position_and_scale();
		component_sheet->velocity_position(menuVelocityFast);
		m_components_list->addChild(component_sheet);

		{
			auto component_name = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.6f - 0.10f, 0.025f, 0.0f));
			component_name->velocity_position(menuVelocityFast);
			component_name->no_focus_alpha(1.0f);
			component_name->text()
				.text(reflection_entry.m_type_name)
				.color({1.0f, 1.0f, 0.0f, 1.0f})
				.text_align_left()
				.font(m_font);

			component_name->align()
				.top_inside()
				.left_inside()
				.offset_kind_relative_to_self()
				.offset_y(-0.5f);


			auto delete_component = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.10f, 0.025f, 0.0f));
			delete_component->velocity_position(menuVelocityFast);
			delete_component->no_focus_alpha(0.7f);
			delete_component->text()
				.text("Remove")
				.color({1.0f, 0.2f, 0.8f, 1.0f})
				.text_align_center()
				.font(m_font);

			// align to right side of component name bar.
			delete_component->align()
				.target(component_name.get())
				.top_inside()
				.right_outside();

			delete_component->on_click([this, parent = component_sheet.get(), &ecs, id, reflection_entry]() {
				execute([=, &ecs]() {
					ecs.editor().removeFromEntity(id, reflection_entry.m_type_index_value);
					parent->die();

					m_state.m_editor->for_each_tool([this, reflection_entry, &ecs, id](rynx::editor::itool* tool_ptr) {
						tool_ptr->on_entity_component_removed(m_context, reflection_entry.m_type_index_value, ecs, id);
					});
				});
			});

			bool allowComponentRemove = true;
			for (auto& tool : m_tools) {
				allowComponentRemove &= tool->allow_component_remove(reflection_entry.m_type_index_value);
			}
			if (allowComponentRemove) {
				component_sheet->addChild(delete_component);
			}
			component_sheet->addChild(component_name);

			for (auto&& tool : m_tools) {
				if (tool->operates_on(reflection_entry.m_type_index_value)) {
					std::vector<rynx::string> actionNames = tool->get_editor_actions_list();
					if (!actionNames.empty()) {
						for (auto&& actionName : actionNames) {
							auto edit_button = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.6f, 0.03f, 0.0f));
							edit_button->velocity_position(menuVelocityFast);
							edit_button->align().target(component_sheet->last_child()).bottom_outside().left_inside();
							edit_button->text().text("^g" + tool->get_tool_name() + ": " + actionName);
							edit_button->text().text_align_left();

							auto component_type_id = reflection_entry.m_type_index_value;
							edit_button->on_click([this, &ecs, actionName, entity_id = id, component_type_id, tool_ptr = tool.get()]() {
								tool_ptr->source_data([component_type_id, entity_id, ecs_ptr = &ecs]() {
									void* value_ptr = rynx::editor::ecs_value_editor().compute_address(*ecs_ptr, entity_id, component_type_id, 0);
									return value_ptr;
								});
								
								if (tool_ptr->editor_action_execute(actionName, m_context)) {
									this->switch_to_tool(*tool_ptr);
								}
							});

							component_sheet->addChild(edit_button);
						}
					}
					else {
						auto edit_button = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.6f, 0.03f, 0.0f));
						edit_button->velocity_position(menuVelocityFast);
						edit_button->align().target(component_name.get()).bottom_outside().left_inside();
						edit_button->text().text("^g" + tool->get_tool_name());
						edit_button->text().text_align_left();

						auto component_type_id = reflection_entry.m_type_index_value;
						edit_button->on_click([this, &ecs, entity_id = id, component_type_id, tool_ptr = tool.get()]() {
							tool_ptr->source_data([component_type_id, entity_id, ecs_ptr = &ecs]() {
								void* value_ptr = rynx::editor::ecs_value_editor().compute_address(*ecs_ptr, entity_id, component_type_id, 0);
								return value_ptr;
								});
							this->switch_to_tool(*tool_ptr);
						});
						
						component_sheet->addChild(edit_button);
					}
				}
			}
		}

		rynx::editor::component_recursion_info_t component_common_info;
		component_common_info.component_type_id = reflection_entry.m_type_index_value;
		component_common_info.ecs = &ecs;
		component_common_info.reflections = &m_reflections;
		component_common_info.component_sheet = component_sheet.get();
		component_common_info.ctx = m_context;

		component_common_info.entity_id = id;
		component_common_info.textures = &textures;
		component_common_info.frame_tex = frame_tex;
		component_common_info.knob_tex = knob_tex;

		component_common_info.gen_type_editor = [this](
			const rynx::reflection::type& type_reflection,
			rynx::editor::component_recursion_info_t info,
			std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
		{
			generate_menu_for_reflection(type_reflection, info, std::move(reflection_stack));
		};

		component_common_info.gen_type_editor(reflection_entry, component_common_info, {});
	}
}


void rynx::editor_rules::add_tool(rynx::unique_ptr<rynx::editor::itool> tool) {
	auto icon_tex_id = m_context->get_resource<rynx::graphics::GPUTextures>().findTextureByName(tool->get_button_texture());
	auto selection_tool_button = rynx::make_shared<rynx::menu::Button>(icon_tex_id, rynx::vec3f(0.25f, 0.25f, 0.0f));
	selection_tool_button->respect_aspect_ratio();

	if (m_tools.empty()) {
		selection_tool_button->align().target(m_tools_bar.get()).top_left_inside().offset(-0.3f);
	}
	else {
		constexpr int toolsPerRow = 3;
		if (m_tools.size() % toolsPerRow != 0) {
			auto* lastComponent = m_tools_bar->last_child();
			selection_tool_button->align().target(lastComponent).top_inside().right_outside().offset_x(+0.1f);
		}
		else {
			auto* lineFirst = m_tools_bar->last_child(toolsPerRow -1);
			selection_tool_button->align().target(lineFirst).bottom_outside().left_inside().offset_y(+0.1f);
		}
	}

	selection_tool_button->velocity_position(menuVelocityFast);
	selection_tool_button->on_click([this, tool = tool.get()]() { switch_to_tool(*tool); });
	m_tools_bar->addChild(selection_tool_button);
	m_tools.emplace_back(std::move(tool));
}

void rynx::editor_rules::display_list_dialog(
	std::vector<rynx::string> entries,
	rynx::function<rynx::floats4(rynx::string)> entryColor,
	rynx::function<void(rynx::string)> on_selection)
{
	disable_tools();
	execute([this, entries, entryColor, on_selection]() {
		auto options_list = rynx::make_shared<rynx::menu::List>(frame_tex, rynx::vec3f(0.3f, 0.7f, 0.0f));
		options_list->list_element_velocity(menuVelocityFast);
		options_list->velocity_position(menuVelocityFast);
		
		for (auto&& entry : entries) {
			auto button = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(1.f, 0.05f, 0.0f));
			button->velocity_position(menuVelocityFast);
			button->text().text(entry);
			button->text().text_align_left();
			button->text().color() = entryColor(entry);
			button->on_click([this, entry, on_selection]() {
				execute([this, entry, on_selection]() {
					pop_popup();
					enable_tools();
					on_selection(entry);
				});
			});

			options_list->addChild(button);
		}

		push_popup(options_list);
	});
}

void rynx::editor_rules::save_scene_to_path(rynx::string path) {
	auto& meshes = m_context->get_resource<rynx::graphics::mesh_collection>();
	auto& ecs = m_context->get_resource<rynx::ecs>();
	auto& scenes = m_context->get_resource<rynx::scenes>();
	auto& vfs = m_context->get_resource<rynx::filesystem::vfs>();

	logmsg("saving active scene to '%s'", path.c_str());
	auto vector_writer = rynx::ecs_detail::scene_serializer(ecs).serialize_scene(m_reflections, vfs, scenes);
	rynx::serialization::vector_writer system_writer = all_rulesets().serialize(*m_context);
	rynx::serialize(system_writer.data(), vector_writer);

	auto pos = path.find_last_of('/');
	rynx::string scene_name = ((pos != rynx::string::npos) ? path.substr(pos + 1) : path);
	scenes.save_scene(vfs, vector_writer.data(), path, scene_name, path);
	meshes.save_all_meshes_to_disk("../meshes");
	// rynx::filesystem::write_file(path, vector_writer.data());
}

void rynx::editor_rules::load_scene_from_path(rynx::string scene_path) {
	auto& meshes = m_context->get_resource<rynx::graphics::mesh_collection>();
	auto& ecs = m_context->get_resource<rynx::ecs>();
	auto& scenes = m_context->get_resource<rynx::scenes>();
	auto& vfs = m_context->get_resource<rynx::filesystem::vfs>();

	meshes.load_all_meshes_from_disk("../meshes");
	
	m_state.m_selected_ids.clear();
	on_entity_selected(0);
	
	// TODO: use scene id instead of path
	rynx::serialization::vector_reader reader(scenes.get(vfs, scene_path));
	
	ecs.clear();
	all_rulesets().clear(*m_context);

	rynx::ecs_detail::scene_serializer(ecs).deserialize_scene(m_reflections, vfs, scenes, rynx::components::transform::position{}, reader);
	auto serialized_rulesets_state = rynx::deserialize<std::vector<char>>(reader);
	
	rynx::serialization::vector_reader rulesets_reader(serialized_rulesets_state);
	all_rulesets().deserialize(*m_context, rulesets_reader);
	ecs.post_deserialize_init(*m_context);
}

rynx::editor_rules::editor_rules(
	rynx::scheduler::context& ctx,
	rynx::binary_config::id game_running_state,
	rynx::reflection::reflections& reflections,
	Font* font,
	rynx::menu::Component* editor_menu_host,
	rynx::graphics::GPUTextures& textures)
	: m_reflections(reflections)
{
	m_context = &ctx;
	m_font = font;
	m_game_running_state = game_running_state;

	ctx.get_resource<rynx::graphics::mesh_collection>().load_all_meshes_from_disk("../meshes");
	register_post_deserialize_operations(m_context->get_resource<rynx::ecs>());

	struct tool_api_impl : public rynx::editor::editor_shared_state::tool_api {
	private:
		editor_rules* m_host;
		
	public:
		tool_api_impl(editor_rules* host) : m_host(host) {}

		virtual void push_popup(rynx::shared_ptr<rynx::menu::Component> popup) override {
			m_host->push_popup(popup);
		}
		
		virtual void pop_popup() override {
			m_host->pop_popup();
		}
		
		virtual void enable_tools() override {
			m_host->enable_tools();
		}
		
		virtual rynx::scheduler::context* get_context() {
			return m_host->m_context;
		}

		virtual void disable_tools() override {
			m_host->disable_tools();
		}

		virtual void activate_default_tool() override {
			auto& default_tool = m_host->get_default_tool();
			m_host->switch_to_tool(default_tool);
		}
		
		virtual void execute(rynx::function<void()> execute_in_main_thread) override {
			m_host->execute(std::move(execute_in_main_thread));
		}

		virtual void for_each_tool(rynx::function<void(rynx::editor::itool*)> op) override {
			for (auto& tool : m_host->m_tools) {
				op(tool.get());
			}
		}

	};

	m_state.m_editor = new tool_api_impl(this);

	m_editor_menu = rynx::make_shared<rynx::menu::Div>(rynx::vec3f{ 1, 1, 0 });
	editor_menu_host->addChild(m_editor_menu);

	this->frame_tex = textures.findTextureByName("frame");
	this->knob_tex = textures.findTextureByName("frame");
	
	rynx::observer_ptr<rynx::filesystem::vfs> vfs = ctx.get_resource_ptr<rynx::filesystem::vfs>();
	rynx::observer_ptr<rynx::scenes> scenes = ctx.get_resource_ptr<rynx::scenes>();

	// create editor menus
	{
		// create tools bar
		m_tools_bar = rynx::make_shared<rynx::menu::Div>(rynx::vec3f{ 0.1f, 1.0f, 0.0f });
		m_tools_bar->align().right_inside().offset(+1.0f);

		// create entity components bar
		m_entity_bar = rynx::make_shared<rynx::menu::Div>(rynx::vec3f(0.3f, 1.0f, 0.0f));
		m_entity_bar->align().left_inside().offset(+1.0f);

		auto m_entity_bar_toggle_button = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.025f, 0.2f, 0.0f));
		m_entity_bar->addChild(m_entity_bar_toggle_button);
		m_entity_bar_toggle_button->ignore_parent_scale();
		m_entity_bar_toggle_button->align().center_y().right_outside();
		m_entity_bar_toggle_button->velocity_position(menuVelocityFast);
		m_entity_bar_toggle_button->no_focus_alpha(1.0f);
		m_entity_bar_toggle_button->on_click([entity_bar_ptr = m_entity_bar.get()]() {
			if (entity_bar_ptr->position_world().x < -1.0f) {
				entity_bar_ptr->align().offset(0.0f);
			}
			else {
				entity_bar_ptr->align().offset(1.0f);
			}
		});

		auto m_tools_bar_toggle_button = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.02f, 0.2f, 0.0f));
		m_tools_bar->addChild(m_tools_bar_toggle_button);
		m_tools_bar_toggle_button->ignore_parent_scale();
		m_tools_bar_toggle_button->align().center_y().left_outside();
		m_tools_bar_toggle_button->velocity_position(menuVelocityFast);
		m_tools_bar_toggle_button->no_focus_alpha(1.0f);
		m_tools_bar_toggle_button->on_click([tools_bar_ptr = m_tools_bar.get()]() {
			if (tools_bar_ptr->position_world().x > +1.0f) {
				tools_bar_ptr->align().offset(0.0f);
			}
			else {
				tools_bar_ptr->align().offset(1.0f);
			}
		});


		// create file actions bar
		m_file_actions_bar = rynx::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.1f, 0.0f));
		m_file_actions_bar->align().center_x().bottom_inside();

		auto save_scene = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.2f, 0.50f, 0.0f));
		save_scene->velocity_position(menuVelocityFast);
		save_scene->text().text("Save");
		save_scene->align().top_left_inside();
		save_scene->on_click([this, vfs, scenes]() mutable {
			disable_tools();
			auto fileSelectDialog =
				rynx::make_shared<rynx::menu::FileSelector>(
					*vfs.get(),
					frame_tex,
					rynx::vec3f(0.3f, 0.6f, 0.0f)
				);
			fileSelectDialog->configure().m_allowNewFile = true;
			fileSelectDialog->configure().m_allowNewDir = true;
			fileSelectDialog->file_type(".rynxscene");
			fileSelectDialog->filepath_to_display_name([vfs, scenes](rynx::string path) {
				return  scenes->filepath_to_info(path).name;
			});
			fileSelectDialog->display("/scenes/",
				// on file selected
				[this](rynx::string fileName) {
					save_scene_to_path(fileName);
					execute([this] {
						enable_tools();
						pop_popup();
					});
				},
				// on directory selected
				[](rynx::string) {}
			);

			execute([this, fileSelectDialog]() { push_popup(fileSelectDialog); });
			
			auto tex_conf_data = m_context->get_resource<rynx::graphics::GPUTextures>().serialize();
			rynx::filesystem::write_file("../configs/textures.dat", tex_conf_data.data());
		});

		auto load_scene = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.2f, 0.5f, 0.0f));
		load_scene->velocity_position(menuVelocityFast);
		load_scene->text().text("Load");
		load_scene->align().target(save_scene.get()).top_inside().right_outside().offset_x(0.1f);
		load_scene->on_click([this, vfs, scenes]() mutable {
			disable_tools();
			auto fileSelectDialog = rynx::make_shared<rynx::menu::FileSelector>(*vfs, frame_tex, rynx::vec3f(0.3f, 0.6f, 0.0f));
			fileSelectDialog->configure().m_allowNewFile = false;
			fileSelectDialog->configure().m_allowNewDir = false;
			fileSelectDialog->file_type(".rynxscene");
			fileSelectDialog->filepath_to_display_name([vfs, scenes](rynx::string path) {
				return scenes->filepath_to_info(path).name;
			});
			fileSelectDialog->display("/scenes/",
				// on file selected
				[this](rynx::string fileName) {
					this->m_state.m_selected_ids.clear();
					load_scene_from_path(fileName);
					execute([this] {
						enable_tools();
						pop_popup();
					});
				},
				// on directory selected
				[](rynx::string) {}
			);

			execute([this, fileSelectDialog]() { push_popup(fileSelectDialog); });
		});

		auto create_empty_scene = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.2f, 0.5f, 0.0f));
		create_empty_scene->text().text("New scene");
		create_empty_scene->align().target(load_scene.get()).top_inside().right_outside().offset_x(0.1f);
		create_empty_scene->on_click([this]() {
			execute([this]() {
				this->m_context->get_resource<rynx::ecs>().clear();
				this->m_state.m_selected_ids.clear();
				this->on_entity_selected(0);
				this->all_rulesets().clear(*m_context);
			});
		});
		
		m_file_actions_bar->addChild(save_scene);
		m_file_actions_bar->addChild(load_scene);
		m_file_actions_bar->addChild(create_empty_scene);

		m_tools_bar->velocity_position(menuVelocityMedium);
		m_entity_bar->velocity_position(menuVelocityMedium);
		m_file_actions_bar->velocity_position(menuVelocityMedium);

		// construct top bar
		{
			m_top_bar = rynx::make_shared<rynx::menu::Div>(rynx::vec3f{ 0.4f, 0.05f, 0.0f });
			m_top_bar->align().center_x().top_inside();
			m_top_bar->set_background(frame_tex);
			m_top_bar->velocity_position(200.0f);
			
			{
				// add toggle editor button
				auto toggle_editor_button = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f{ 0.9f, 0.9f, 0.0f });
				toggle_editor_button->respect_aspect_ratio();
				toggle_editor_button->align().left_inside().top_inside().offset_x(-0.5f);
				toggle_editor_button->velocity_position(200.0f);

				auto update_button_visual = [this, self = toggle_editor_button.get()]() {
					if (this->state_id().is_enabled()) {
						self->text().text("ON");
						self->text().color({ 0.0f, 1.0f, 0.0f, 1.0f });
					}
					else {
						self->text().text("OFF");
						self->text().color({ 1.0f, 0.3f, 0.3f, 1.0f });
					}
				};

				toggle_editor_button->on_click([this, update_button_visual, self = toggle_editor_button.get()]() mutable {
					this->state_id().toggle();
					update_button_visual();
				});

				toggle_editor_button->on_update([update_button_visual]() {
					update_button_visual();
				});

				m_top_bar->addChild(toggle_editor_button);
			}

			{
				// add pause button
				auto tex_pause_icon = textures.findTextureByName("icon_pause");
				auto tex_play_icon = textures.findTextureByName("icon_play");
				auto pause_button = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f{ 0.9f, 0.9f, 0.0f });
				pause_button->respect_aspect_ratio();
				pause_button->align().target(m_top_bar->last_child()).right_outside().top_inside().offset_x(+0.5f);
				pause_button->velocity_position(200.0f);


				auto update_button_visual = [this, tex_pause_icon, tex_play_icon, self = pause_button.get()]() {
					if (m_game_running_state.is_enabled()) {
						self->set_texture(tex_pause_icon);
					}
					else {
						self->set_texture(tex_play_icon);
					}
				};

				pause_button->on_click([this, update_button_visual, self = pause_button.get()]() mutable {
					if (!m_game_running_state.is_enabled()) {
						rynx::editor::itool::error_emitter emitter(m_context);
						for (auto&& tool : m_tools) {
							tool->verify(*m_context, emitter);
						}

						if (!emitter.errors().empty()) {
							const auto& error = emitter.errors().front();
							auto popupdiv = rynx::make_shared<rynx::menu::Div>(rynx::vec3f{0.6f, 0.5f, 0});
							popupdiv->set_background(frame_tex);
							
							auto error_text = rynx::make_shared<rynx::menu::Text>(rynx::vec3f(0.8f, 0.1f, 0));
							error_text->text(error.what);
							error_text->align().center_x().center_y();
							popupdiv->addChild(error_text);
							
							for (const auto& op : error.options) {
								auto option_button = rynx::make_shared<rynx::menu::Button>(
									frame_tex,
									rynx::vec3f{ 0.3f, 0.1f, 0 }
								);

								option_button->align().target(popupdiv->last_child()).bottom_outside().center_x();
								option_button->text().text(op.name);
								option_button->on_click([this, op, id = error.entity_id]() {
									execute([this, op, id]() {
										op.op(); 
										pop_popup();
										on_entity_selected(id);
									});
								});

								popupdiv->addChild(option_button);
							}
							
							push_popup(popupdiv);
							return;
						}
					}

					m_game_running_state.toggle();
					update_button_visual();

					if (m_game_running_state.is_enabled()) {
						save_scene_to_path("/scenes/_autosave.rynxscene");
					}
					else {
						load_scene_from_path("/scenes/_autosave.rynxscene");
					}
				});

				pause_button->on_update([update_button_visual]() {
					update_button_visual();
				});

				m_top_bar->addChild(pause_button);
			}
		}

		m_info_text = rynx::make_shared<rynx::menu::Text>(rynx::vec3f{ 0.2f, 0.05f, 0.0f });
		m_info_text->align().target(m_top_bar.get()).center_x().bottom_outside();

		m_editor_menu->addChild(m_info_text);
		m_editor_menu->addChild(m_tools_bar);
		m_editor_menu->addChild(m_entity_bar);
		m_editor_menu->addChild(m_file_actions_bar);
		m_editor_menu->addChild(m_top_bar);

		m_tools_bar->set_background(frame_tex);
		m_entity_bar->set_background(frame_tex);
		m_file_actions_bar->set_background(frame_tex);

		// editor entity view menu
		{
			m_components_list = rynx::make_shared<rynx::menu::List>(frame_tex, rynx::vec3f(1.0f, 0.95f, 0.0f));
			m_components_list->list_endpoint_margin(0.05f);
			m_components_list->list_element_margin(0.05f);
			m_components_list->list_element_velocity(menuVelocityFast);
			m_components_list->align().bottom_inside().left_inside();
			m_components_list->velocity_position(menuVelocityFast);
			
			auto delete_entity_button = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.5f, 0.05f, 0.0f));
			auto add_component_button = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.5f, 0.05f, 0.0f));

			delete_entity_button->velocity_position(menuVelocityFast);
			delete_entity_button->align().target(add_component_button.get()).top_inside().right_outside();
			delete_entity_button->text().text("Delete entity");
			delete_entity_button->on_click([this]() {
				execute([this]() {
					if (m_state.m_selected_ids.size() == 1) {
						m_context->get_resource<rynx::ecs>().erase(m_state.m_selected_ids.front());
						m_state.m_selected_ids.clear();
						m_state.m_on_entity_selected({});
					}
				});
			});
			// add_component_button->addChild(delete_entity_button);

			add_component_button->velocity_position(menuVelocityFast);
			add_component_button->align().top_inside().left_inside();
			add_component_button->text().text("Add component");
			add_component_button->on_click([this, &textures]() {
				execute([this, &textures]() {
					if (m_state.m_selected_ids.size() == 1) {
						rynx::ecs& ecs = m_context->get_resource<rynx::ecs>();
						if (ecs.exists(m_state.m_selected_ids.front())) {
							auto reflection_data = m_reflections.get_reflection_data();
							std::ranges::sort(reflection_data, [](auto& a, auto& b) {return a.first < b.first; });
							auto menu_list = rynx::make_shared<rynx::menu::List>(frame_tex, rynx::vec3f(0.3f, 0.7f, 0.0f));
							menu_list->list_element_velocity(menuVelocityFast);
							menu_list->velocity_position(menuVelocityFast);

							push_popup(menu_list);
							disable_tools();

							auto selected_entity = m_state.m_selected_ids.front().value;

							for (auto&& reflect : reflection_data) {
								auto& type_reflection = reflect.second;

								// only suggest components entity does not already have.
								if (ecs[selected_entity].has(type_reflection.m_type_index_value)) {
									continue;
								}

								// component type filtering
								{
									bool allowed = m_component_filters.empty();
									for (auto&& component_filter : m_component_filters) {
										allowed |= (type_reflection.m_type_name.find(component_filter) != rynx::string::npos);
									}
									if (!allowed)
										continue;
								}

								auto button = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(1.f, 0.05f, 0.0f));
								menu_list->addChild(button);
								button->align().center_x();
								button->velocity_position(menuVelocityFast);
								button->text().text(humanize(type_reflection.m_type_name));
								button->text().text_align_left();
								button->on_click([this, selected_entity, type_reflection, &ecs]() mutable {
									execute([this, selected_entity, type_reflection, &ecs]() mutable {
										opaque_unique_ptr<void> component = type_reflection.m_create_instance_func();
										ecs.editor().attachToEntity_typeErased(
											selected_entity,
											type_reflection.m_type_index_value,
											type_reflection.m_is_type_segregated,
											type_reflection.m_create_table_func,
											type_reflection.m_create_map_func,
											std::move(component)
										);

										m_state.m_editor->for_each_tool([this, type_reflection, &ecs, selected_entity](rynx::editor::itool* tool_ptr) {
											tool_ptr->on_entity_component_added(
												m_context,
												type_reflection.m_type_index_value,
												ecs,
												selected_entity
											);
										});

										pop_popup();
										on_entity_selected(selected_entity);
										enable_tools();
									});
								});
							}
						}
					}
				});
			});

			m_entity_bar->addChild(m_components_list);
			m_entity_bar->addChild(add_component_button);
			m_entity_bar->addChild(delete_entity_button);
		}
	}

	m_state.m_on_entity_selected = [this](rynx::id id) {
		on_entity_selected(id);
	};

	add_tool<rynx::editor::tools::selection_tool>(ctx);

	m_active_tool = m_tools[0].get();
	m_active_tool->on_tool_selected();
}


void rynx::editor_rules::onFrameProcess(rynx::scheduler::context& context, float dt) {

	if (m_tools_enabled) {
		for (auto&& tool : m_tools) {
			tool->update_nofocus(context);
		}
		m_active_tool->update(context);
	}

	while (!m_execute_in_main_stack.empty()) {
		m_execute_in_main_stack.front()();
		m_execute_in_main_stack.erase(m_execute_in_main_stack.begin());
	}

	for (int32_t i = 0; i < int32_t(m_long_ops.size()); ++i) {
		auto& promise = m_long_ops[i];
		if (promise->ready()) {
			promise->execute();
			m_long_ops[i] = std::move(m_long_ops.back());
			m_long_ops.pop_back();
			--i;
		}
	}

	{
		auto& gameInput = context.get_resource<rynx::mapped_input>();
		auto& gameCamera = context.get_resource<rynx::camera>();

		auto mouseRay = gameInput.mouseRay(gameCamera);
		auto mouse_z_plane = mouseRay.intersect(rynx::plane(0, 0, 1, 0));
		mouse_z_plane.first.z = 0;

		if (mouse_z_plane.second) {
			// mouse input stuff
			if (gameInput.isKeyClicked(gameInput.getMouseKeyPhysical(1))) {
				create_empty_entity(mouse_z_plane.first);
			}
		}
	}

	m_info_text->text(m_active_tool->get_tool_name() + ": " + m_active_tool->get_info());

	context.add_task("editor tick", [this, dt](
		rynx::ecs& game_ecs,
		rynx::camera& game_camera,
		rynx::application::DebugVisualization& debugVis)
		{
			const auto& ids = m_state.m_selected_ids;
			for (auto id : ids) {
				if (game_ecs.exists(id)) {
					auto entity = game_ecs[id];
					auto* pos = entity.try_get<rynx::components::transform::position>();
					auto* r = entity.try_get<rynx::components::transform::radius>();
					if (r && pos) {
						rynx::matrix4 m;
						m.discardSetTranslate(pos->value);
						m.scale(r->r * 1.01f);
						debugVis.addDebugCircle(m, { 1, 1 ,1 ,1 }, 0.0f);
					}

					// draw line to parent entity if exists.
					if (auto* parent_comp = entity.try_get<rynx::components::scene::parent>()) {
						if (game_ecs.exists(parent_comp->entity)) {
							auto parent_pos = game_ecs[parent_comp->entity].get<rynx::components::transform::position>();
							debugVis.addDebugLine(parent_pos.value, pos->value, { 1.0f, 0.0f, 1.0f, 1.0f });
						}
					}

					// draw lines to child entities if exists
					if (auto* children_comp = entity.try_get<rynx::components::scene::children>()) {
						for (auto child_id : children_comp->entities) {
							if (game_ecs.exists(child_id)) {
								auto child_pos = game_ecs[child_id].get<rynx::components::transform::position>();
								rynx::floats4 color{ 0.2f, 1.0f, 0.2f, 1.0f };
								debugVis.addDebugLine(child_pos.value, pos->value, color);

								rynx::matrix4 circleMat;
								circleMat.discardSetTranslate(child_pos.value);
								circleMat.scale(game_ecs[child_id].get<rynx::components::transform::radius>().r);
								debugVis.addDebugCircle(circleMat, {0.2f, 1.0f, 0.2f, 1.0f});
							}
						}
					}
				}
			}

			// draw scene roots (circle + name of scene) or something
			game_ecs.query().for_each([&](rynx::components::scene::link subscene_link, rynx::components::transform::position pos) {
				rynx::matrix4 circleMat;
				circleMat.discardSetTranslate(pos.value);
				circleMat.scale(10.0f);

				debugVis.addDebugCircle(circleMat, {1.0f, 0.6f, 0.6f, 1.0f});
			});

			auto pos = game_camera.position();

			// visualize world origin
			debugVis.addDebugLine({ -1000.0f + pos.x, 0, 0 }, { +1000 + pos.x, 0, 0 }, { 0.0f, 1.0f, 0.0f, 0.5f });
			debugVis.addDebugLine({ 0, -1000.0f + pos.y, 0 }, { 0, +1000 + pos.y, 0 }, { 0.0f, 1.0f, 0.0f, 0.5f });

			float cameraHeight = std::fabsf(pos.z);
			float smallestGridEntry = 1.0f;
			if (cameraHeight > smallestGridEntry) {
				while (cameraHeight > smallestGridEntry * 0.3f) {
					smallestGridEntry *= 10.0f;
				}
				smallestGridEntry *= 0.01f;
			}
			else {
				while (cameraHeight > smallestGridEntry) {
					smallestGridEntry *= 0.1f;
				}
			}

			// minPos = x * smallestGridEntry -> x = minPos / smallestGridEntry
			float x_min_value = pos.x - cameraHeight;
			float x_max_value = pos.x + cameraHeight;
			float y_min_value = pos.y - cameraHeight;
			float y_max_value = pos.y + cameraHeight;

			int min_x = int((pos.x - cameraHeight * 1.5f) / smallestGridEntry); // * 1.5f is aspect ratio correction.
			int max_x = int((pos.x + cameraHeight * 1.5f) / smallestGridEntry); // * 1.5f is aspect ratio correction.
			int min_y = int((pos.y - cameraHeight) / smallestGridEntry);
			int max_y = int((pos.y + cameraHeight) / smallestGridEntry);

			// TODO: draw some kind of helpers to show distances
			for (int current_x = min_x; current_x < max_x; ++current_x) {
				if (current_x == 0)
					continue;

				float line_x_value = current_x * smallestGridEntry;
				debugVis.addDebugLine(
					{ line_x_value, y_min_value, 0 },
					{ line_x_value, y_max_value, 0 },
					{ 0.0f, 0.0f, 1.0f, 0.5f }
				);
			}

			for (int current_y = min_y; current_y < max_y; ++current_y) {
				if (current_y == 0)
					continue;

				float line_y_value = current_y * smallestGridEntry;
				debugVis.addDebugLine(
					{ x_min_value, line_y_value, 0 },
					{ x_max_value, line_y_value, 0 },
					{ 0.0f, 0.0f, 1.0f, 0.5f }
				);
			}
		}
	);
}