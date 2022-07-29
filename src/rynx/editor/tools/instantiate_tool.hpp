
#pragma once

#include <rynx/editor/tools/tool.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/input/mapped_input.hpp>
#include <rynx/math/geometry/plane.hpp>
#include <rynx/tech/components.hpp>
#include <rynx/graphics/texture/id.hpp>

namespace rynx {
	class scenes;
	namespace filesystem {
		class vfs;
	}
	namespace editor {
		namespace tools {
			class EditorDLL instantiation_tool : public itool {
			public:
				instantiation_tool(rynx::scheduler::context& ctx);

				virtual void update(rynx::scheduler::context& ctx) override;
				virtual void on_tool_selected() override;
				virtual void on_tool_unselected() override {}
				virtual void verify(rynx::scheduler::context&, error_emitter&) override {}

				virtual rynx::string get_tool_name() {
					return "instantiation tool";
				}

				virtual rynx::string get_button_texture() override {
					return "instantiation_tool";
				}

				virtual bool operates_on(const rynx::string& /* type_name */) override {
					return false;
				}

			private:
				void on_key_press(rynx::ecs& game_ecs, rynx::vec3f cursorWorldPos);
				
				// Todo: loading from file is not ideal. Could keep at least some scenes in memory?
				// Todo: keep unordered map of scene path -> ecs instance in editor state?
				rynx::string m_selectedScene;

				rynx::graphics::texture_id m_frame_tex_id;
				rynx::observer_ptr<rynx::filesystem::vfs> m_vfs;
				rynx::observer_ptr<rynx::scenes> m_scenes;
			};
		}
	}
}