
#pragma once

#include <rynx/editor/tools/tool.hpp>
#include <rynx/editor/tools/selection_tool.hpp>

namespace rynx {
	namespace graphics {
		class GPUTextures;
	}

	namespace editor {
		namespace tools {

			class texture_selection : public itool {
			public:
				texture_selection(rynx::scheduler::context& ctx);

				virtual void update(rynx::scheduler::context& ctx) override;
				virtual void on_tool_selected() override;
				virtual void on_tool_unselected() override {}

				virtual bool operates_on(const std::string& type_name) override {
					return type_name.find("rynx::components::texture") != std::string::npos;
				}

				virtual std::string get_tool_name() override {
					return "select texture";
				}

				virtual std::string get_button_texture() override {
					return "frame";
				}

			private:
				rynx::graphics::GPUTextures* m_textures;
				rynx::ecs& m_ecs;
			};

		}
	}
}