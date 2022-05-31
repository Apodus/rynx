#include <rynx/editor/tools/texture_selection_tool.hpp>
#include <rynx/application/components.hpp>
#include <rynx/menu/Button.hpp>
#include <rynx/menu/List.hpp>
#include <rynx/menu/Slider.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/system/typeid.hpp>

#include <iostream>

rynx::editor::tools::texture_selection::texture_selection(rynx::scheduler::context& ctx)
	: m_ecs(ctx.get_resource<rynx::ecs>())
{
	m_textures = &ctx.get_resource<rynx::graphics::GPUTextures>();

	define_action_no_tool_activate("", "select texture", [this]([[maybe_unused]]rynx::scheduler::context* ctx) {
		m_editor_state->m_editor->disable_tools();
		m_editor_state->m_editor->execute([this]() {
			void* ptr = this->address_of_operand();
			if (!ptr) {
				return;
			}

			rynx::components::texture* ptr_tex = reinterpret_cast<rynx::components::texture*>(ptr);

			auto frame_tex = m_textures->findTextureByName("frame");
			auto texture_list = std::make_shared<rynx::menu::List>(frame_tex, rynx::vec3f(0.3f, 0.7f, 0.0f));
			texture_list->list_element_velocity(150.0f);

			auto tex = m_textures->getListOfTextures();
			for (auto&& entry : tex) {
				if (!entry.info.in_atlas) {
					continue;
				}

				auto button = std::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(1.f, 0.05f, 0.0f));
				button->text().text(entry.info.name);
				button->on_click([this, ptr_tex, tex_name = entry.info.name, tex_id = entry.tex_id]() {
					m_editor_state->m_editor->execute([this, ptr_tex, tex_name, tex_id]() {
						ptr_tex->textureName = tex_name;

						// have to check if the texture component we are changing is a direct component
						// before changing the texture id component, because texture could be a member field
						// in some other component.
						auto entity_id = selected_id();
						if (m_ecs[entity_id].try_get<rynx::components::texture>() == ptr_tex) {
							auto* tex_id_ptr = m_ecs[entity_id].try_get<rynx::graphics::texture_id>();
							if (tex_id_ptr) {
								*tex_id_ptr = tex_id;
							}
						}

						m_editor_state->m_editor->pop_popup();
						m_editor_state->m_editor->enable_tools();
						m_editor_state->m_editor->activate_default_tool();
					});
				});

				texture_list->addChild(button);
			}

			m_editor_state->m_editor->push_popup(texture_list);
		});
	});
}

bool rynx::editor::tools::texture_selection::try_generate_menu(
	rynx::reflection::field field_type,
	rynx::editor::component_recursion_info_t info,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	rynx::reflection::type type = info.reflections->get(field_type);
	if (info.component_type_id == info.ecs->get_type_index().id<rynx::components::texture>())
	{
		std::cout << "reflection stack size: " << reflection_stack.size() << std::endl;

		auto field_div = std::make_shared<rynx::menu::Div>(rynx::vec3f(0.6f, 0.06f, 0.0f));
		field_div->velocity_position(200.0f); // TODO

		{
			auto* tex_id = (*info.ecs)[info.entity_id].try_get<rynx::graphics::texture_id>();
			if (!tex_id) {
				std::cout << "oh no tex not here" << std::endl;
				return false;
			}

			auto scale_text = std::make_shared<rynx::menu::Text>(rynx::vec3f(0.1f, 0.5f, 0.0f));
			scale_text->text("scale");
			scale_text->text_align_left();
			scale_text->align().left_inside().top_inside().offset_x(-0.3f);
			scale_text->velocity_position(200.0f);

			auto variable_value_field = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.20f, 0.5f, 0.0f));
			variable_value_field->velocity_position(200.0f); // TODO
			float current_value = info.textures->get_tex_config(*tex_id).vertex_position_scale;
			variable_value_field->text().text(std::to_string(current_value));
			variable_value_field->text().text_input_enable();
			variable_value_field->align().target(scale_text.get()).right_outside().top_inside();
			variable_value_field->text().on_value_changed([info, self = variable_value_field.get()](std::string s) {
				auto* tex_id = (*info.ecs)[info.entity_id].try_get<rynx::graphics::texture_id>();
				if (!tex_id)
					return;
				auto tex_id_v = *tex_id;
				
				float new_v = std::stof(s);
				auto conf = info.textures->get_tex_config(tex_id_v);
				conf.vertex_position_scale = new_v;
				info.textures->set_tex_config(tex_id_v, conf);
				self->text().text(std::to_string(new_v));
			});

			auto value_slider = std::make_shared<rynx::menu::SlideBarVertical>(info.frame_tex, info.frame_tex, rynx::vec3f(0.2f, 0.5f, 0.0f), -1.0f, +1.0f);
			value_slider->align().target(variable_value_field.get()).right_outside().top_inside();
			value_slider->velocity_position(200.0f);
			value_slider->setValue(0);
			value_slider->on_active_tick([info, self = value_slider.get(), text_element = variable_value_field.get()](float /* input_v */, float dt) {
				auto* tex_id = (*info.ecs)[info.entity_id].try_get<rynx::graphics::texture_id>();
				if (!tex_id)
					return;
				auto tex_id_v = *tex_id;

				auto conf = info.textures->get_tex_config(tex_id_v);
				float& v = conf.vertex_position_scale;
				
				float input_v = self->getValueCubic_AroundCenter();
				float tmp = v + dt * input_v;
				constexpr float value_modify_velocity = 3.0f;
				if (input_v * v > 0) {
					tmp *= (1.0f + std::fabs(input_v) * value_modify_velocity * dt);
				}
				else {
					tmp *= 1.0f / (1.0f + std::fabs(input_v) * value_modify_velocity * dt);
				}
				v = std::clamp(tmp, 0.0f, 100000.0f);
				text_element->text().text(std::to_string(v));
				
				info.textures->set_tex_config(tex_id_v, conf);
			});

			value_slider->on_drag_end([self = value_slider.get()](float /* v */) {
				self->setValue(0);
			});

			auto toggle_vertex_pos_as_uv = std::make_shared<rynx::menu::Button>(info.frame_tex, rynx::vec3f(0.70f, 0.5f, 0.0f));
			toggle_vertex_pos_as_uv->velocity_position(200.0f); // TODO
			bool is_pos_as_uv = info.textures->get_tex_config(*tex_id).vertex_positions_as_uv;
			toggle_vertex_pos_as_uv->text().text(is_pos_as_uv ? "vertex pos as uv" : "uv from attributes");
			toggle_vertex_pos_as_uv->align().target(scale_text.get()).left_inside().bottom_outside();
			toggle_vertex_pos_as_uv->on_click([info, self = toggle_vertex_pos_as_uv.get()]() {
				auto* tex_id = (*info.ecs)[info.entity_id].try_get<rynx::graphics::texture_id>();
				if (!tex_id)
					return;
				auto tex_id_v = *tex_id;

				auto conf = info.textures->get_tex_config(tex_id_v);
				conf.vertex_positions_as_uv = !conf.vertex_positions_as_uv;
				info.textures->set_tex_config(tex_id_v, conf);
				self->text().text(conf.vertex_positions_as_uv ? "vertex pos as uv" : "uv from attributes");
			});

			field_div->addChild(scale_text);
			field_div->addChild(variable_value_field);
			field_div->addChild(value_slider);
			field_div->addChild(toggle_vertex_pos_as_uv);
		}

		field_div->align()
			.target(info.component_sheet->last_child())
			.bottom_outside()
			.left_inside();

		info.component_sheet->addChild(field_div);
		return true;
	}
	
	return false;
}

void rynx::editor::tools::texture_selection::update(rynx::scheduler::context&) {}

void rynx::editor::tools::texture_selection::on_tool_selected() {

}
