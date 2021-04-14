
#pragma once

#include <rynx/tech/ecs/id.hpp>
#include <vector>
#include <functional>

namespace rynx {
	namespace scheduler {
		class context;
	}
	namespace menu {
		class Component;
	}

	namespace editor {
		struct editor_shared_state {
			struct tool_api {
				virtual void push_popup(std::shared_ptr<rynx::menu::Component> popup) = 0;
				virtual void pop_popup() = 0;
				virtual void enable_tools() = 0;
				virtual void disable_tools() = 0;
				virtual void activate_default_tool() = 0;
				virtual void execute(std::function<void()> execute_in_main_thread) = 0;
			};

			tool_api* m_editor;

			std::vector<rynx::id> m_selected_ids;
			std::function<void(rynx::id)> m_on_entity_selected;
		};

		class itool {
		public:
			virtual ~itool() { m_editor_state = nullptr; }
			virtual void update(rynx::scheduler::context& ctx) = 0;
			virtual void on_tool_selected() = 0;
			virtual void on_tool_unselected() = 0;

			virtual std::string get_tool_name() = 0;
			virtual std::string get_button_texture() = 0;

			void source_data(std::function<void* ()> func) {
				m_get_data = std::move(func);
			}

			void* address_of_operand() {
				if (m_get_data)
					return m_get_data();
				return nullptr;
			}
			
			virtual bool operates_on(const std::string& type_name) = 0;

			rynx::id selected_id() const {
				if (m_editor_state->m_selected_ids.size() == 1) {
					return m_editor_state->m_selected_ids.front();
				}
				return rynx::id{ 0 };
			}

			editor_shared_state* m_editor_state = nullptr;

		private:
			std::function<void* ()> m_get_data;
		};
	}
}