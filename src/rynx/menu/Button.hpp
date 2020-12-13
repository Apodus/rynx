
#pragma once

#include <rynx/menu/Frame.hpp>
#include <rynx/menu/Text.hpp>
#include <rynx/menu/Component.hpp>

#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>

#include <rynx/graphics/renderer/textrenderer.hpp> // annoying but have to include to get vision to TextRenderer::Align

#include <functional>

class Font;

namespace rynx {
	class mapped_input;
	
	namespace menu {

		class Button : public Component {
		protected:
			vec3<float> m_defaultScale;
			std::shared_ptr<rynx::menu::Text> m_textField;

		public:
			Button(
				rynx::graphics::GPUTextures& textures,
				std::string texture,
				vec3<float> scale,
				vec3<float> position = vec3<float>(),
				float frame_edge_percentage = 0.2f
			) : Component(scale, position)
			{
				m_textField = std::make_shared<rynx::menu::Text>(rynx::vec3f(1.0f, 1.0f, 0.0f));
				set_background(textures, std::move(texture), frame_edge_percentage);
				m_defaultScale = scale;
				this->addChild(m_textField);
				initialize();
			}

			Text& text() { return *m_textField; } 
			
		private:
			void initialize();

			virtual void onInput(rynx::mapped_input& input) override;
			virtual void draw(rynx::graphics::renderer& meshRenderer, rynx::graphics::text_renderer& textRenderer) const override;
			virtual void update(float dt) override;
		};
	}
}
