
#pragma once

#include <rynx/tech/unordered_map.hpp>
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
			struct action {
				action() = default;
				action(bool activate_tool, std::string name, std::function<void(rynx::scheduler::context*)> op)
					: activate_tool(activate_tool), name(std::move(name)), m_action(std::move(op))
				{}

				bool activate_tool = true;
				std::string name;
				std::function<void(rynx::scheduler::context*)> m_action;
			};

			virtual ~itool() { m_editor_state = nullptr; }
			virtual void update(rynx::scheduler::context& ctx) = 0;
			virtual void on_tool_selected() = 0;
			virtual void on_tool_unselected() = 0;

			virtual std::string get_tool_name() = 0;
			virtual std::string get_button_texture() = 0;

			virtual bool operates_on(const std::string& type_name) = 0;
			virtual bool allow_component_remove(const std::string& type_name) { return true; }

			void source_data(std::function<void* ()> func);
			void* address_of_operand();

			std::vector<std::string> get_editor_actions_list();
			bool editor_action_execute(std::string actionName, rynx::scheduler::context* ctx);

			rynx::id selected_id() const;

			editor_shared_state* m_editor_state = nullptr;

		protected:
			void define_action(std::string name, std::function<void(rynx::scheduler::context*)> op);
			void define_action_no_tool_activate(std::string name, std::function<void(rynx::scheduler::context*)> op);

		private:
			std::function<void* ()> m_get_data;
			rynx::unordered_map<std::string, action> m_actions;
		};
	}
}