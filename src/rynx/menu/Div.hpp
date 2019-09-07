
#pragma once

#include <rynx/menu/Component.hpp>

namespace rynx {
	namespace menu {
		class Div : public Component {
		public:
			Div(Component* parent, vec3<float> scale, vec3<float> position = {}) : Component(parent, scale, position) {}
			Div(vec3<float> scale, vec3<float> position = {}) : Component(scale, position) {}

		private:
			virtual void onInput(rynx::input::mapped_input&) {}
			virtual void draw(MeshRenderer&, TextRenderer&) const override {}
			virtual void update(float) override {}
		};
	}
}
