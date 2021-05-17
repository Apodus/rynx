
#pragma once

#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/ecs/id.hpp>
#include <rynx/graphics/texture/id.hpp>
#include <rynx/tech/reflection.hpp>
#include <rynx/tech/ecs.hpp>

#include <vector>
#include <functional>

namespace rynx {
	class ecs;
	namespace scheduler {
		class context;
	}
	namespace menu {
		class Component;
	}
	namespace graphics {
		class GPUTextures;
	}

	namespace editor {
		class itool;

		struct ecs_value_editor {
			template<typename T>
			T& access(rynx::ecs& ecs, rynx::ecs::id id, int32_t component_type_id, int32_t memoffset) {
				char* component_ptr = static_cast<char*>(ecs[id].get(component_type_id));
				return *reinterpret_cast<T*>(component_ptr + memoffset);
			}

			void* compute_address(rynx::ecs& ecs, rynx::ecs::id id, int32_t component_type_id, int32_t memoffset) {
				char* component_ptr = static_cast<char*>(ecs[id].get(component_type_id));
				return reinterpret_cast<void*>(component_ptr + memoffset);
			}
		};

		struct component_recursion_info_t {
			rynx::ecs* ecs = nullptr;
			const rynx::reflection::reflections* reflections = nullptr;
			rynx::menu::Component* component_sheet = nullptr;
			rynx::graphics::GPUTextures* textures = nullptr;

			// constant data per component
			rynx::id entity_id = 0;
			int32_t component_type_id = 0;

			// data that changes per field of component
			int32_t cumulative_offset = 0;

			// visualization info
			int32_t indent = 0;
			rynx::graphics::texture_id frame_tex;
			rynx::graphics::texture_id knob_tex;

			std::function<void(
				const rynx::reflection::type& type_reflection,
				rynx::editor::component_recursion_info_t info,
				std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>>)> gen_type_editor;
		};

		struct editor_shared_state {
			struct tool_api {
				virtual void push_popup(std::shared_ptr<rynx::menu::Component> popup) = 0;
				virtual void pop_popup() = 0;
				virtual void enable_tools() = 0;
				virtual void disable_tools() = 0;
				virtual void activate_default_tool() = 0;
				virtual rynx::scheduler::context* get_context() = 0;
				virtual void execute(std::function<void()> execute_in_main_thread) = 0;
				virtual void for_each_tool(std::function<void(itool*)> op) = 0;
			};

			tool_api* m_editor;

			std::vector<rynx::id> m_selected_ids;
			std::function<void(rynx::id)> m_on_entity_selected;
		};

		class itool {
		public:
			struct action {
				action() = default;
				action(bool activate_tool, std::string target_type, std::string name, std::function<void(rynx::scheduler::context*)> op)
					: activate_tool(activate_tool), target_type(std::move(target_type)), name(std::move(name)), m_action(std::move(op))
				{}

				bool activate_tool = true;
				std::string target_type;
				std::string name;
				std::function<void(rynx::scheduler::context*)> m_action;
			};

			virtual ~itool() { m_editor_state = nullptr; }
			virtual void update(rynx::scheduler::context& ctx) = 0;
			virtual void update_nofocus(rynx::scheduler::context& /* ctx */) {} // called even when this tool is not selected.
			virtual void on_tool_selected() = 0;
			virtual void on_tool_unselected() = 0;

			virtual void on_entity_component_removed(
				rynx::scheduler::context*,
				std::string /* componentTypeName */,
				rynx::ecs&,
				rynx::id) {}
			
			virtual void on_entity_component_added(
				rynx::scheduler::context*,
				std::string /* componentTypeName */,
				rynx::ecs&,
				rynx::id) {}

			virtual void on_entity_component_value_changed(
				rynx::scheduler::context*,
				std::string /* componentTypeName */,
				rynx::ecs&,
				rynx::id) {}

			virtual std::string get_info() { return {}; } // displayed in editor
			virtual std::string get_tool_name() = 0;
			virtual std::string get_button_texture() = 0;

			virtual bool try_generate_menu(
				rynx::reflection::field type,
				rynx::editor::component_recursion_info_t info,
				std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> reflection_stack)
			{
				return false;
			}

			virtual bool operates_on(const std::string& type_name) = 0;
			virtual bool allow_component_remove(const std::string& /* type_name */) { return true; }

			// TODO: Wrap under some common namespace or struct
			void source_data(std::function<void* ()> func);
			void source_data_cb(std::function<void(void*)> func);
			void* address_of_operand();
			void operand_value_changed(void* ptr) { if (m_data_changed) { m_data_changed(ptr); } }

			std::vector<std::string> get_editor_actions_list();
			bool editor_action_execute(std::string actionName, rynx::scheduler::context* ctx);

			rynx::id selected_id() const;
			const std::vector<rynx::id>& selected_id_vector() const;

			void execute(std::function<void()> op) { m_editor_state->m_editor->execute(std::move(op)); }
			void for_each_tool(std::function<void(itool*)> op) { m_editor_state->m_editor->for_each_tool(op); }

			editor_shared_state* m_editor_state = nullptr;

		protected:
			void define_action(std::string target_type, std::string name, std::function<void(rynx::scheduler::context*)> op);
			void define_action_no_tool_activate(std::string target_type, std::string name, std::function<void(rynx::scheduler::context*)> op);

		private:
			std::function<void* ()> m_get_data;
			std::function<void(void*)> m_data_changed;
			rynx::unordered_map<std::string, action> m_actions;
		};
	}
}