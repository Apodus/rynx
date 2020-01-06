
#pragma once

#include <rynx/application/visualisation/renderer.hpp>

class Camera;
namespace rynx {
	class MeshRenderer;
	namespace graphics {
		class shaders;
	}

	namespace application {
		namespace visualisation {
			std::unique_ptr<rynx::application::graphics_step> default_geometry_pass(MeshRenderer* pRenderer, Camera* pCamera);
			std::unique_ptr<rynx::application::graphics_step> default_lighting_pass(std::shared_ptr<rynx::graphics::shaders> shaders);
		}
	}
}
