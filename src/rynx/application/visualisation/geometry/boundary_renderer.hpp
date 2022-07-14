#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/application/components.hpp>
#include <rynx/tech/binary_config.hpp>

#include <rynx/tech/profiling.hpp>
#include <rynx/tech/ecs.hpp>

namespace rynx {
	namespace application {
		namespace visualisation {
			struct boundary_renderer : public rynx::application::graphics_step::igraphics_step {

				rynx::shared_ptr<rynx::binary_config::id> m_enabled;

				boundary_renderer(rynx::graphics::mesh_id boxMesh, rynx::graphics::renderer* meshRenderer) {
					m_boxMesh = boxMesh;
					m_meshRenderer = meshRenderer;
					m_edges = rynx::make_accumulator_shared_ptr<matrix4, floats4>();
				}
				
				virtual ~boundary_renderer() {}
				
				virtual void prepare(rynx::scheduler::context* ctx) override {
					rynx_profile("visualisation", "boundary_renderer");
					ctx->add_task("fetch polygon boundaries", [this](const rynx::ecs& ecs, rynx::scheduler::task& task_context) {
						m_edges->clear();
						
						auto query_conf = ecs.query();
						query_conf.notIn<components::frustum_culled, rynx::components::invisible>();
						if(m_enabled && !m_enabled->is_enabled()) {
							query_conf.notIn<rynx::components::mesh>();
						}
						query_conf.for_each_parallel(task_context, [this, &ecs](
							const rynx::components::boundary& m,
							const rynx::components::color& color)
							{
								for (size_t i = 0; i < m.segments_world.size(); ++i) {
									auto p1 = m.segments_world.vertex_position(i);
									auto p2 = m.segments_world.vertex_position(i + 1);
									vec3<float> mid = (p1 + p2) * 0.5f;

									matrix4 model_;
									model_.discardSetTranslate(mid.x, mid.y, mid.z);
									model_.rotate_2d(std::atan((p1.y - p2.y) / (p1.x - p2.x)));
									model_.scale((p1 - p2).length() * 0.5f, m_line_width * 0.5f, 1.0f);

									m_edges->emplace_back(model_);
									m_edges->emplace_back(color.value);
								}

								auto draw_point = [&](rynx::vec3f point, rynx::floats4 color, float radius) {
									rynx::matrix4 model;
									model.discardSetTranslate(point);
									model.scale(radius);
									m_edges->emplace_back(model);
									m_edges->emplace_back(color);
								};

								for (size_t i = 0; i < m.segments_world.size(); ++i) {
									draw_point(m.segments_world.vertex_position(i), rynx::floats4{ 1.0f, 1.0f, 1.0f, 1.0f }, m_line_width);
								}
							});
					});
				}
				
				virtual void execute() override {
					rynx_profile("visualisation", "debug poly boundaries");
					m_edges->for_each([this](std::vector<matrix4>& matrices, std::vector<floats4>& colors) {
						std::vector<rynx::graphics::texture_id> textures;
						textures.resize(matrices.size(), {});
						m_meshRenderer->drawMeshInstancedDeferred(m_boxMesh, matrices, colors, textures);
					});
				}

				float m_line_width = 1.5f;
				rynx::shared_ptr<rynx::parallel_accumulator<matrix4, floats4>> m_edges;
				rynx::graphics::renderer* m_meshRenderer;
				rynx::graphics::mesh_id m_boxMesh;
			};
		}
	}
}
