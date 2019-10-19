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
					out |= p.y + r < cameraTop;
					out |= p.y - r > cameraBot;
					return out;
				}

				virtual ~ball_renderer() {}
				virtual void render(const rynx::ecs& ecs) override {
					rynx_profile("visualisation", "ball_renderer");
					cameraLeft = m_camera->position().x - m_camera->position().z;
					cameraRight = m_camera->position().x + m_camera->position().z;
					cameraTop = m_camera->position().y + m_camera->position().z;
					cameraBot = m_camera->position().y - m_camera->position().z;

					ecs.query().notIn<rynx::components::boundary, rynx::components::mesh>().execute([this](
						const rynx::components::position pos,
						const rynx::components::radius r,
						const rynx::components::color color)
					{
						if (!inScreen(pos.value, r.r))
							return;

						matrix4 model;
						model.discardSetTranslate(pos.value);
						model.scale(r.r);
						m_meshRenderer->drawMesh(*m_circleMesh, model, "Empty", color.value);
						m_meshRenderer->drawLine(pos.value, pos.value + r.r * vec3<float>(std::cos(pos.angle), std::sin(pos.angle), 0), r.r * 0.3f, Color::DARK_GREY);
					});
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
