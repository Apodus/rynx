#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/application/components.hpp>

#include <rynx/profiling/profiling.hpp>


// required for implementation
#include <rynx/ecs/ecs.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>

namespace rynx {
	namespace application {
		namespace visualisation {
			struct mesh_renderer : public rynx::application::graphics_step::igraphics_step {
			private:
				static_assert(sizeof(rynx::components::transform::matrix) == sizeof(rynx::matrix4), "matrices need to be in consecutive memory");

				struct buffer {
					size_t num;
					const rynx::graphics::mesh* mesh;
					const rynx::components::transform::position* positions;
					const rynx::components::transform::radius* radii;
					const rynx::components::graphics::color* colors;
					const rynx::components::transform::matrix* models;
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
							.notIn<rynx::components::graphics::translucent, components::graphics::frustum_culled, rynx::components::graphics::invisible>()
							.for_each_buffer([this](
								size_t num_entities,
								const rynx::components::graphics::mesh* meshes,
								const rynx::components::transform::position* positions,
								const rynx::components::transform::radius* radii,
								const rynx::components::graphics::color* colors,
								const rynx::components::transform::matrix* models,
								const rynx::graphics::texture_id* tex_ids)
								{
									auto* mesh = m_meshes->get(meshes[0].m);
									m_bufs.emplace_back(buffer{
										num_entities,
										mesh,
										positions,
										radii,
										colors,
										models,
										tex_ids
									});
								});

						ecs.query()
							.notIn<components::graphics::frustum_culled, rynx::components::graphics::invisible>()
							.in<rynx::components::graphics::translucent>()
							.for_each_buffer([this](
								size_t num_entities,
								const rynx::components::graphics::mesh* meshes,
								const rynx::components::transform::position* positions,
								const rynx::components::transform::radius* radii,
								const rynx::components::graphics::color* colors,
								const rynx::components::transform::matrix* models,
								const rynx::graphics::texture_id* tex_ids)
								{
									auto* mesh = m_meshes->get(meshes[0].m);
									m_bufs.emplace_back(buffer{
										num_entities,
										mesh,
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
					std::sort(m_bufs.begin(), m_bufs.end(), [](const auto& a, const auto& b) -> bool { return a.positions[0].value.z < b.positions[0].value.z; });
					for (auto&& buf : m_bufs) {
						m_meshRenderer->drawMeshInstancedDeferred(
							*buf.mesh,
							buf.num,
							reinterpret_cast<const rynx::matrix4*>(buf.models),
							reinterpret_cast<const floats4*>(buf.colors),
							buf.tex_ids
						);
					}
				}

			private:
				std::vector<buffer> m_bufs;
				rynx::graphics::renderer* m_meshRenderer;
				rynx::graphics::mesh_collection* m_meshes;
			};
		}
	}
}
