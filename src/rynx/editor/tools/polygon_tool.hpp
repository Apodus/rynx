
#pragma once

#include <rynx/editor/tools/tool.hpp>
#include <rynx/editor/tools/selection_tool.hpp>

namespace rynx {
	class collision_detection;
	
	namespace graphics {
		class mesh_collection;
	}

	namespace editor {
		namespace tools {

			class EditorDLL polygon_tool : public itool {
			public:
				polygon_tool(rynx::scheduler::context& ctx);
				virtual void update(rynx::scheduler::context& ctx) override;
				virtual void verify(rynx::scheduler::context&, error_emitter&) override {}

				virtual void on_tool_selected() override {}
				virtual void on_tool_unselected() override {
					m_drag_action_active = false;
					m_selected_vertex = -1;
				}

				virtual bool operates_on(rynx::type_id_t type_id) override {
					return type_id == rynx::type_index::id<rynx::components::phys::boundary>();
				}

				virtual rynx::string get_tool_name() override {
					return "polygon editor";
				}

				virtual rynx::string get_button_texture() override {
					return "polygon_tool";
				}

				virtual void on_entity_component_added(
					rynx::scheduler::context* ctx,
					rynx::type_index::type_id_t componentTypeName,
					rynx::ecs& ecs,
					rynx::id id) override;

				virtual void on_entity_component_removed(
					rynx::scheduler::context* ctx,
					rynx::type_index::type_id_t componentTypeName,
					rynx::ecs& ecs,
					rynx::id id) override;
				
				virtual void on_entity_component_value_changed(
					rynx::scheduler::context* ctx,
					rynx::type_index::type_id_t type_id,
					rynx::ecs& ecs,
					rynx::id id) override;

			private:
				bool vertex_create(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos, float allowedDistance);
				int32_t vertex_select(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos, float allowedDistance);
				void drag_operation_update(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos, float activationDistance);
				void drag_operation_start(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos);
				void drag_operation_end(rynx::ecs& game_ecs, rynx::collision_detection& detection);

				void action_smooth(rynx::ecs& ecs);
				void action_rebuild_mesh(rynx::ecs& ecs, rynx::graphics::mesh_collection& meshes);
				void action_rebuild_boundary_mesh(rynx::ecs& ecs, rynx::graphics::mesh_collection& meshes);

				int32_t m_selected_vertex = -1; // -1 is none, otherwise this is an index to polygon vertex array.
				rynx::key::logical m_activation_key;
				rynx::key::logical m_secondary_activation_key;

				rynx::key::logical m_key_smooth;

				rynx::vec3f m_drag_action_mouse_origin;
				rynx::vec3f m_drag_action_object_origin;
				bool m_drag_action_active = false;
			};

		}
	}
}