
#pragma once

#include <rynx/editor/tools/tool.hpp>

namespace rynx {
	class ecs;

	namespace editor {
		namespace tools {

			class collisions_tool : public itool {
			public:
				collisions_tool(rynx::scheduler::context& ctx);

				virtual void update(rynx::scheduler::context& ctx) override;
				virtual void on_tool_selected() override {}
				virtual void on_tool_unselected() override {}
				virtual void verify(rynx::scheduler::context& ctx, error_emitter& emitter) override;

				virtual bool operates_on(const std::string& type_name) override {
					return type_name.find("rynx::components::collision") != std::string::npos;
				}

				virtual void on_entity_component_removed(
					rynx::scheduler::context* ctx,
					std::string componentTypeName,
					rynx::ecs& ecs,
					rynx::id id) override;

				// default removing collision type component is not ok,
				// need to know how to inform collision detection of the change.
				virtual bool allow_component_remove(const std::string& type_name) override { return !operates_on(type_name); }

				virtual std::string get_tool_name() override {
					return "collisions";
				}

				virtual std::string get_button_texture() override {
					return "collisions_tool";
				}

			private:
				rynx::ecs& m_ecs;
			};

		}
	}
}
