
#pragma once

#include <rynx/tech/ecs.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>

#include <rynx/menu/Div.hpp>
#include <rynx/menu/Text.hpp>
#include <rynx/menu/Button.hpp>
#include <rynx/menu/Slider.hpp>
#include <rynx/menu/List.hpp>

#include <rynx/application/logic.hpp>
#include <rynx/input/key_types.hpp>
#include <rynx/tech/collision_detection.hpp>

#include <rynx/editor/tools/selection_tool.hpp>
#include <rynx/editor/tools/polygon_tool.hpp>

#include <rynx/math/geometry/ray.hpp>

namespace rynx {
	namespace editor {

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

		struct rynx_common_info {
			rynx::ecs* ecs = nullptr;
			rynx::ecs::id entity_id = 0;
			rynx::graphics::GPUTextures* textures = nullptr;
			rynx::graphics::texture_id frame_tex;
			rynx::graphics::texture_id knob_tex;

			int32_t component_type_id = 0;
			int32_t cumulative_offset = 0;
			int32_t indent = 0;
		};

		void field_float(
			const rynx::reflection::field& member,
			struct rynx_common_info info,
			rynx::menu::Component* component_sheet,
			std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>>
		);

		void field_bool(
			const rynx::reflection::field& member,
			struct rynx_common_info info,
			rynx::menu::Component* component_sheet,
			std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>>
		);
	}

	class editor_rules : public rynx::application::logic::iruleset {
	private:
		rynx::editor::editor_shared_state m_state;

		rynx::key::logical key_selection_tool;
		rynx::key::logical key_polygon_tool;

		rynx::graphics::texture_id frame_tex;
		rynx::graphics::texture_id knob_tex;

		std::shared_ptr<rynx::menu::Div> m_editor_menu; // editor root level menu container.
		std::shared_ptr<rynx::menu::Div> m_tools_bar; // right side menu, containing info of tools
		
		// TODO!
		std::shared_ptr<rynx::menu::Div> m_scene_bar; // top side menu, containing existing scenes, selecting one will instantiate it in current scene.
		
		// TODO!
		std::shared_ptr<rynx::menu::Div> m_file_actions_bar; // bottom side menu, containing global actions (new empty scene, save scene, load scene)
		
		std::shared_ptr<rynx::menu::Div> m_entity_bar; // left side menu, containing info of selection
		std::shared_ptr<rynx::menu::List> m_components_list; // entity bar component list view - showing all components of an entity.

		std::vector<std::shared_ptr<rynx::menu::Component>> m_popups;

		rynx::editor::itool* m_active_tool = nullptr;
		std::vector<std::unique_ptr<rynx::editor::itool>> m_tools;
		
		rynx::reflection::reflections& m_reflections;
		rynx::scheduler::context* m_context = nullptr;
		Font* m_font = nullptr;

		bool m_tools_enabled = true;

		void push_popup(std::shared_ptr<rynx::menu::Component> popup) {
			if (!popup->parent()) {
				m_editor_menu->addChild(popup);
			}
			popup->capture_dedicated_mouse_input();
			popup->capture_dedicated_keyboard_input();
			m_popups.emplace_back(popup);
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

		std::vector<std::function<void()>> m_execute_in_main_stack;

		template<typename Func>
		void execute(Func&& f) {
			m_execute_in_main_stack.emplace_back(std::forward<Func>(f));
		}

	public:
		void add_tool(std::unique_ptr<rynx::editor::itool> tool);

		template<typename ToolType, typename... ArgTypes>
		void add_tool(ArgTypes&&... args) {
			static_assert(std::is_base_of_v<rynx::editor::itool, ToolType>, "only tools derived from editor::itool are allowed");
			auto tool = std::make_unique<ToolType>(std::forward<ArgTypes>(args)...);
			tool->m_editor_state = &m_state;
			return add_tool(std::move(tool));
		}

		void create_empty_entity(rynx::vec3f pos) {
			rynx::ecs& ecs = m_context->get_resource<rynx::ecs>();
			auto id = ecs.create(
				rynx::components::position(pos),
				rynx::components::radius(1.0f),
				rynx::components::color({ 1, 1, 1, 1 }),
				rynx::matrix4()
			);

			on_entity_selected(id);
		}

		void generate_menu_for_reflection(
			rynx::reflection::reflections& reflections_,
			const rynx::reflection::type& type_reflection,
			rynx::editor::rynx_common_info info,
			rynx::menu::Component* component_sheet_,
			std::vector<std::pair<rynx::reflection::type, rynx::reflection::field>> = {}
		);

		void on_entity_selected(rynx::id id);

		editor_rules(
			rynx::scheduler::context& ctx,
			rynx::reflection::reflections& reflections,
			Font* font,
			rynx::menu::Component* editor_menu_host,
			rynx::graphics::GPUTextures& textures,
			rynx::mapped_input& gameInput);

		rynx::editor::itool& get_default_tool() {
			return *m_tools[0];
		}

		void switch_to_tool(rynx::editor::itool& tool) {
			m_active_tool->on_tool_unselected();
			tool.on_tool_selected();
			m_active_tool = &tool;
		}

	private:
		virtual void onFrameProcess(rynx::scheduler::context& context, float /* dt */) override {

			if (m_tools_enabled) {
				m_active_tool->update(context);
			}

			while (!m_execute_in_main_stack.empty()) {
				m_execute_in_main_stack.front()();
				m_execute_in_main_stack.erase(m_execute_in_main_stack.begin());
			}

			{
				auto& gameInput = context.get_resource<rynx::mapped_input>();
				auto& gameCamera = context.get_resource<rynx::camera>();

				auto mouseRay = gameInput.mouseRay(gameCamera);
				auto mouse_z_plane = mouseRay.intersect(rynx::plane(0, 0, 1, 0));
				mouse_z_plane.first.z = 0;

				if (mouse_z_plane.second) {
					// mouse input stuff
					if (gameInput.isKeyClicked(gameInput.getMouseKeyPhysical(1))) {
						create_empty_entity(mouse_z_plane.first);
					}
				}
			}

			/*
			context.add_task("editor tick", [this, dt](
				rynx::ecs& game_ecs,
				rynx::mapped_input& gameInput,
				rynx::camera& gameCamera)
				{
				}
			);
			*/
		}
	};
}