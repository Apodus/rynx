

#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/tech/smooth_value.hpp>

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace rynx {
	
	class TextRenderer;
	class MeshRenderer;
	class mapped_input;
	
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

		class Component {
		private:
			struct Attachment {
				Component* attachedTo = nullptr;
				vec3<float> align;
				float offset = 0;
				float innerOuterMultiplier = 1;
				bool is_attached = false;
				operator bool() const { return is_attached; }
			};

			Attachment m_verticalAttachment;
			Attachment m_horizontalAttachment;

			void privateAlign(Align sideOf, float mul);
			void privateAttach(Component* other, Align align);

			void updateAttachment();
			void updatePosition();
			void updateScale();

		protected:
			vec3<float> m_positionAlign;
			Component* m_pParent;
			std::vector<std::shared_ptr<Component>> m_children;

			rynx::smooth<vec3<float>> m_position;
			rynx::smooth<vec3<float>> m_scale;
			
			vec3<float> m_worldPosition;
			vec3<float> m_worldScale;

			float m_position_update_velocity = 1.0f;
			float m_scale_update_velocity = 1.0f;

			float m_aspectRatio = 1;
			bool m_active = true;
			bool m_respect_aspect_ratio = false;

			std::vector<std::function<bool(rynx::vec3f, bool)>> m_on_hover;
			std::vector<std::function<void(rynx::mapped_input&)>> m_on_input;
			std::vector<std::function<void()>> m_on_click;

			virtual void onInput(rynx::mapped_input& input) = 0;
			virtual void draw(MeshRenderer& meshRenderer, TextRenderer& textRenderer) const = 0;
			virtual void update(float dt) = 0;

			float aspectRatio() const { return m_aspectRatio; }

		public:
			Component(
				vec3<float> scale,
				vec3<float> position = vec3<float>(0, 0, 0)
			);

			virtual ~Component() {}

			void on_hover(std::function<bool(rynx::vec3f, bool)> hover_func) { m_on_hover.emplace_back(std::move(hover_func)); }
			void on_click(std::function<void()> click_func) { m_on_click.emplace_back(std::move(click_func)); }
			void on_input(std::function<void(rynx::mapped_input&)> input_func) { m_on_input.emplace_back(std::move(input_func)); }

			Component* parent() const { return m_pParent; }
			float& velocity_position() { return m_position_update_velocity; }
			float& scale_position() { return m_scale_update_velocity; }

			void input(rynx::mapped_input& input);
			void tick(float dt, float aspectRatio);
			void position_approach(float percentage) { m_position.tick(percentage); }
			void visualise(MeshRenderer& meshRenderer, TextRenderer& textRenderer) const;

			class AlignConfig {
			private:
				Component* m_host;
				Component* m_align_to;
				
				void private_do_align(rynx::menu::Align sideOf, float directionMultiplier) {
					m_host->privateAttach(m_align_to, sideOf);
					m_host->privateAlign(sideOf, directionMultiplier);
				}

			public:
				AlignConfig(Component* host) { m_host = host; m_align_to = m_host->parent(); }
				AlignConfig& target(Component* component) { m_align_to = component; return *this; }

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

				AlignConfig& left_inside() { private_do_align(rynx::menu::Align::LEFT, -1); return *this; }
				AlignConfig& right_inside() { private_do_align(rynx::menu::Align::RIGHT, -1); return *this; }
				AlignConfig& bottom_inside() { private_do_align(rynx::menu::Align::BOTTOM, -1); return *this; }
				AlignConfig& top_inside() { private_do_align(rynx::menu::Align::TOP, -1); return *this; }

				AlignConfig& top_right_inside() { private_do_align(rynx::menu::Align::TOP_RIGHT, -1); return *this; }
				AlignConfig& top_left_inside() { private_do_align(rynx::menu::Align::TOP_LEFT, -1); return *this; }
				AlignConfig& bottom_right_inside() { private_do_align(rynx::menu::Align::BOTTOM_RIGHT, -1); return *this; }
				AlignConfig& bottom_left_inside() { private_do_align(rynx::menu::Align::BOTTOM_LEFT, -1); return *this; }

				AlignConfig& left_outide() { private_do_align(rynx::menu::Align::LEFT, +1); return *this; }
				AlignConfig& right_outside() { private_do_align(rynx::menu::Align::RIGHT, +1); return *this; }
				AlignConfig& bottom_outside() { private_do_align(rynx::menu::Align::BOTTOM, +1); return *this; }
				AlignConfig& top_outside() { private_do_align(rynx::menu::Align::TOP, +1); return *this; }

				AlignConfig& top_right_outside() { private_do_align(rynx::menu::Align::TOP_RIGHT, +1); return *this; }
				AlignConfig& top_left_outside() { private_do_align(rynx::menu::Align::TOP_LEFT, +1); return *this; }
				AlignConfig& bottom_right_outside() { private_do_align(rynx::menu::Align::BOTTOM_RIGHT, +1); return *this; }
				AlignConfig& bottom_left_outside() { private_do_align(rynx::menu::Align::BOTTOM_LEFT, +1); return *this; }
			};

			AlignConfig align() { return { this }; }

			void addChild(std::shared_ptr<Component> child);
			std::shared_ptr<Component> detachChild(const Component* ptr);
			
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
	}
}

inline rynx::menu::Align operator | (rynx::menu::Align a, rynx::menu::Align b) {
	return static_cast<rynx::menu::Align>(static_cast<int>(a) | static_cast<int>(b));
}