
#pragma once

#include <rynx/menu/Component.hpp>
#include <rynx/math/matrix.hpp>

#include <string>
#include <functional>


namespace rynx {
	namespace graphics {
		class GPUTextures;
	}
	namespace menu {
		class Div;
		
		class SlideBarVertical : public Component {
			
			std::function<void(float)> m_callback;
			
			float m_minValue;
			float m_maxValue;
			float m_currentValue;
			float m_roundTo;

			bool m_mouseDragActive;
			Div* m_knobDiv = nullptr;

		public:
			SlideBarVertical(rynx::graphics::GPUTextures& textures, std::string knobTexture, std::string baseTexture, Component* parent, vec3<float> scale, float minValue = 0, float maxValue = 1, float initialValue = 0.5f);

			SlideBarVertical& onValueChanged(std::function<void(float)> t);

			virtual void draw(MeshRenderer&, TextRenderer&) const override {}
			virtual void onInput(rynx::mapped_input& input) override;
			virtual void update(float dt) override;
			void onActivate(float x, float /* y */);
			void setValue(float value);
		};
	}
}
