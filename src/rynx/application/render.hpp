

#pragma once

#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/graphics/framebuffer.hpp>
#include <memory>

class Camera;
namespace rynx {
	namespace graphics {
		class shader;
	}
	
	namespace application {
		class Application;

		class renderer : public rynx::application::igraphics_step {
		public:
			renderer(rynx::application::Application& application, std::shared_ptr<Camera> camera);
			virtual void execute() override;
			virtual void prepare(rynx::scheduler::context* ctx) override;

		private:
			std::unique_ptr<rynx::application::graphics_step> geometry_pass;
			std::unique_ptr<rynx::application::graphics_step> lighting_pass;

			std::shared_ptr<rynx::graphics::shader> shader_copy_color;
			std::shared_ptr<rynx::graphics::framebuffer> fbo_world_geometry;
			std::shared_ptr<rynx::graphics::framebuffer> fbo_lights;
			
			rynx::application::Application& m_application;
			std::shared_ptr<Camera> camera;
		};
	}
}
