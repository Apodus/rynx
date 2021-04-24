
#pragma once

#include <rynx/menu/Text.hpp>
#include <rynx/menu/Component.hpp>
#include <rynx/math/vector.hpp>
#include <functional>

namespace rynx {
	class mapped_input;
	
	namespace menu {
		class Button : public Component {
		protected:
			vec3<float> m_defaultScale;
			std::shared_ptr<rynx::menu::Text> m_textField;
			float m_no_focus_alpha = 0.3f;
		public:
			Button(
				rynx::graphics::texture_id texture,
				vec3<float> scale,
				vec3<float> position = vec3<float>(),
				float frame_edge_percentage = 0.2f
			) : Component(scale, position)
			{
				m_textField = std::make_shared<rynx::menu::Text>(rynx::vec3f(1.0f, 1.0f, 0.0f));
				set_background(texture, frame_edge_percentage);
				m_defaultScale = scale;
				this->addChild(m_textField);
				initialize();
			}

			Button& no_focus_alpha(float v) {
				m_no_focus_alpha = v;
				return *this;
			}

			Text& text() { return *m_textField; } 
			
		private:
			void initialize();

			virtual void onInput(rynx::mapped_input& input) override;
			virtual void draw(rynx::graphics::renderer& renderer) const override;
			virtual void update(float dt) override;
		};
	}
}
