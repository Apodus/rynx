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
					rynx_profile("visualisation", "ball_draw");
					cameraLeft = m_camera->position().x - m_camera->position().z;
					cameraRight = m_camera->position().x + m_camera->position().z;
					cameraTop = m_camera->position().y + m_camera->position().z;
					cameraBot = m_camera->position().y - m_camera->position().z;

					std::vector<matrix4> spheres_to_draw;
					spheres_to_draw.reserve(10 * 1024);
					
					std::vector<floats4> colors;
					
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
				
					m_meshRenderer->drawMeshInstanced(*m_circleMesh, "Empty", spheres_to_draw, colors);

					spheres_to_draw.clear();
					ecs.for_each([this, &ecs, &spheres_to_draw](const rynx::components::rope& rope) {
						
						auto entity_a = ecs[rope.id_a];
						auto entity_b = ecs[rope.id_b];

						auto pos_a = entity_a.get<components::position>();
						auto pos_b = entity_b.get<components::position>();
						auto relative_pos_a = math::rotatedXY(rope.point_a, pos_a.angle);
						auto relative_pos_b = math::rotatedXY(rope.point_b, pos_b.angle);
						auto world_pos_a = pos_a.value + relative_pos_a;
						auto world_pos_b = pos_b.value + relative_pos_b;
						vec3<float> mid = (world_pos_a + world_pos_b) * 0.5f;
						
						auto direction_vector = world_pos_a - world_pos_b;
						float length = direction_vector.length() * 0.5f;
						constexpr float width = 0.6f;
						
						if (!inScreen(mid, length * 0.5f))
							return;

						matrix4 model;
						model.discardSetTranslate(mid.x, mid.y, mid.z);
						model.rotate(math::atan_approx(direction_vector.y / direction_vector.x), 0, 0, 1);
						model.scale(length, width, 1.0f);
						spheres_to_draw.emplace_back(model);
					});

					colors.clear();
					colors.resize(spheres_to_draw.size(), vec4<float>(0, 0, 0, 1));
					m_meshRenderer->drawMeshInstanced(*m_circleMesh, "Empty", spheres_to_draw, colors);
				}

				// Assumes axis aligned top-down camera :(
				float cameraLeft;
				float cameraRight;
				float cameraTop;
				float cameraBot;

				std::shared_ptr<rynx::parallel_accumulator<matrix4>> m_balls_matrices;
				std::shared_ptr<rynx::parallel_accumulator<matrix4>> m_balls_colors;

				std::shared_ptr<rynx::parallel_accumulator<matrix4>> m_particles_matrices;
				std::shared_ptr<rynx::parallel_accumulator<matrix4>> m_particles_colors;

				MeshRenderer* m_meshRenderer;
				Mesh* m_circleMesh;
				Camera* m_camera;
			};
		}
	}
}
