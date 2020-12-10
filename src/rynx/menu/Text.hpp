
#pragma once

#include <rynx/menu/Frame.hpp>
#include <rynx/menu/Component.hpp>

#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>

#include <rynx/graphics/renderer/textrenderer.hpp> // annoying but have to include to get vision to TextRenderer::Align

#include <functional>

class Font;

namespace rynx {
	class mesh;
	class mapped_input;

	namespace menu {

		class Text : public Component {
		private:
			std::string m_text;
			rynx::floats4 m_text_color;
			rynx::floats4 m_text_dimming_when_not_hovering;

			vec3<float> m_defaultScale;
			TextRenderer::Align m_align;
			Font* m_font = nullptr;

			struct Config {
				bool allow_input = true;
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

			Text& text_align(TextRenderer::Align align) { m_align = align; return *this; }
			Text& font(Font* font) { m_font = font; return *this; }
			Text& text(std::string t) { m_text = std::move(t); return *this; }
			
			Text& dimming(float v) {
				m_text_dimming_when_not_hovering = rynx::floats4(v, v, v, 0.0f);
				return *this;
			}

		private:
			virtual void onDedicatedInputGained() override;
			virtual void onDedicatedInputLost() override;
			virtual void onDedicatedInput(rynx::mapped_input& input) override;
			virtual void onInput(rynx::mapped_input& input) override;
			virtual void draw(MeshRenderer& meshRenderer, TextRenderer& textRenderer) const override;
			virtual void update(float dt) override;
		};
	}
}
