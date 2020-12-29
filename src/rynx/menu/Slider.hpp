
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
			std::function<void(float)> m_on_drag_end;
			std::function<void(float, float)> m_on_active_tick;

			float m_minValue;
			float m_maxValue;
			float m_currentValue;
			float m_roundTo;

			bool m_mouseDragActive;
			Div* m_knobDiv = nullptr;

			void onActivate(float x, float /* y */);

		public:
			SlideBarVertical(rynx::graphics::GPUTextures& textures, std::string baseTexture, std::string knobTexture, vec3<float> scale, float minValue = 0, float maxValue = 1, float initialValue = 0.5f);

			SlideBarVertical& on_value_changed(std::function<void(float)> t);
			SlideBarVertical& on_drag_end(std::function<void(float)> t);
			SlideBarVertical& on_active_tick(std::function<void(float, float)> t);

			virtual void draw(rynx::graphics::renderer&) const override {}
			virtual void onInput(rynx::mapped_input& input) override;
			virtual void onDedicatedInput(rynx::mapped_input&) override;
			virtual void update(float dt) override;
			

			void setValue(float value);
			float getValue() const;
			float getValueQuadratic() const;
			float getValueQuadratic_AroundCenter() const;
			float getValueCubic() const;
			float getValueCubic_AroundCenter() const;
		};
	}
}
