
#pragma once

#include <rynx/menu/Frame.hpp>
#include <rynx/menu/Component.hpp>

#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>

#include <rynx/graphics/renderer/textrenderer.hpp> // annoying but have to include to get vision to TextRenderer::Align

#include <functional>

class Font;

namespace rynx {
	class mapped_input;

	namespace menu {

		class Text : public Component {
		private:
			rynx::floats4 m_text_color;
			rynx::floats4 m_text_dimming_when_not_hovering;

			vec3<float> m_defaultScale;
			
			rynx::graphics::renderable_text m_textline;

			struct Config {
				bool allow_input = false;
				bool input_type_numeric = true;
				bool input_type_alhabet = true;
				bool dim_when_not_hover = true;
			};

			Config config;
			bool m_hasDedicatedInput = false;

			std::function<void(std::string)> m_commit;
			int32_t m_cursor_pos = 0;

		public:
			Text(
				vec3<float> scale,
				vec3<float> position = vec3<float>()
			);

			Text& input_enable() { config.allow_input = true; return *this; }
			Text& input_disable() { config.allow_input = false; return *this; }
			Text& text_align_left() { m_textline.align_left(); return *this; }
			Text& text_align_right() { m_textline.align_right(); return *this; }
			Text& text_align_center() { m_textline.align_center(); return *this; }
			
			Text& font(Font* font) { m_textline.font(font); return *this; }
			Text& text(std::string t) { m_textline.text() = std::move(t); return *this; }
			Text& on_value_changed(std::function<void(const std::string&)> op) { m_commit = std::move(op); return *this; }
			Text& color(floats4 c) { m_text_color = c; return *this; }
			floats4& color() { return m_text_color; }

			Text& dimming(float v) {
				m_text_dimming_when_not_hovering = rynx::floats4(v, v, v, 0.0f);
				return *this;
			}

		private:
			virtual void onDedicatedInputGained() override;
			virtual void onDedicatedInputLost() override;
			virtual void onDedicatedInput(rynx::mapped_input& input) override;
			virtual void onInput(rynx::mapped_input& input) override;
			virtual void draw(rynx::graphics::renderer& renderer) const override;
			virtual void update(float dt) override;
		};
	}
}
