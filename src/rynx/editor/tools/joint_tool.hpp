
#pragma once

#include <rynx/editor/tools/tool.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/input/mapped_input.hpp>
#include <rynx/math/geometry/plane.hpp>

#include <rynx/application/components.hpp>
#include <rynx/tech/components.hpp>
#include <rynx/system/typeid.hpp>

namespace rynx {
	namespace editor {
		namespace tools {
			class EditorDLL joint_tool : public itool {
			public:
				joint_tool(rynx::scheduler::context& ctx);

				virtual void update(rynx::scheduler::context& ctx) override;
				virtual void update_nofocus(rynx::scheduler::context& ctx) override;

				virtual void on_tool_selected() override {}
				virtual void on_tool_unselected() override {}
				virtual void verify(rynx::scheduler::context& ctx, error_emitter& emitter) override;
				
				virtual rynx::string get_info() override {
					return {};
				}

				virtual rynx::string get_tool_name() {
					return "joint";
				}

				virtual rynx::string get_button_texture() override {
					return "joint_tool";
				}

				virtual bool operates_on(rynx::type_id_t type_id) override {
					return type_id == rynx::type_index::id<rynx::components::phys::joint>() ||
						type_id == rynx::type_index::id<rynx::components::phys::joint::connector_type>();
				}

				virtual bool try_generate_menu(
					rynx::reflection::field type,
					rynx::editor::component_recursion_info_t info,
					std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack) override;
			
				virtual void on_entity_component_value_changed(
					rynx::scheduler::context* ctx,
					rynx::type_index::type_id_t type_id,
					rynx::ecs& ecs,
					rynx::id id) override;



			private:
				
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