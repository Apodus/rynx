
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
			struct directed_lights_effect : public rynx::application::graphics_step::igraphics_step {
			public:
				directed_lights_effect(
					std::shared_ptr<rynx::graphics::shaders> shader_manager
				);
				
				virtual ~directed_lights_effect() {}
				virtual void prepare(rynx::scheduler::context* ctx) override;
				virtual void execute() override;
			
			private:
				std::vector<floats4> m_light_colors;
				std::vector<floats4> m_light_directions;
				std::vector<floats4> m_light_settings; // x=edge softness[0...inf], y=linear attenuation[0..inf], z=quadratic attenuation[0..inf], a=backside lighting (penetrating) [0..1]
				std::vector<vec3f> m_light_positions;
				
				std::shared_ptr<rynx::graphics::shader> m_lights_shader;
			};
		}
	}
}
