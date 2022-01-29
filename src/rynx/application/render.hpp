

#pragma once

#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/graphics/framebuffer.hpp>
#include <rynx/tech/binary_config.hpp>
#include <rynx/math/vector.hpp>
#include <memory>

namespace rynx {
	class camera;
	namespace graphics {
		class shader;
		class shaders;
		class renderer;
	}
	
	namespace application {
		class Application;

		class renderer : public rynx::application::igraphics_step {
		public:
			renderer(
				std::shared_ptr<rynx::graphics::GPUTextures> textures,
				std::shared_ptr<rynx::graphics::shaders> shaders,
				rynx::graphics::renderer& renderer,
				rynx::observer_ptr<camera> camera);

			virtual void execute() override;
			virtual void prepare(rynx::scheduler::context* ctx) override;

			void geometry_step_insert_front(std::unique_ptr<igraphics_step>);

			void light_global_ambient(rynx::floats4 color);
			void light_global_directed(rynx::floats4 color, rynx::vec3f direction);

			void set_geometry_resolution(float percentage_x, float percentage_y);
			void set_lights_resolution(float percentage_x, float percentage_y);
			void on_resolution_change(size_t new_size_x, size_t new_size_y);

			renderer& debug_draw_binary_config(rynx::binary_config::id& conf) {
				m_debug_draw_config->include_id(conf);
				return *this;
			}

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

			igraphics_step* m_ambients = nullptr;
			rynx::graphics::renderer& m_rynx_renderer;

			std::shared_ptr<rynx::binary_config::id> m_debug_draw_config;

			rynx::observer_ptr<camera> camera;
		};
	}
}
