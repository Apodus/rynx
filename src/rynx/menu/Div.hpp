
#pragma once

#include <rynx/menu/Component.hpp>

namespace rynx {
	namespace menu {
		class Div : public Component {
		public:
			Div(vec3<float> scale, vec3<float> position = {}) : Component(scale, position) {}

		private:
			virtual void onInput(rynx::mapped_input&) override {}
			virtual void draw(rynx::graphics::renderer&) const override {}
			virtual void update(float) override {}
		};
	}
}
