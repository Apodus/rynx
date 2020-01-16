#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/application/components.hpp>

#include <rynx/tech/profiling.hpp>


// required for implementation
#include <rynx/tech/ecs.hpp>
#include <rynx/tech/math/matrix.hpp>
#include <rynx/tech/math/vector.hpp>

namespace rynx {
	namespace application {
		namespace visualisation {
			struct mesh_renderer : public rynx::application::graphics_step::igraphics_step {
			private:
				struct buffer {
					size_t num;
					const Mesh* mesh;
					const rynx::components::position* positions;
					const rynx::components::radius* radii;
					const rynx::components::color* colors;
					const rynx::matrix4* models;
				};

			public:

				mesh_renderer(MeshRenderer* meshRenderer) {
					m_meshRenderer = meshRenderer;
				}
				virtual ~mesh_renderer() {}
				
				virtual void prepare(rynx::scheduler::context* ctx) override {
					ctx->add_task("model matrices", [this](rynx::scheduler::task& task_context, rynx::ecs& ecs) mutable {
						// rynx_profile("visualisation", "mesh matrices");
						m_bufs.clear();

						// update model matrices
						ecs.query().notIn<rynx::components::translucent>()
							.for_each_parallel(task_context, [this](
								rynx::components::position pos,
								rynx::components::radius r,
								rynx::matrix4& model)
								{
									model.discardSetTranslate(pos.value);
									model.scale(r.r);
									model.rotate_2d(pos.angle);
								});
						
						// collect buffers for drawing
						ecs.query().notIn<rynx::components::translucent>()
							.for_each_buffer([this](
								size_t num_entities,
								const rynx::components::mesh* meshes,
								const rynx::components::position* positions,
								const rynx::components::radius* radii,
								const rynx::components::color* colors,
								const rynx::matrix4* models)
								{
									m_bufs.emplace_back(buffer{
										num_entities,
										meshes[0].m,
										positions,
										radii,
										colors,
										models
									});
								});
					});
				}
				
				virtual void execute() override {
					for(auto&& buf : m_bufs)
						m_meshRenderer->drawMeshInstancedDeferred(*buf.mesh, "Empty", buf.num, buf.models, reinterpret_cast<const floats4*>(buf.colors));
				}

			private:
				std::vector<buffer> m_bufs;
				MeshRenderer* m_meshRenderer;
			};
		}
	}
}
