
#pragma once

#include <rynx/ecs/ecs.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>

#include <rynx/menu/Div.hpp>
#include <rynx/menu/Text.hpp>
#include <rynx/menu/Button.hpp>
#include <rynx/menu/Slider.hpp>
#include <rynx/menu/List.hpp>

#include <rynx/application/logic.hpp>
#include <rynx/input/key_types.hpp>
#include <rynx/tech/collision_detection.hpp>

#include <rynx/editor/tools/tool.hpp>

#include <rynx/math/geometry/ray.hpp>

namespace rynx {
	namespace editor {
		void field_float(
			const rynx::reflection::field& member,
			rynx::editor::component_recursion_info_t info,
			std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>>
		);

		void field_bool(
			const rynx::reflection::field& member,
			rynx::editor::component_recursion_info_t info,
			std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>>
		);

		struct ipromise {
			virtual ~ipromise() {}
			virtual bool ready() const = 0;
			virtual void execute() = 0;
		};
	}

	class EditorDLL editor_rules : public rynx::application::logic::iruleset {
	private:
		rynx::editor::editor_shared_state m_state;

		// TODO: Get rid of these
		rynx::graphics::texture_id frame_tex;
		rynx::graphics::texture_id knob_tex;

		rynx::shared_ptr<rynx::menu::Div> m_editor_menu; // editor root level menu container.
		
		// TODO: Maybe wrap the editor menus inside a class?
		rynx::shared_ptr<rynx::menu::Div> m_entity_bar; // left side menu, containing info of selection
		rynx::shared_ptr<rynx::menu::List> m_components_list; // entity bar component list view - showing all components of an entity.

		rynx::shared_ptr<rynx::menu::Div> m_tools_bar; // right side menu, containing info of tools
		rynx::shared_ptr<rynx::menu::Div> m_file_actions_bar; // bottom side menu, containing global actions (new empty scene, save scene, load scene)
		
		rynx::shared_ptr<rynx::menu::Div> m_top_bar; // top side menu, contains run/pause, editor on/off, ..?
		rynx::shared_ptr<rynx::menu::Text> m_info_text;

		rynx::binary_config::id m_game_running_state;

		std::vector<rynx::shared_ptr<rynx::menu::Component>> m_popups;
		
		std::vector<rynx::function<void()>> m_execute_in_main_stack;
		std::vector<rynx::unique_ptr<rynx::editor::ipromise>> m_long_ops;

		rynx::editor::itool* m_active_tool = nullptr;
		std::vector<rynx::unique_ptr<rynx::editor::itool>> m_tools;
		std::vector<rynx::string> m_component_filters; // if any matches, component can be included

		rynx::reflection::reflections& m_reflections;
		rynx::scheduler::context* m_context = nullptr;
		
		Font* m_font = nullptr;
		bool m_tools_enabled = true;

		void push_popup(rynx::shared_ptr<rynx::menu::Component> popup) {
			execute([this, popup]() {
				if (!popup->parent()) {
					m_editor_menu->addChild(popup);
				}
				popup->capture_dedicated_mouse_input();
				popup->capture_dedicated_keyboard_input();
				m_popups.emplace_back(popup);
			});
		}

		void pop_popup() {
			if (!m_popups.empty()) {
				m_popups.back()->release_dedicated_keyboard_input();
				m_popups.back()->release_dedicated_mouse_input();
				m_popups.back()->die();
				m_popups.pop_back();
				if (!m_popups.empty()) {
					m_popups.back()->capture_dedicated_keyboard_input();
					m_popups.back()->capture_dedicated_mouse_input();
				}
			}
		}

		void disable_tools() {
			m_tools_enabled = false;
		}

		void enable_tools() {
			m_tools_enabled = true;
		}

		void display_list_dialog(
			std::vector<rynx::string> entries,
			rynx::function<rynx::floats4(rynx::string)> entryColor,
			rynx::function<void(rynx::string)> on_selection);

		template<typename Func>
		void execute(Func&& f) {
			m_execute_in_main_stack.emplace_back(std::forward<Func>(f));
		}

		void save_scene_to_path(rynx::string path);
		void load_scene_from_path(rynx::string path);

	public:
		void add_component_include_filter(rynx::string match) { m_component_filters.emplace_back(std::move(match)); }
		void add_tool(rynx::unique_ptr<rynx::editor::itool> tool);

		template<typename ToolType, typename... ArgTypes>
		void add_tool(ArgTypes&&... args) {
			static_assert(std::is_base_of_v<rynx::editor::itool, ToolType>, "only tools derived from editor::itool are allowed");
			auto tool = rynx::make_unique<ToolType>(std::forward<ArgTypes>(args)...);
			tool->m_editor_state = &m_state;
			return add_tool(std::move(tool));
		}

		void create_empty_entity(rynx::vec3f pos) {
			rynx::ecs& ecs = m_context->get_resource<rynx::ecs>();
			auto id = ecs.create(
				rynx::components::transform::position(pos),
				rynx::components::transform::radius(1.0f),
				rynx::components::graphics::color({ 1, 1, 1, 1 }),
				rynx::components::transform::matrix()
			);

			on_entity_selected(id);
		}

		void generate_menu_for_reflection(
			const rynx::reflection::type& type_reflection,
			rynx::editor::component_recursion_info_t info,
			std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> = {}
		);

		void on_entity_selected(rynx::id id);

		editor_rules(
			rynx::scheduler::context& ctx,
			rynx::binary_config::id game_running,
			rynx::reflection::reflections& reflections,
			Font* font,
			rynx::menu::Component* editor_menu_host,
			rynx::graphics::GPUTextures& textures);

		rynx::editor::itool& get_default_tool() {
			return *m_tools[0];
		}

		void switch_to_tool(rynx::editor::itool& tool) {
			if (&tool == m_active_tool)
				return;
			
			m_active_tool->on_tool_unselected();
			tool.on_tool_selected();
			m_active_tool = &tool;
		}

	private:
		virtual void onFrameProcess(rynx::scheduler::context& context, float /* dt */) override;
	};
}