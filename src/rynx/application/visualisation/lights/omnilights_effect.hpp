
#pragma once

#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/tech/math/vector.hpp>

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
					std::shared_ptr<rynx::graphics::shaders> shader_manager
				);
				
				virtual ~omnilights_effect() {}
				virtual void prepare(rynx::scheduler::context* ctx) override;
				virtual void execute() override;
			
			private:
				std::vector<floats4> m_light_colors;
				std::vector<vec3f> m_light_positions;
				std::vector<float> m_light_ambients;

				std::shared_ptr<rynx::graphics::shader> m_lights_shader;
			};
		}
	}
}