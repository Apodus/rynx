#include <rynx/editor/tools/texture_selection_tool.hpp>
#include <rynx/application/components.hpp>
#include <rynx/menu/Button.hpp>
#include <rynx/menu/List.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>

rynx::editor::tools::texture_selection::texture_selection(rynx::scheduler::context& ctx)
	: m_ecs(ctx.get_resource<rynx::ecs>())
{
	m_textures = &ctx.get_resource<rynx::graphics::GPUTextures>();
}

void rynx::editor::tools::texture_selection::update(rynx::scheduler::context& ctx) {}

void rynx::editor::tools::texture_selection::on_tool_selected() {
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
}
