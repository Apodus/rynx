#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/application/render.hpp>
#include <rynx/application/components.hpp>

// required for implementation
#include <rynx/tech/ecs.hpp>

namespace rynx {
	namespace application {
		namespace visualisation {
			struct ball_renderer : public rynx::application::renderer::irenderer {
				ball_renderer(Mesh* circleMesh, MeshRenderer* meshRenderer) {
					m_circleMesh = circleMesh;
					m_meshRenderer = meshRenderer;
				}
				virtual ~ball_renderer() {}
				virtual void render(const rynx::ecs& ecs) override {
					rynx_profile(Game, "ball_renderer");
					ecs.query().notIn<rynx::components::boundary, rynx::components::mesh>().execute([this](
						const rynx::components::position& pos,
						const rynx::components::radius& r,
						const rynx::components::color& color,
						const rynx::components::frame_collisions& c)
					{
						vec4<float> used_color = color.value;
						if (!c.collisions.empty())
							used_color = Color::RED;
						
						matrix4 model;
						model.discardSetTranslate(pos.value);
						model.scale(r.r);
						m_meshRenderer->drawMesh(*m_circleMesh, model, "Empty", used_color);
					});
				}

				MeshRenderer* m_meshRenderer;
				Mesh* m_circleMesh;
			};
		}
	}
}
