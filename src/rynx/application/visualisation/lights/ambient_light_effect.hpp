
#pragma once

#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/math/vector.hpp>

namespace rynx {
	namespace graphics {
		class shader;
		class shaders;
		class screenspace_draws;
	}

	namespace application {
		namespace visualisation {
			struct ambient_light_effect : public rynx::application::graphics_step::igraphics_step {
			public:
				ambient_light_effect(
					std::shared_ptr<rynx::graphics::shaders> shader_manager
				);
				
				virtual ~ambient_light_effect() {}
				virtual void prepare(rynx::scheduler::context* ctx) override;
				virtual void execute() override;
			
			private:
				floats4 m_light_colors[2];
				vec3f m_light_direction;

				std::shared_ptr<rynx::graphics::shader> m_lights_shader;
			};
		}
	}
}
