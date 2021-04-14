
#include <rynx/editor/editor.hpp>

#include <sstream>

std::string humanize(std::string s) {
	auto replace_all = [&s](std::string what, std::string with) {
		while (s.find(what) != s.npos) {
			s.replace(s.find(what), what.length(), with);
		}
	};

	replace_all("class", "");
	replace_all("struct", "");
	replace_all(" ", "");
	replace_all("rynx::math", "r::m");
	replace_all("rynx::", "r::");
	replace_all("vec3<float>", "vec3f");
	replace_all("vec4<float>", "vec4f");
	return s;
}

void rynx::editor::field_float(
	const rynx::reflection::field& member,
	rynx_common_info info,
	rynx::menu::Component* component_sheet,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	int32_t mem_offset = info.cumulative_offset + member.m_memory_offset;
	float value = ecs_value_editor().access<float>(*info.ecs, info.entity_id, info.component_type_id, mem_offset);

	auto field_container = std::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.03f, 0.0f));
	auto variable_name_label = std::make_shared<rynx::menu::Text>(rynx::vec3f(0.4f, 1.0f, 0.0f));
	variable_name_label->text(std::string(info.indent, '-') + member.m_field_name);
	variable_name_label->text_align_left();

	auto variable_value_field = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.4f, 1.0f, 0.0f));

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
				std::stringstream ss(annotation);
				std::string v;
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
				std::stringstream ss(annotation);
				std::string v;
				ss >> v >> v;
				std::string source_name = v;
				ss >> v;
				if (source_name == member.m_field_name) {
					variable_name_label->text(std::string(info.indent, '-') + v);
				}
			}
			else if (annotation == ">=0") {
				config.min_value = 0;
			}
			else if (annotation.starts_with("except")) {
				std::stringstream ss(annotation);
				std::string v;
				ss >> v;

				while (ss >> v) {
					if (v == member.m_field_name) {
						skip_next = true;
					}
				}
			}
			else if (annotation.starts_with("range")) {
				std::stringstream ss(annotation);
				std::string v;
				ss >> v;
				ss >> v;
				config.min_value = std::stof(v);
				ss >> v;
				config.max_value = std::stof(v);
				config.slider_dynamic = false;
			}
		}
	};

	for (auto it = reflection_stack.rbegin(); it != reflection_stack.rend(); ++it)
		handle_annotations(it->second);
	handle_annotations(member);

	variable_value_field->text()
		.text(std::to_string(value))
		.text_align_right()
		.input_enable();

	std::shared_ptr<rynx::menu::SlideBarVertical> value_slider;

	if (config.slider_dynamic) {
		value_slider = std::make_shared<rynx::menu::SlideBarVertical>(info.frame_tex, info.frame_tex, rynx::vec3f(0.2f, 1.0f, 0.0f), -1.0f, +1.0f);
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
			text_element->text().text(std::to_string(v));
		});

		value_slider->on_drag_end([self = value_slider.get()](float /* v */) {
			self->setValue(0);
		});
	}
	else {
		value_slider = std::make_shared<rynx::menu::SlideBarVertical>(
			info.frame_tex,
			info.frame_tex,
			rynx::vec3f(0.2f, 1.0f, 0.0f),
			config.min_value,
			config.max_value);

		value_slider->setValue(ecs_value_editor().access<float>(*info.ecs, info.entity_id, info.component_type_id, mem_offset));
		value_slider->on_value_changed([info, mem_offset, text_element = variable_value_field.get()](float v) {
			float& field_value = ecs_value_editor().access<float>(*info.ecs, info.entity_id, info.component_type_id, mem_offset);
			field_value = v;
			text_element->text().text(std::to_string(v));
		});
	}

	variable_value_field->text().on_value_changed([info, mem_offset, config, slider_ptr = value_slider.get()](const std::string& s) {
		float new_value = 0.0f;
		try { new_value = config.constrain(std::stof(s)); }
		catch (...) { return; }

		ecs_value_editor().access<float>(*info.ecs, info.entity_id, info.component_type_id, mem_offset) = new_value;
		if (!config.slider_dynamic) {
			slider_ptr->setValue(new_value);
		}
	});

	value_slider->align().right_inside().top_inside().offset_x(-0.15f);
	variable_value_field->align().target(value_slider.get()).left_outside().top_inside();
	variable_name_label->align().left_inside().top_inside();

	field_container->addChild(variable_name_label);
	field_container->addChild(value_slider);
	field_container->addChild(variable_value_field);

	field_container->align()
		.target(component_sheet->last_child())
		.bottom_outside()
		.left_inside();

	variable_name_label->velocity_position(2000.0f);
	variable_value_field->velocity_position(1000.0f);
	value_slider->velocity_position(1000.0f);
	component_sheet->addChild(field_container);
}


void rynx::editor::field_bool(
	const rynx::reflection::field& member,
	struct rynx_common_info info,
	rynx::menu::Component* component_sheet,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	int32_t mem_offset = info.cumulative_offset + member.m_memory_offset;
	bool value = ecs_value_editor().access<bool>(*info.ecs, info.entity_id, info.component_type_id, mem_offset);

	auto field_container = std::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.03f, 0.0f));
	auto variable_name_label = std::make_shared<rynx::menu::Text>(rynx::vec3f(0.4f, 1.0f, 0.0f));

	variable_name_label->text(std::string(info.indent, '-') + member.m_field_name);
	variable_name_label->text_align_left();

	auto variable_value_field = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.4f, 1.0f, 0.0f));

	variable_value_field->text()
		.text(value ? "^gYes" : "^rNo")
		.text_align_center()
		.input_disable();

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
		.target(component_sheet->last_child())
		.bottom_outside()
		.left_inside();

	variable_name_label->velocity_position(2000.0f);
	variable_value_field->velocity_position(1000.0f);
	component_sheet->addChild(field_container);
}


void rynx::editor_rules::generate_menu_for_reflection(
	rynx::reflection::reflections& reflections_,
	const rynx::reflection::type& type_reflection,
	rynx::editor::rynx_common_info info,
	rynx::menu::Component* component_sheet_,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	for (auto&& member : type_reflection.m_fields) {
		if (member.m_type_name == "float") {
			rynx::editor::rynx_common_info field_info = info;
			++field_info.indent;
			field_float(member, field_info, component_sheet_, reflection_stack);
		}
		else if (member.m_type_name == "bool") {
			rynx::editor::rynx_common_info field_info = info;
			++field_info.indent;
			field_bool(member, field_info, component_sheet_, reflection_stack);
		}
		else if (reflections_.has(member.m_type_name)) {
			auto label = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.6f, 0.03f, 0.0f));
			label->text()
				.text(std::string(info.indent + 1, '-') + member.m_field_name + " (" + humanize(member.m_type_name) + ")")
				.text_align_left();

			label->align()
				.target(component_sheet_->last_child())
				.bottom_outside()
				.left_inside();

			label->velocity_position(100.0f);
			component_sheet_->addChild(label);

			for (auto&& tool : m_tools) {
				if (tool->operates_on(member.m_type_name)) {
					auto edit_button = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.6f, 0.03f, 0.0f));
					edit_button->align().target(component_sheet_->last_child()).bottom_outside().left_inside();
					edit_button->text().text(tool->get_tool_name());
					edit_button->on_click([this, info, member, tool_ptr = tool.get()]() {
						tool_ptr->source_data([=]() {
							int32_t mem_offset = info.cumulative_offset + member.m_memory_offset;
							void* value_ptr = rynx::editor::ecs_value_editor().compute_address(*info.ecs, info.entity_id, info.component_type_id, mem_offset);
							return value_ptr;
						});
						this->switch_to_tool(*tool_ptr);
					});
				}
			}

			rynx::editor::rynx_common_info field_info;
			field_info = info;
			field_info.cumulative_offset += member.m_memory_offset;
			++field_info.indent;

			reflection_stack.emplace_back(type_reflection, member);
			const auto& member_type_reflection = reflections_.get(member);
			generate_menu_for_reflection(reflections_, member_type_reflection, field_info, component_sheet_, reflection_stack);
			reflection_stack.pop_back();
		}
		else {
			auto label = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.6f, 0.03f, 0.0f));
			label->text()
				.text(std::string(info.indent + 1, '-') + member.m_field_name + " (" + humanize(member.m_type_name) + ")")
				.text_align_left();

			label->align()
				.target(component_sheet_->last_child())
				.bottom_outside()
				.left_inside();

			label->velocity_position(100.0f);
			component_sheet_->addChild(label);
		}
	}
}



void rynx::editor_rules::on_entity_selected(rynx::id id) {
	m_state.m_selected_ids.clear();
	m_state.m_selected_ids.emplace_back(id);

	rynx::ecs& ecs = m_context->get_resource<rynx::ecs>();
	rynx::graphics::GPUTextures& textures = m_context->get_resource<rynx::graphics::GPUTextures>();

	auto reflections_vec = ecs[id].reflections(m_reflections);
	m_components_list->clear_children();

	for (auto&& reflection_entry : reflections_vec) {
		auto component_sheet = std::make_shared<rynx::menu::Div>(rynx::vec3f(0.0f, 0.0f, 0.0f));
		component_sheet->set_background(frame_tex);
		component_sheet->set_dynamic_position_and_scale();
		m_components_list->addChild(component_sheet);

		{
			auto component_name = std::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.6f - 0.10f, 0.025f, 0.0f));
			component_name->text()
				.text(reflection_entry.m_type_name)
				.text_align_left()
				.font(m_font);

			component_name->align()
				.top_inside()
				.left_inside()
				.offset_kind_relative_to_self()
				.offset_y(-0.5f);

			component_name->velocity_position(100.0f);


			auto delete_component = std::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.10f, 0.025f, 0.0f));
			delete_component->text()
				.text("^yRemove")
				.text_align_center()
				.font(m_font);

			// align to right side of component name bar.
			delete_component->align()
				.target(component_name.get())
				.top_inside()
				.right_outside();

			delete_component->velocity_position(100.0f);

			delete_component->on_click([this, parent = component_sheet.get(), &ecs, id, type_index_v = reflection_entry.m_type_index_value]() {
				execute([=, &ecs]() {
					ecs.editor().removeFromEntity(id, type_index_v);
					parent->die();
				});
			});

			component_sheet->addChild(delete_component);
			component_sheet->addChild(component_name);

			for (auto&& tool : m_tools) {
				if (tool->operates_on(reflection_entry.m_type_name)) {
					auto edit_button = std::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.6f, 0.03f, 0.0f));
					edit_button->align().target(component_name.get()).bottom_outside().left_inside();
					edit_button->text().text(tool->get_tool_name());
					
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

		rynx::editor::rynx_common_info component_common_info;
		component_common_info.component_type_id = reflection_entry.m_type_index_value;
		component_common_info.ecs = &ecs;
		component_common_info.entity_id = id;
		component_common_info.textures = &textures;
		component_common_info.frame_tex = frame_tex;
		component_common_info.knob_tex = knob_tex;

		generate_menu_for_reflection(m_reflections, reflection_entry, component_common_info, component_sheet.get());
	}
}


void rynx::editor_rules::add_tool(std::unique_ptr<rynx::editor::itool> tool) {

	auto selection_tool_button = std::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(0.15f, 0.15f * 0.3f, 0.0f));
	selection_tool_button->respect_aspect_ratio();

	if (m_tools.empty()) {
		selection_tool_button->align().target(m_tools_bar.get()).top_left_inside().offset(-0.3f);
	}
	else {
		auto* lastComponent = m_tools_bar->last_child();
		selection_tool_button->align().target(lastComponent).top_inside().left_outside().offset_x(-0.3f);
	}

	selection_tool_button->velocity_position(100.0f);
	selection_tool_button->on_click([this, tool = tool.get()]() { switch_to_tool(*tool); });
	m_tools_bar->addChild(selection_tool_button);
	m_tools.emplace_back(std::move(tool));
}



rynx::editor_rules::editor_rules(
	rynx::scheduler::context& ctx,
	rynx::reflection::reflections& reflections,
	Font* font,
	rynx::menu::Component* editor_menu_host,
	rynx::graphics::GPUTextures& textures,
	rynx::mapped_input& gameInput)
	: m_reflections(reflections)
{
	m_context = &ctx;
	m_font = font;

	struct tool_api_impl : public rynx::editor::editor_shared_state::tool_api {
	private:
		editor_rules* m_host;
		
	public:
		tool_api_impl(editor_rules* host) : m_host(host) {}

		virtual void push_popup(std::shared_ptr<rynx::menu::Component> popup) override {
			m_host->push_popup(popup);
		}
		
		virtual void pop_popup() override {
			m_host->pop_popup();
		}
		
		virtual void enable_tools() override {
			m_host->enable_tools();
		}
		
		virtual void disable_tools() override {
			m_host->disable_tools();
		}

		virtual void activate_default_tool() override {
			auto& default_tool = m_host->get_default_tool();
			m_host->switch_to_tool(default_tool);
		}
		
		virtual void execute(std::function<void()> execute_in_main_thread) override {
			m_host->execute(std::move(execute_in_main_thread));
		}
	};

	m_state.m_editor = new tool_api_impl(this);

	m_editor_menu = std::make_shared<rynx::menu::Div>(rynx::vec3f{ 1, 1, 0 });
	editor_menu_host->addChild(m_editor_menu);

	this->frame_tex = textures.findTextureByName("frame");
	this->knob_tex = textures.findTextureByName("frame");

	// create editor menus
	{
		m_tools_bar = std::make_shared<rynx::menu::Div>(rynx::vec3f{ 0.3f, 1.0f, 0.0f });
		m_tools_bar->align().right_inside().offset(+0.9f);
		m_tools_bar->on_hover([ptr = m_tools_bar.get()](rynx::vec3f /* mousePos */ , bool inRect) {
			if (inRect) {
				ptr->align().offset(0.0f);
			}
			else {
				ptr->align().offset(+0.9f);
			}
			return inRect;
		});

		m_entity_bar = std::make_shared<rynx::menu::Div>(rynx::vec3f(0.3f, 1.0f, 0.0f));
		m_entity_bar->align().left_inside().offset(+0.9f);
		m_entity_bar->on_hover([ptr = m_entity_bar.get()](rynx::vec3f /* mousePos */, bool inRect) {
			if (inRect) {
				ptr->align().offset(0.0f);
			}
			else {
				ptr->align().offset(+0.9f);
			}
			return inRect;
		});

		m_editor_menu->addChild(m_tools_bar);
		m_editor_menu->addChild(m_entity_bar);

		m_tools_bar->set_background(frame_tex);
		m_entity_bar->set_background(frame_tex);

		// editor entity view menu
		{
			m_components_list = std::make_shared<rynx::menu::List>(frame_tex, rynx::vec3f(1.0f, 0.95f, 0.0f));
			m_components_list->list_endpoint_margin(0.05f);
			m_components_list->list_element_margin(0.05f);
			m_components_list->list_element_velocity(2500.0f);
			m_components_list->align().bottom_inside().left_inside();

			auto add_component_button = std::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(1.0f, 0.05f, 0.0f));
			add_component_button->align().top_inside().left_inside();
			add_component_button->text().text("Add component");
			add_component_button->on_click([this, &textures]() {
				execute([this, &textures]() {
					if (m_state.m_selected_ids.size() == 1) {
						rynx::ecs& ecs = m_context->get_resource<rynx::ecs>();
						if (ecs.exists(m_state.m_selected_ids.front())) {
							auto reflection_data = m_reflections.get_reflection_data();
							std::ranges::sort(reflection_data, [](auto& a, auto& b) {return a.first < b.first; });
							auto menu_list = std::make_shared<rynx::menu::List>(frame_tex, rynx::vec3f(0.3f, 0.7f, 0.0f));
							push_popup(menu_list);
							disable_tools();

							auto selected_entity = m_state.m_selected_ids.front().value;

							for (auto&& reflect : reflection_data) {
								printf("%s\n", reflect.first.c_str());
								auto& type_reflection = reflect.second;

								// only suggest components entity does not already have.
								if (ecs[selected_entity].has(type_reflection.m_type_index_value)) {
									continue;
								}

								auto button = std::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(1.f, 0.05f, 0.0f));
								menu_list->addChild(button);
								menu_list->list_element_velocity(100.0f);
								button->align().center_x();
								button->text().text(type_reflection.m_type_name);
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

										pop_popup();
										on_entity_selected(selected_entity);
										enable_tools();
										});
									});
							}

							// create menu list of reflections
							// list must get exclusive input
						}
					}
					});
				});

			m_entity_bar->addChild(m_components_list);
			m_entity_bar->addChild(add_component_button);
		}
	}

	m_state.m_on_entity_selected = [this](rynx::id id) {
		on_entity_selected(id);
	};

	add_tool<rynx::editor::tools::selection_tool>(ctx);
	// TODO: Tool construction can't require these inputs.
	// TODO: Move tool initialization inside the add_tool function.

	m_active_tool = m_tools[0].get();
	m_active_tool->on_tool_selected();
}