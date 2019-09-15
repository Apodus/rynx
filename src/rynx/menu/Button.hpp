
#pragma once

#include <rynx/menu/Frame.hpp>
#include <rynx/menu/Component.hpp>

#include <rynx/tech/math/matrix.hpp>
#include <rynx/tech/math/vector.hpp>

#include <rynx/graphics/renderer/textrenderer.hpp> // annoying but have to include to get vision to TextRenderer::Align

#include <functional>

class Mesh;
class Font;

namespace rynx {
	namespace input {
		class mapped_input;
	}

	namespace menu {

		class Button : public Component {
		protected:
			std::string m_texture;
			std::string m_text;
			rynx::smooth<vec4<float>> m_color;
			rynx::smooth<vec4<float>> m_textColor;

			vec3<float> m_defaultScale;
			matrix4 m_model;

			Frame m_frame;
			std::unique_ptr<Mesh> m_mesh;
			TextRenderer::Align m_align;
			Font* m_font = nullptr;

			std::function<void()> m_onClick;

		public:
			Button(
				GPUTextures& textures,
				std::string texture,
				Component* parent,
				vec3<float> scale,
				vec3<float> position = vec3<float>()
			) : Component(parent, scale, position)
				, m_frame(this, textures, texture)
			{
				m_defaultScale = scale;
				initialize(textures, std::move(texture));
			}

			Button& font(Font* font) { m_font = font; return *this; }
			Button& text(std::string t, Font* font) { m_text = std::move(t); m_font = font; return *this; }
			Button& text(std::string t) { m_text = std::move(t); return *this; }
			Button& color_frame(vec4<float> color) { m_color = color; return *this; }
			Button& color_text(vec4<float> color) { m_textColor = color; return *this; }
			template<typename F> Button& onClick(F&& op) { m_onClick = std::forward<F>(op); return *this; }

		private:

			void initialize(
				GPUTextures& textures,
				std::string texture
			);

			virtual void onInput(rynx::input::mapped_input& input) override;
			virtual void draw(MeshRenderer& meshRenderer, TextRenderer& textRenderer) const override;
			virtual void update(float dt) override;
		};

	}
}