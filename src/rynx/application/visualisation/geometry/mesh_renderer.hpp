#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/application/components.hpp>

#include <rynx/tech/profiling.hpp>


// required for implementation
#include <rynx/tech/ecs.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>

namespace rynx {
	namespace application {
		namespace visualisation {
			struct mesh_renderer : public rynx::application::graphics_step::igraphics_step {
			private:
				struct buffer {
					size_t num;
					const rynx::graphics::mesh* mesh;
					const rynx::components::position* positions;
					const rynx::components::radius* radii;
					const rynx::components::color* colors;
					const rynx::matrix4* models;
					const rynx::graphics::texture_id* tex_ids;
				};

			public:

				mesh_renderer(rynx::graphics::renderer* meshRenderer) {
					m_meshRenderer = meshRenderer;
					m_meshes = m_meshRenderer->meshes().get();
				}
				virtual ~mesh_renderer() {}
				
				virtual void prepare(rynx::scheduler::context* ctx) override {
					ctx->add_task("model matrices", [this](rynx::ecs& ecs) mutable {
						m_bufs.clear();
						
						// collect buffers for drawing, first solids, then translucents.
						ecs.query()
							.notIn<rynx::components::translucent, components::frustum_culled, rynx::components::invisible>()
							.for_each_buffer([this](
								size_t num_entities,
								const rynx::components::mesh* meshes,
								const rynx::components::position* positions,
								const rynx::components::radius* radii,
								const rynx::components::color* colors,
								const rynx::matrix4* models,
								const rynx::graphics::texture_id* tex_ids)
								{
									m_bufs.emplace_back(buffer{
										num_entities,
										m_meshes->get(meshes[0].m),
										positions,
										radii,
										colors,
										models,
										tex_ids
									});
								});

						ecs.query()
							.notIn<components::frustum_culled, rynx::components::invisible>()
							.in<rynx::components::translucent>()
							.for_each_buffer([this](
								size_t num_entities,
								const rynx::components::mesh* meshes,
								const rynx::components::position* positions,
								const rynx::components::radius* radii,
								const rynx::components::color* colors,
								const rynx::matrix4* models,
								const rynx::graphics::texture_id* tex_ids)
								{
									m_bufs.emplace_back(buffer{
										num_entities,
										m_meshes->get(meshes[0].m),
										positions,
										radii,
										colors,
										models,
										tex_ids
									});
								});
					});
				}
				
				virtual void execute() override {
					for(auto&& buf : m_bufs)
						m_meshRenderer->drawMeshInstancedDeferred(*buf.mesh, buf.num, buf.models, reinterpret_cast<const floats4*>(buf.colors), buf.tex_ids);
				}

			private:
				std::vector<buffer> m_bufs;
				rynx::graphics::renderer* m_meshRenderer;
				rynx::graphics::mesh_collection* m_meshes;
			};
		}
	}
}
