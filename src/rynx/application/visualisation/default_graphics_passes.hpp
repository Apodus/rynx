
#pragma once

#include <rynx/application/visualisation/renderer.hpp>

namespace rynx {
	namespace graphics {
		class renderer;
		class shaders;
	}

	namespace application {
		namespace visualisation {
			rynx::unique_ptr<rynx::application::graphics_step> default_geometry_pass(rynx::graphics::renderer* pRenderer);
			rynx::unique_ptr<rynx::application::graphics_step> default_lighting_pass(rynx::shared_ptr<rynx::graphics::shaders> shaders);
		}
	}
}
