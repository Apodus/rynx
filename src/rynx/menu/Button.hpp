
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

		class Button : public Component {
		protected:
			std::string m_text;
			rynx::smooth<floats4> m_color;
			rynx::smooth<floats4> m_textColor;

			vec3<float> m_defaultScale;
			
			Frame m_frame;
			TextRenderer::Align m_align;
			Font* m_font = nullptr;

			std::function<void()> m_onClick;

		public:
			Button(
				GPUTextures& textures,
				std::string texture,
				Component* parent,
				vec3<float> scale,
				vec3<float> position = vec3<float>(),
				float frame_edge_percentage = 0.2f
			) : Component(parent, scale, position)
				, m_frame(this, textures, std::move(texture), frame_edge_percentage)
			{
				m_defaultScale = scale;
				initialize();
			}

			Button& font(Font* font) { m_font = font; return *this; }
			Button& text(std::string t, Font* font) { m_text = std::move(t); m_font = font; return *this; }
			Button& text(std::string t) { m_text = std::move(t); return *this; }
			Button& color_frame(floats4 color) { m_color = color; return *this; }
			Button& color_text(floats4 color) { m_textColor = color; return *this; }
			template<typename F> Button& onClick(F&& op) { m_onClick = std::forward<F>(op); return *this; }

		private:
			void initialize();
			virtual void onInput(rynx::mapped_input& input) override;
			virtual void draw(MeshRenderer& meshRenderer, TextRenderer& textRenderer) const override;
			virtual void update(float dt) override;
		};

	}
}
