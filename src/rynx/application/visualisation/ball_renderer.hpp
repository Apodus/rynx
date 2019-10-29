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
			struct ball_renderer : public rynx::application::renderer::irenderer {
				ball_renderer(Mesh* circleMesh, MeshRenderer* meshRenderer, Camera* camera) {
					m_circleMesh = circleMesh;
					m_meshRenderer = meshRenderer;
					m_camera = camera;
				}

				bool inScreen(vec3<float> p, float r) {
					bool out = p.x + r < cameraLeft;
					out |= p.x - r > cameraRight;
					out |= p.y + r > cameraTop;
					out |= p.y - r < cameraBot;
					return !out;
				}

				virtual ~ball_renderer() {}
				virtual void render(const rynx::ecs& ecs) override {
					rynx_profile("visualisation", "ball_renderer");
					cameraLeft = m_camera->position().x - m_camera->position().z;
					cameraRight = m_camera->position().x + m_camera->position().z;
					cameraTop = m_camera->position().y + m_camera->position().z;
					cameraBot = m_camera->position().y - m_camera->position().z;

					std::vector<matrix4> spheres_to_draw;
					std::vector<vec4<float>> colors;
					ecs.query().notIn<rynx::components::boundary, rynx::components::mesh>()
						.execute([this, &spheres_to_draw, &colors](
						const rynx::components::position pos,
						const rynx::components::radius r,
						const rynx::components::color color)
					{
						if (!inScreen(pos.value, r.r))
							return;

						matrix4 model;
						model.discardSetTranslate(pos.value);
						model.scale(r.r);
						model.rotate(pos.angle, 0, 0, 1);
						spheres_to_draw.emplace_back(model);
						colors.emplace_back(color.value);
					});
				
					m_meshRenderer->drawMeshInstanced(*m_circleMesh, "Empty", spheres_to_draw);
				}

				// Assumes axis aligned top-down camera :(
				float cameraLeft;
				float cameraRight;
				float cameraTop;
				float cameraBot;

				MeshRenderer* m_meshRenderer;
				Mesh* m_circleMesh;
				Camera* m_camera;
			};
		}
	}
}
