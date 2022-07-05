
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
					rynx::shared_ptr<rynx::graphics::shaders> shader_manager
				);

				virtual ~ambient_light_effect() {}
				virtual void prepare(rynx::scheduler::context* ctx) override;
				virtual void execute() override;

				void set_global_ambient(rynx::floats4 color);
				void set_global_directed(rynx::floats4 color, rynx::vec3f direction);
			private:
				rynx::floats4 m_light_colors[2];
				rynx::vec3f m_light_direction;
				
				rynx::shared_ptr<rynx::graphics::shader> m_lights_shader;
			};
		}
	}
}
