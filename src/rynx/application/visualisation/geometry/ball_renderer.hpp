#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/application/components.hpp>

#include <rynx/profiling/profiling.hpp>
#include <rynx/ecs/ecs.hpp>
#include <rynx/scheduler/context.hpp>

namespace rynx {
	namespace application {
		namespace visualisation {
			struct ball_renderer : public rynx::application::graphics_step::igraphics_step {
				ball_renderer(rynx::graphics::mesh* circleMesh, rynx::graphics::renderer* meshRenderer) {
					m_circleMesh = circleMesh;
					m_meshRenderer = meshRenderer;
					m_ropes = rynx::make_accumulator_shared_ptr<matrix4, floats4>();
				}

				struct buffer {
					size_t num;
					const rynx::graphics::mesh* mesh;
					const rynx::components::position* positions;
					const rynx::components::radius* radii;
					const rynx::components::color* colors;
					const rynx::components::transform_matrix* models;
					const rynx::graphics::texture_id* tex_ids;
				};

				virtual ~ball_renderer() {}
				
				virtual void prepare(rynx::scheduler::context* ctx) override {
					ctx->add_task("model matrices", [this](rynx::ecs& ecs) mutable {
						// rynx_profile("visualisation", "mesh matrices");
						m_bufs.clear();

						// collect buffers for drawing
						ecs.query()
							.notIn<rynx::components::mesh, rynx::components::boundary, rynx::components::translucent, rynx::components::frustum_culled, rynx::components::invisible>()
							.for_each_buffer([this](
								size_t num_entities,
								const rynx::components::position* positions,
								const rynx::components::radius* radii,
								const rynx::components::color* colors,
								const rynx::components::transform_matrix* models)
								{
									m_bufs.emplace_back(buffer{
										num_entities,
										m_circleMesh,
										positions,
										radii,
										colors,
										models
									});
								}
						);

						ecs.query()
							.notIn<rynx::components::mesh, rynx::components::boundary, rynx::components::frustum_culled, rynx::components::invisible>()
							.in<rynx::components::translucent>()
							.for_each_buffer([this](
								size_t num_entities,
								const rynx::components::position* positions,
								const rynx::components::radius* radii,
								const rynx::components::color* colors,
								const rynx::components::transform_matrix* models)
								{
									m_bufs.emplace_back(buffer{
										num_entities,
										m_circleMesh,
										positions,
										radii,
										colors,
										models
									});
								}
						);
					});

					ctx->add_task("rope matrices", [this](rynx::scheduler::task& task_context, const rynx::ecs& ecs) {
						{
							rynx_profile("visualisation", "model matrices");
							m_ropes->clear();
							ecs.query().notIn<rynx::components::invisible>().for_each_parallel(task_context, [this, &ecs](const rynx::components::phys::joint& rope) {
								if (!(ecs.exists(rope.a.id) & ecs.exists(rope.b.id))) {
									return;
								}

								auto entity_a = ecs[rope.a.id];
								auto entity_b = ecs[rope.b.id];

								const auto& pos_a = entity_a.get<const components::position>();
								const auto& pos_b = entity_b.get<const components::position>();
								auto relative_pos_a = math::rotatedXY(rope.a.pos, pos_a.angle);
								auto relative_pos_b = math::rotatedXY(rope.b.pos, pos_b.angle);
								auto world_pos_a = pos_a.value + relative_pos_a;
								auto world_pos_b = pos_b.value + relative_pos_b;
								vec3<float> mid = (world_pos_a + world_pos_b) * 0.5f;

								auto direction_vector = world_pos_a - world_pos_b;
								float length = direction_vector.length() * 0.5f;
								constexpr float width = 0.6f;

								matrix4 model;
								model.discardSetTranslate(mid.x, mid.y, 0.0f);
								model.rotate_2d(math::atan_approx(direction_vector.y / direction_vector.x));
								// model.rotate(math::atan_approx(direction_vector.y / direction_vector.x), 0, 0, 1);
								model.scale(length, width, 1.0f);
								
								m_ropes->emplace_back(model);

								float r = rope.cumulative_stress / (700.0f * rope.strength);
								m_ropes->emplace_back(floats4(r, 0.2f, 1.0f - r, 1));
							});
						}

					});
				}
				
				virtual void execute() override {
					{
						for (auto&& buf : m_bufs) {
							std::vector<rynx::graphics::texture_id> tex;
							tex.resize(buf.num);
							m_meshRenderer->drawMeshInstancedDeferred(
								*buf.mesh,
								buf.num,
								reinterpret_cast<const rynx::matrix4*>(buf.models),
								reinterpret_cast<const floats4*>(buf.colors),
								tex.data()
							);
						}
					}

					{
						rynx_profile("visualisation", "ball draw ropes");
						m_ropes->for_each([this](std::vector<matrix4>& matrices, std::vector<floats4>& colors) {
							std::vector<rynx::graphics::texture_id> tex;
							tex.resize(colors.size());
							m_meshRenderer->drawMeshInstancedDeferred(*m_circleMesh, matrices, colors, tex);
						});
					}
				}

				rynx::shared_ptr<rynx::parallel_accumulator<matrix4, floats4>> m_ropes;
				std::vector<buffer> m_bufs;

				rynx::graphics::renderer* m_meshRenderer;
				rynx::graphics::mesh* m_circleMesh;
			};
		}
	}
}
