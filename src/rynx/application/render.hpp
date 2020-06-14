

#pragma once

#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/graphics/framebuffer.hpp>
#include <memory>

namespace rynx {
	class camera;
	namespace graphics {
		class shader;
	}
	
	namespace application {
		class Application;

		class renderer : public rynx::application::igraphics_step {
		public:
			renderer(rynx::application::Application& application, std::shared_ptr<camera> camera);
			virtual void execute() override;
			virtual void prepare(rynx::scheduler::context* ctx) override;

			void set_geometry_resolution(float percentage_x, float percentage_y);
			void set_lights_resolution(float percentage_x, float percentage_y);
			void on_resolution_change(size_t new_size_x, size_t new_size_y);

		private:
			std::unique_ptr<rynx::application::graphics_step> geometry_pass;
			std::unique_ptr<rynx::application::graphics_step> lighting_pass;

			std::shared_ptr<rynx::graphics::GPUTextures> gpu_textures;
			std::shared_ptr<rynx::graphics::shader> shader_copy_color;
			std::shared_ptr<rynx::graphics::framebuffer> fbo_world_geometry;
			std::shared_ptr<rynx::graphics::framebuffer> fbo_lights;
			
			std::pair<size_t, size_t> current_resolution;
			std::pair<float, float> current_internal_resolution_geometry;
			std::pair<float, float> current_internal_resolution_lighting;

			rynx::application::Application& m_application;
			std::shared_ptr<camera> camera;
		};
	}
}
