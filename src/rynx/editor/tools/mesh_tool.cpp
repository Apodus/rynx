#include <rynx/editor/tools/mesh_tool.hpp>
#include <rynx/application/components.hpp>
#include <rynx/menu/Button.hpp>
#include <rynx/menu/List.hpp>
#include <rynx/menu/Slider.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/mesh/collection.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/system/typeid.hpp>

rynx::editor::tools::mesh_tool::mesh_tool(rynx::scheduler::context& ctx)
	: m_ecs(ctx.get_resource<rynx::ecs>())
{
	m_textures = &ctx.get_resource<rynx::graphics::GPUTextures>();

	define_action_no_tool_activate(rynx::type_index::id<rynx::components::graphics::mesh>(), "select mesh", [this](rynx::scheduler::context* ctx) {
		m_editor_state->m_editor->disable_tools();
		m_editor_state->m_editor->execute([this, ctx]() {
			void* ptr = this->address_of_operand();
			if (!ptr) {
				return;
			}

			rynx::components::graphics::mesh* ptr_mesh = reinterpret_cast<rynx::components::graphics::mesh*>(ptr);

			auto frame_tex = m_textures->findTextureByName("frame");
			auto mesh_list = rynx::make_shared<rynx::menu::List>(frame_tex, rynx::vec3f(0.3f, 0.7f, 0.0f));
			mesh_list->list_element_velocity(150.0f);

			auto& mesh_collection = ctx->get_resource<rynx::graphics::mesh_collection>();
			auto meshes = mesh_collection.getListOfMeshes();
			for (auto&& entry : meshes) {
				auto button = rynx::make_shared<rynx::menu::Button>(frame_tex, rynx::vec3f(1.f, 0.05f, 0.0f));
				button->text().text(entry.name);
				button->on_click([this, ptr_mesh, mesh_entry = entry]() {
					m_editor_state->m_editor->execute([this, ptr_mesh, mesh_entry]() {
						
						// have to check if the texture component we are changing is a direct component
						// before changing the texture id component, because texture could be a member field
						// in some other component.
						auto entity_id = selected_id();
						if (m_ecs[entity_id].try_get<const rynx::components::graphics::mesh>() == ptr_mesh) {
							if (m_ecs[entity_id].has<rynx::graphics::mesh_id>())
								m_ecs.removeFromEntity<rynx::components::graphics::mesh, rynx::graphics::mesh_id>(entity_id);
							else
								m_ecs.removeFromEntity<rynx::components::graphics::mesh>(entity_id);
							
							m_ecs.attachToEntity(entity_id, rynx::components::graphics::mesh{ mesh_entry.id });
						}
						else {
							ptr_mesh->m = mesh_entry.id;
						}

						m_editor_state->m_editor->pop_popup();
						m_editor_state->m_editor->enable_tools();
						m_editor_state->m_editor->activate_default_tool();
					});
				});

				mesh_list->addChild(button);
			}

			m_editor_state->m_editor->push_popup(mesh_list);
		});
	});
}

bool rynx::editor::tools::mesh_tool::try_generate_menu(
	rynx::reflection::field field_type,
	rynx::editor::component_recursion_info_t info,
	std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
{
	rynx::reflection::type type = info.reflections->get(field_type);
	if (info.component_type_id == rynx::type_index::id<rynx::components::graphics::mesh>())
	{
		return false;
	}
	
	return false;
}

void rynx::editor::tools::mesh_tool::update(rynx::scheduler::context&) {}

void rynx::editor::tools::mesh_tool::on_tool_selected() {

}
