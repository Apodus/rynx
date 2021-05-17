
#pragma once

#include <rynx/editor/tools/tool.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/input/mapped_input.hpp>
#include <rynx/math/geometry/plane.hpp>

#include <rynx/tech/components.hpp>

namespace rynx {
	namespace editor {
		namespace tools {
			class selection_tool : public itool {
			public:
				selection_tool(rynx::scheduler::context& ctx);

				virtual void update(rynx::scheduler::context& ctx) override;
				virtual void on_tool_selected() override {}
				virtual void on_tool_unselected() override {}

				virtual std::string get_info() override {
					if (m_mode == Mode::IdField_Pick)
						return "Pick entity";
					if (m_mode == Mode::Vec3fDrag)
						return "Drag vec3f";
					return {};
				}

				virtual std::string get_tool_name() {
					return "selection";
				}

				virtual std::string get_button_texture() override {
					return "selection_tool";
				}

				virtual bool operates_on(const std::string& type_name) override;
				virtual bool try_generate_menu(
					rynx::reflection::field type,
					rynx::editor::component_recursion_info_t info,
					std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack) override;

			private:
				enum class Mode {
					Default,
					IdField_Pick,
					Vec3fDrag
				};

				Mode m_mode = Mode::Default;
				rynx::ecs::id m_mode_local_space_id;

				std::pair<rynx::ecs::id, float> find_nearest_entity(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos);

				std::function<void()> m_run_on_main_thread;
				rynx::key::logical m_activation_key;

				struct drag_op_t {
					rynx::vec3f start_point;
					rynx::vec3f prev_point;
					rynx::vec3f middle_point;
					bool active = false;
					float previous_angle = 0;
				};

				drag_op_t m_drag;
			};
		}
	}
}