

#pragma once

#include <rynx/graphics/texture/id.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/tech/smooth_value.hpp>
#include <rynx/std/memory.hpp>
#include <rynx/std/string.hpp>

#include <vector>

namespace rynx {
	namespace graphics {
		class text_renderer;
		class renderer;
	}
	class mapped_input;
	class scoped_input_inhibitor;

	namespace graphics {
		class GPUTextures;
	}
	
	namespace menu {
		enum Align {
			NONE = 0,
			LEFT = 1,
			RIGHT = 2,
			TOP = 4,
			TOP_LEFT = 5,
			TOP_RIGHT = 6,
			BOTTOM = 8,
			BOTTOM_LEFT = 9,
			BOTTOM_RIGHT = 10,
			CENTER_VERTICAL = 16,
			CENTER_HORIZONTAL = 32
		};

		class System;

		class MenuDLL Component {
			friend class rynx::menu::System;

		private:
			enum class offset_kind {
				RelativeToHostScale,
				RelativeToSelfScale,
				RelativeToParentScale,
				Absolute
			};
			
			struct Attachment {
				const Component* attachedTo = nullptr;
				vec3<float> align;
				float offset = 0;
				float innerOuterMultiplier = 1;
				offset_kind offsetKind = offset_kind::RelativeToSelfScale;
				bool is_attached = false;
				operator bool() const { return is_attached; }
			};

			Attachment m_verticalAttachment;
			Attachment m_horizontalAttachment;

			void privateAlign(Align sideOf, float mul);
			void privateAttach(const Component* other, Align align);

			void updateAttachment();
			void updatePosition();
			void updateScale();

		protected:
			vec3<float> m_positionAlign;
			Component* m_pParent;
			std::vector<rynx::shared_ptr<Component>> m_children;

			rynx::smooth<vec3<float>> m_position;
			rynx::smooth<vec3<float>> m_scale;
			rynx::smooth<rynx::floats4> m_color;
			
			vec3<float> m_worldPosition;
			vec3<float> m_worldScale;

			float m_position_update_velocity = 1.0f;
			float m_scale_update_velocity = 1.0f;

			float m_aspectRatio = 1;
			bool m_active = true;
			bool m_respect_aspect_ratio = false;
			
			// if dynamic scale is true, the scale is adapted to contain all children.
			// children will be marked to ignore parent scale.
			bool m_dynamic_scale = false;
			bool m_ignore_parent_scale = false;

			std::vector<rynx::function<bool(rynx::vec3f, bool)>> m_on_hover;
			std::vector<rynx::function<void(rynx::mapped_input&)>> m_on_input;
			std::vector<rynx::function<void()>> m_on_click;
			std::vector<rynx::function<void()>> m_on_update;

			rynx::unique_ptr<Component> m_background;
			System* m_menuSystem = nullptr;

			virtual void onInput(rynx::mapped_input& input) = 0;
			virtual void draw(rynx::graphics::renderer& renderer) const = 0;
			virtual void update(float dt) = 0;

			float aspectRatio() const { return m_aspectRatio; }

		public:
			Component(
				vec3<float> scale,
				vec3<float> position = vec3<float>(0, 0, 0)
			);

			virtual ~Component();

			void disable_input() { m_active = false; }
			void enable_input() { m_active = true; }
			void set_input_enabled(bool enableInput) {
				m_active = enableInput;
			}

			void recursive_call(rynx::function<void(rynx::menu::Component*)> func) {
				func(this);
				for (auto& child : m_children)
					child->recursive_call(func);
			}

			// TODO:
			void die() {
				parent()->detachChild(this);
			}

			void capture_dedicated_mouse_input();
			void capture_dedicated_keyboard_input();
			void release_dedicated_mouse_input();
			void release_dedicated_keyboard_input();
			bool has_dedicated_mouse_input() const;
			bool has_dedicated_keyboard_input() const;

			void on_hover(rynx::function<bool(rynx::vec3f, bool)> hover_func) { m_on_hover.emplace_back(std::move(hover_func)); }
			void on_click(rynx::function<void()> click_func) { m_on_click.emplace_back(std::move(click_func)); }
			void on_input(rynx::function<void(rynx::mapped_input&)> input_func) { m_on_input.emplace_back(std::move(input_func)); }
			void on_update(rynx::function<void()> update_func) { m_on_update.emplace_back(std::move(update_func)); }

			void set_background(rynx::graphics::texture_id, float edge_size = 0.2f);

			Component* parent() const { return m_pParent; }
			float velocity_position() const { return m_position_update_velocity; }
			float scale_position() const { return m_scale_update_velocity; }
			void velocity_position(float v) { m_position_update_velocity = v; }
			void scale_position(float v) { m_scale_update_velocity = v; }

			rynx::floats4& color() { return m_color.value(); }
			void color(rynx::floats4 value) { m_color.target_value() = value; }

			void input(rynx::mapped_input& input);
			
			virtual void onDedicatedInput(rynx::mapped_input& input) { for (auto&& child : m_children) child->input(input); }
			virtual void onDedicatedInputLost() {}
			virtual void onDedicatedInputGained() {}

			void tick(float dt, float aspectRatio);
			void position_approach(float percentage) { m_position.tick(percentage); }
			void visualise(rynx::graphics::renderer& renderer) const;

			void set_dynamic_position_and_scale() { m_dynamic_scale = true; }
			void ignore_parent_scale() { m_ignore_parent_scale = true; }

			class AlignConfig {
			private:
				Component* m_host;
				const Component* m_align_to;

				void private_do_align(rynx::menu::Align sideOf, float directionMultiplier) {
					m_host->privateAttach(m_align_to, sideOf);
					m_host->privateAlign(sideOf, directionMultiplier);
				}

			public:
				AlignConfig(Component* host) { m_host = host; m_align_to = m_host->parent(); }
				AlignConfig& target(const Component* component) { m_align_to = component; return *this; }

				AlignConfig& offset(float offset_v) {
					m_host->m_horizontalAttachment.offset = offset_v;
					m_host->m_verticalAttachment.offset = offset_v;
					return *this;
				}

				AlignConfig& offset_x(float offset_v) {
					m_host->m_horizontalAttachment.offset = offset_v;
					return *this;
				}

				AlignConfig& offset_y(float offset_v) {
					m_host->m_verticalAttachment.offset = offset_v;
					return *this;
				}

				AlignConfig& offset_kind_relative_to_self() {
					m_host->m_horizontalAttachment.offsetKind = offset_kind::RelativeToSelfScale;
					m_host->m_verticalAttachment.offsetKind = offset_kind::RelativeToSelfScale;
					return *this;
				}

				AlignConfig& offset_kind_relative_to_host() {
					m_host->m_horizontalAttachment.offsetKind = offset_kind::RelativeToHostScale;
					m_host->m_verticalAttachment.offsetKind = offset_kind::RelativeToHostScale;
					return *this;
				}

				AlignConfig& offset_kind_relative_to_parent() {
					m_host->m_horizontalAttachment.offsetKind = offset_kind::RelativeToParentScale;
					m_host->m_verticalAttachment.offsetKind = offset_kind::RelativeToParentScale;
					return *this;
				}

				AlignConfig& offset_kind_absolute() {
					m_host->m_horizontalAttachment.offsetKind = offset_kind::Absolute;
					m_host->m_verticalAttachment.offsetKind = offset_kind::Absolute;
					return *this;
				}

				AlignConfig& center_x() { private_do_align(rynx::menu::Align::CENTER_HORIZONTAL, 0.0f); return *this; }
				AlignConfig& center_y() { private_do_align(rynx::menu::Align::CENTER_VERTICAL, 0.0f); return *this; }

				AlignConfig& left_inside() { private_do_align(rynx::menu::Align::LEFT, -1); return *this; }
				AlignConfig& right_inside() { private_do_align(rynx::menu::Align::RIGHT, -1); return *this; }
				AlignConfig& bottom_inside() { private_do_align(rynx::menu::Align::BOTTOM, -1); return *this; }
				AlignConfig& top_inside() { private_do_align(rynx::menu::Align::TOP, -1); return *this; }

				AlignConfig& top_right_inside() { private_do_align(rynx::menu::Align::TOP_RIGHT, -1); return *this; }
				AlignConfig& top_left_inside() { private_do_align(rynx::menu::Align::TOP_LEFT, -1); return *this; }
				AlignConfig& bottom_right_inside() { private_do_align(rynx::menu::Align::BOTTOM_RIGHT, -1); return *this; }
				AlignConfig& bottom_left_inside() { private_do_align(rynx::menu::Align::BOTTOM_LEFT, -1); return *this; }

				AlignConfig& left_outside() { private_do_align(rynx::menu::Align::LEFT, +1); return *this; }
				AlignConfig& right_outside() { private_do_align(rynx::menu::Align::RIGHT, +1); return *this; }
				AlignConfig& bottom_outside() { private_do_align(rynx::menu::Align::BOTTOM, +1); return *this; }
				AlignConfig& top_outside() { private_do_align(rynx::menu::Align::TOP, +1); return *this; }

				AlignConfig& top_right_outside() { private_do_align(rynx::menu::Align::TOP_RIGHT, +1); return *this; }
				AlignConfig& top_left_outside() { private_do_align(rynx::menu::Align::TOP_LEFT, +1); return *this; }
				AlignConfig& bottom_right_outside() { private_do_align(rynx::menu::Align::BOTTOM_RIGHT, +1); return *this; }
				AlignConfig& bottom_left_outside() { private_do_align(rynx::menu::Align::BOTTOM_LEFT, +1); return *this; }
			};

			AlignConfig align() { return { this }; }

			void clear_children() { m_children.clear(); }
			void addChild(rynx::shared_ptr<Component> child);
			rynx::shared_ptr<Component> detachChild(const Component* ptr);
			rynx::menu::Component* last_child(int32_t n=0) {
				rynx_assert(n < m_children.size(), "nth child from back not available");
				return (m_children.end()-(n+1))->get();
			}
			
			void set_parent(Component* other);
			void reparent(Component& other);
			void respect_aspect_ratio() { m_respect_aspect_ratio = true; }

			bool inRectComponent(float x, float y) const;
			bool inRectComponent(vec3<float> v) { return inRectComponent(v.x, v.y); }
			bool inScreen() const;

			vec3<float> position_local() const;
			const vec3<float>& position_world() const;
			

			vec3<float> scale_local() const;
			const vec3<float>& scale_world() const;
			
			Component& position_local(vec3<float> pos);
			Component& scale_local(vec3<float> scale);
			vec3<float> position_exterior_absolute(Align positionAlign_) const;
			vec3<float> position_relative(vec3<float> direction) const;
			vec3<float>& target_scale();
			vec3<float>& target_position();
		};

		// TODO: naming is too vague. this is an instance of a menu tree.
		//       contains the menu root component, which may have child components.
		//       a menu "System" is guaranteed to not have a parent menu element.
		class MenuDLL System {
			rynx::unique_ptr<Component> m_root;
			std::vector<rynx::shared_ptr<rynx::menu::Component>> m_popups;
			std::vector<rynx::function<void()>> m_delayed_ops;

			// some components require dedicated focus state to function correctly.
			// for example think of a text input box. clickin this will enable inputs
			// for the text input box, but then we must also disable inputs for any other
			// text input box. or else user can by accident select multiple text input boxes
			// and be surprised when key strokes will be added to all of them. so this focused
			// component points to either nothing, or the currently active dedicated input.
			Component* m_keyboardInputCapturedBy = nullptr;
			Component* m_mouseInputCapturedBy = nullptr;

		public:
			System();
			
			void capture_mouse_input(Component* component);
			void capture_keyboard_input(Component* component);
			void release_mouse_input();
			void release_keyboard_input();

			bool has_dedicated_mouse_input(const Component* component) const;
			bool has_dedicated_keyboard_input(const Component* component) const;

			rynx::scoped_input_inhibitor inhibit_dedicated_inputs(rynx::mapped_input& input);

			Component* root() { return m_root.get(); }

			void input(rynx::mapped_input& input);
			void update(float dt, float aspectRatio);
			void draw(rynx::graphics::renderer& renderer);

			
			void execute(rynx::function<void()> op) {
				m_delayed_ops.emplace_back(std::move(op));
			}

			void push_popup(rynx::shared_ptr<rynx::menu::Component> popup) {
				execute([this, popup]() {
					if (!popup->parent()) {
						m_root->addChild(popup);
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
		};
	}
}

inline rynx::menu::Align operator | (rynx::menu::Align a, rynx::menu::Align b) {
	return static_cast<rynx::menu::Align>(static_cast<int>(a) | static_cast<int>(b));
}