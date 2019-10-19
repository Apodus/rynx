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
				boundary_renderer(MeshRenderer* meshRenderer) {
					m_meshRenderer = meshRenderer;
				}
				virtual ~boundary_renderer() {}
				virtual void render(const rynx::ecs& ecs) override {
					rynx_profile("visualisation", "boundary_renderer");
					ecs.query().notIn<rynx::components::mesh>().execute([this](const rynx::components::boundary& m, const rynx::components::color& color) {
						for (const auto& line : m.segments_world) {
							m_meshRenderer->drawLine(line.p1, line.p2, 0.4f, color.value);
						}
					});
				}

				MeshRenderer* m_meshRenderer;
			};
		}
	}
}
