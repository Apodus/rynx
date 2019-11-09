#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/application/render.hpp>
#include <rynx/application/components.hpp>

#include <rynx/tech/profiling.hpp>

// required for implementation
#include <rynx/tech/ecs.hpp>

namespace rynx {
	namespace application {
		namespace visualisation {
			struct boundary_renderer : public rynx::application::renderer::irenderer {

				struct a_draw_call {
					vec3<float> p1;
					vec3<float> p2;
					float width;
					floats4 color;
				};

				boundary_renderer(MeshRenderer* meshRenderer) {
					m_meshRenderer = meshRenderer;
				}
				virtual ~boundary_renderer() {}
				
				virtual void prepare(rynx::scheduler::context* ctx) override {
					rynx_profile("visualisation", "boundary_renderer");
					ctx->add_task("fetch polygon boundaries", [this](const rynx::ecs& ecs) {
						draws.clear();
						ecs.for_each([this](
							const rynx::components::boundary& m,
							const rynx::components::color& color
							) {
								for (const auto& line : m.segments_world) {
									draws.emplace_back(a_draw_call{line.p1, line.p2, 0.4f, color.value});
								}
							});
					});
				}
				
				virtual void render() override {
					for (auto&& draw : draws) {
						m_meshRenderer->drawLine(draw.p1, draw.p2, draw.width, draw.color);
					}
				}

				std::vector<a_draw_call> draws;
				MeshRenderer* m_meshRenderer;
			};
		}
	}
}
