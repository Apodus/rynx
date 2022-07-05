
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
			struct omnilights_effect : public rynx::application::graphics_step::igraphics_step {
			public:
				omnilights_effect(
					rynx::shared_ptr<rynx::graphics::shaders> shader_manager
				);
				
				virtual ~omnilights_effect() {}
				virtual void prepare(rynx::scheduler::context* ctx) override;
				virtual void execute() override;
			
			private:
				std::vector<floats4> m_light_colors;
				std::vector<floats4> m_light_settings;
				std::vector<vec3f> m_light_positions;

				rynx::shared_ptr<rynx::graphics::shader> m_lights_shader;
			};
		}
	}
}
