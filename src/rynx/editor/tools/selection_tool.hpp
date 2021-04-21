
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
				selection_tool(rynx::scheduler::context& ctx) {
					auto& input = ctx.get_resource<rynx::mapped_input>();
					m_activation_key = input.generateAndBindGameKey(input.getMouseKeyPhysical(0), "selection tool activate");
				}

				virtual void update(rynx::scheduler::context& ctx) override;
				virtual void on_tool_selected() override {}
				virtual void on_tool_unselected() override {}

				virtual std::string get_tool_name() {
					return "selection tool";
				}

				virtual std::string get_button_texture() override {
					return "selection_tool";
				}

				virtual bool operates_on(const std::string& /* type_name */) override {
					return false;
				}

			private:
				void on_key_press(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos);

				std::function<void()> m_run_on_main_thread;
				rynx::key::logical m_activation_key;
			};
		}
	}
}