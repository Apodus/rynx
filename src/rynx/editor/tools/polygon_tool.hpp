
#pragma once

#include <rynx/editor/tools/tool.hpp>
#include <rynx/editor/tools/selection_tool.hpp>

namespace rynx {
	class collision_detection;
	
	namespace editor {
		namespace tools {

			class polygon_tool : public itool {
			public:
				polygon_tool(rynx::scheduler::context& ctx) {
					auto& input = ctx.get_resource<rynx::mapped_input>();
					m_activation_key = input.generateAndBindGameKey(input.getMouseKeyPhysical(0), "polygon tool activate");
					m_secondary_activation_key = input.generateAndBindGameKey(input.getMouseKeyPhysical(1), "polygon tool activate");
					m_key_smooth = input.generateAndBindGameKey(',', "polygon smooth op");
				}

				virtual void update(rynx::scheduler::context& ctx) override;

				virtual void on_tool_selected() override {}

				virtual void on_tool_unselected() override {
					m_drag_action_active = false;
					m_selected_vertex = -1;
				}

				virtual bool operates_on(const std::string& type_name) override {
					return type_name.find("rynx::components::boundary") != std::string::npos;
				}

				virtual std::string get_tool_name() override {
					return "polygon editor";
				}

				virtual std::string get_button_texture() override {
					return "frame";
				}

			private:
				bool vertex_create(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos);
				int32_t vertex_select(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos);
				void drag_operation_start(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos);
				void drag_operation_update(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos);
				void drag_operation_end(rynx::ecs& game_ecs, rynx::collision_detection& detection);

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