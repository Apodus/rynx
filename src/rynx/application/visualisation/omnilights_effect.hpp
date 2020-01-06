
#pragma once

#include <rynx/application/render.hpp>
#include <rynx/tech/math/vector.hpp>

namespace rynx {
	namespace graphics {
		class shader;
		class shaders;
		class screenspace_draws;
	}

	namespace application {
		namespace visualisation {
			struct omnilights_effect : public rynx::application::renderer::irenderer {
			public:
				omnilights_effect(
					std::shared_ptr<rynx::graphics::shaders> shader_manager,
					std::shared_ptr<rynx::graphics::screenspace_draws> screenspace
				);
				
				virtual ~omnilights_effect() {}
				virtual void prepare(rynx::scheduler::context* ctx) override;
				virtual void render() override;
			
			private:
				std::vector<floats4> m_light_colors;
				std::vector<vec3f> m_light_positions;
				std::vector<float> m_light_ambients;

				std::shared_ptr<rynx::graphics::shader> m_lights_shader;
				std::shared_ptr<rynx::graphics::screenspace_draws> m_screenspace;
			};
		}
	}
}
