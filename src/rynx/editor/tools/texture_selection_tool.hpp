
#pragma once

#include <rynx/editor/tools/tool.hpp>

namespace rynx {
	namespace graphics {
		class GPUTextures;
	}
	class ecs;

	namespace editor {
		namespace tools {

			class texture_selection : public itool {
			public:
				texture_selection(rynx::scheduler::context& ctx);

				virtual void update(rynx::scheduler::context& ctx) override;
				virtual void on_tool_selected() override;
				virtual void on_tool_unselected() override {}

				virtual bool try_generate_menu(
					rynx::reflection::field field_type,
					rynx::editor::component_recursion_info_t info,
					std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack) override;

				virtual bool operates_on(const std::string& type_name) override {
					return type_name.find("rynx::components::texture") != std::string::npos;
				}

				virtual std::string get_tool_name() override {
					return "select texture";
				}

				virtual std::string get_button_texture() override {
					return "texture_tool";
				}

			private:
				rynx::graphics::GPUTextures* m_textures;
				rynx::ecs& m_ecs;
			};

		}
	}
}