#pragma once

#include <rynx/editor/tools/tool.hpp>

namespace rynx {
	class collision_detection;

	namespace graphics {
		class mesh_collection;
	}

	namespace editor {
		namespace tools {

			class EditorDLL subscene_tool : public itool {
			public:
				subscene_tool(rynx::scheduler::context& ctx);
				virtual void update(rynx::scheduler::context& ctx) override;
				virtual void verify(rynx::scheduler::context&, error_emitter&) override;

				virtual void on_tool_selected() override;
				virtual void on_tool_unselected() override;

				virtual bool operates_on(rynx::type_id_t) override;

				virtual rynx::string get_tool_name() override;
				virtual rynx::string get_button_texture() override;

				virtual void on_entity_component_added(
					rynx::scheduler::context* ctx,
					rynx::type_index::type_id_t componentType,
					rynx::ecs& ecs,
					rynx::id id) override;

				virtual void on_entity_component_removed(
					rynx::scheduler::context* ctx,
					rynx::type_index::type_id_t componentType,
					rynx::ecs& ecs,
					rynx::id id) override;

				virtual void on_entity_component_value_changed(
					rynx::scheduler::context* ctx,
					rynx::type_index::type_id_t type_id,
					rynx::ecs& ecs,
					rynx::id id) override;

				void gen_menu_field_scenelink(
					const rynx::reflection::field& member,
					rynx::editor::component_recursion_info_t info,
					std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack);

				virtual bool try_generate_menu(
					rynx::reflection::field type,
					rynx::editor::component_recursion_info_t info,
					std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
				{
					if (type.m_type_name.find("rynx::scene_id") != rynx::string::npos) {
						gen_menu_field_scenelink(type, info, reflection_stack);
						return true;
					}
					return false;
				}
			};

		}
	}
}