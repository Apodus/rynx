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

					m_balls = rynx::make_accumulator_shared_ptr<matrix4, floats4>();
					m_balls_translucent = rynx::make_accumulator_shared_ptr<matrix4, floats4>();
					m_ropes = rynx::make_accumulator_shared_ptr<matrix4, floats4>();
				}

				bool inScreen(vec3<float> p, float r) {
					bool out = p.x + r < cameraLeft;
					out |= p.x - r > cameraRight;
					out |= p.y + r > cameraTop;
					out |= p.y - r < cameraBot;
					return !out;
				}

				virtual ~ball_renderer() {}
				
				virtual void prepare(rynx::scheduler::context* ctx) override {
					ctx->add_task("model matrices", [this](rynx::scheduler::task& task_context, rynx::ecs& ecs) mutable {
						rynx_profile("visualisation", "model matrices");
						
						cameraLeft = m_camera->position().x - m_camera->position().z;
						cameraRight = m_camera->position().x + m_camera->position().z;
						cameraTop = m_camera->position().y + m_camera->position().z;
						cameraBot = m_camera->position().y - m_camera->position().z;
						
						m_balls->clear();
						m_balls_translucent->clear();
						
						/*
						auto ids = ecs.query().notIn<rynx::components::boundary, rynx::components::mesh, rynx::components::translucent>()
							.in<rynx::components::position, rynx::components::radius, rynx::components::color>()
							.gather();

						for (auto id : ids) {
							ecs.attachToEntity(id, matrix4());
						}
						*/

						ecs.query().notIn<rynx::components::boundary, rynx::components::mesh, rynx::components::translucent>()
							.for_each_parallel(task_context, [this] (
								const rynx::components::position& pos,
								const rynx::components::radius& r,
								const rynx::components::color& color)
								{
									if (!inScreen(pos.value, r.r))
										return;

									matrix4 model;
									model.discardSetTranslate(pos.value);
									model.scale(r.r);
									model.rotate_2d(pos.angle);
									// model.rotate(pos.angle, 0, 0, 1);

									m_balls->emplace_back(model);
									m_balls->emplace_back(color.value);
								});

						ecs.query().notIn<rynx::components::boundary, rynx::components::mesh>().in<rynx::components::translucent>()
							.for_each_parallel(task_context, [this](
								const rynx::components::position& pos,
								const rynx::components::radius& r,
								const rynx::components::color& color)
								{
									if (!inScreen(pos.value, r.r))
										return;

									matrix4 model;
									model.discardSetTranslate(pos.value);
									model.scale(r.r);
									model.rotate_2d(pos.angle);
									// model.rotate(pos.angle, 0, 0, 1);

									m_balls_translucent->emplace_back(model);
									m_balls_translucent->emplace_back(color.value);
								});
					});

					ctx->add_task("rope matrices", [this](rynx::scheduler::task& task_context, const rynx::ecs& ecs) {
						{
							rynx_profile("visualisation", "model matrices");
							m_ropes->clear();
							ecs.query().for_each_parallel(task_context, [this, &ecs](const rynx::components::rope& rope) {
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
								model.rotate_2d(math::atan_approx(direction_vector.y / direction_vector.x));
								// model.rotate(math::atan_approx(direction_vector.y / direction_vector.x), 0, 0, 1);
								model.scale(length, width, 1.0f);
								
								m_ropes->emplace_back(model);
								m_ropes->emplace_back(floats4(rope.cumulative_stress / (1500.0f * rope.strength), 0, 0, 1));
							});
						}

					});
				}
				
				virtual void render() override {
					{
						rynx_profile("visualisation", "ball draw solids");
						m_balls->for_each([this](std::vector<matrix4>& matrices, std::vector<floats4>& colors) {
							m_meshRenderer->drawMeshInstanced(*m_circleMesh, "Empty", matrices, colors);
						});
					}

					{
						rynx_profile("visualisation", "ball draw ropes");
						m_ropes->for_each([this](std::vector<matrix4>& matrices, std::vector<floats4>& colors) {
							m_meshRenderer->drawMeshInstanced(*m_circleMesh, "Empty", matrices, colors);
						});
					}

					{
						rynx_profile("visualisation", "ball draw translucent");
						m_balls_translucent->for_each([this](std::vector<matrix4>& matrices, std::vector<floats4>& colors) {
							m_meshRenderer->drawMeshInstanced(*m_circleMesh, "Empty", matrices, colors);
						});
					}
				}

				// Assumes axis aligned top-down camera :(
				float cameraLeft;
				float cameraRight;
				float cameraTop;
				float cameraBot;

				std::shared_ptr<rynx::parallel_accumulator<matrix4, floats4>> m_balls;
				std::shared_ptr<rynx::parallel_accumulator<matrix4, floats4>> m_balls_translucent;
				std::shared_ptr<rynx::parallel_accumulator<matrix4, floats4>> m_ropes;
				
				MeshRenderer* m_meshRenderer;
				Mesh* m_circleMesh;
				Camera* m_camera;
			};
		}
	}
}
