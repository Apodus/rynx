#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/application/components.hpp>

#include <rynx/tech/profiling.hpp>
#include <rynx/tech/ecs.hpp>

namespace rynx {
	namespace application {
		namespace visualisation {
			struct boundary_renderer : public rynx::application::graphics_step::igraphics_step {

				boundary_renderer(Mesh* boxMesh, MeshRenderer* meshRenderer) {
					m_boxMesh = boxMesh;
					m_meshRenderer = meshRenderer;
					m_edges = rynx::make_accumulator_shared_ptr<matrix4, floats4>();
				}
				
				virtual ~boundary_renderer() {}
				
				virtual void prepare(rynx::scheduler::context* ctx) override {
					rynx_profile("visualisation", "boundary_renderer");
					ctx->add_task("fetch polygon boundaries", [this](const rynx::ecs& ecs, rynx::scheduler::task& task_context) {
						m_edges->clear();
						
						ecs.query().notIn<rynx::components::mesh, rynx::components::translucent, components::frustum_culled>()
							.for_each_parallel(task_context, [this](
								const rynx::components::boundary& m,
								const rynx::components::color& color)
								{
									for (auto&& edge : m.segments_world) {
										vec3<float> mid = (edge.p1 + edge.p2) * 0.5f;

										matrix4 model_;
										model_.discardSetTranslate(mid.x, mid.y, mid.z);
										model_.rotate_2d(std::atan((edge.p1.y - edge.p2.y) / (edge.p1.x - edge.p2.x)));
										// model_.rotate_2d(math::atan_approx((edge.p1.y - edge.p2.y) / (edge.p1.x - edge.p2.x)));
										// model_.rotate(math::atan_approx((p1.y - p2.y) / (p1.x - p2.x)), 0, 0, 1);
										model_.scale((edge.p1 - edge.p2).length(), m_line_width * 0.5f, 1.0f);

										m_edges->emplace_back(model_);
										m_edges->emplace_back(color.value);
									}
								});
					});
				}
				
				virtual void execute() override {
					rynx_profile("visualisation", "ball draw solids");
					m_edges->for_each([this](std::vector<matrix4>& matrices, std::vector<floats4>& colors) {
						m_meshRenderer->drawMeshInstancedDeferred(*m_boxMesh, "Empty", matrices, colors);
					});
				}

				float m_line_width = 5.5f;
				std::shared_ptr<rynx::parallel_accumulator<matrix4, floats4>> m_edges;
				MeshRenderer* m_meshRenderer;
				Mesh* m_boxMesh;
			};
		}
	}
}
