
#pragma once

#include <rynx/editor/tools/tool.hpp>
#include <rynx/application/components.hpp>

namespace rynx {
	namespace graphics {
		class GPUTextures;
	}
	class ecs;

	namespace editor {
		namespace tools {

			class EditorDLL mesh_tool : public itool {
			public:
				mesh_tool(rynx::scheduler::context& ctx);

				virtual void update(rynx::scheduler::context& ctx) override;
				virtual void on_tool_selected() override;
				virtual void on_tool_unselected() override {}
				virtual void verify(rynx::scheduler::context&, error_emitter&) override {}

				virtual bool try_generate_menu(
					rynx::reflection::field field_type,
					rynx::editor::component_recursion_info_t info,
					std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack) override;

				virtual bool operates_on(rynx::type_id_t type_id) override {
					return type_id == rynx::type_index::id<rynx::components::graphics::mesh>();
				}

				virtual rynx::string get_tool_name() override {
					return "mesh tool";
				}

				virtual rynx::string get_button_texture() override {
					return "mesh_tool";
				}

			private:
				rynx::graphics::GPUTextures* m_textures;
				rynx::ecs& m_ecs;
			};

		}
	}
}