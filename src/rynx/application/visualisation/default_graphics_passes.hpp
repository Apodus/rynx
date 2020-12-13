
#pragma once

#include <rynx/application/visualisation/renderer.hpp>

namespace rynx {
	namespace graphics {
		class renderer;
		class shaders;
	}

	namespace application {
		namespace visualisation {
			std::unique_ptr<rynx::application::graphics_step> default_geometry_pass(rynx::graphics::renderer* pRenderer);
			std::unique_ptr<rynx::application::graphics_step> default_lighting_pass(std::shared_ptr<rynx::graphics::shaders> shaders);
		}
	}
}
