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
			struct model_matrix_updates : public rynx::application::graphics_step::igraphics_step {
				model_matrix_updates() {}
				virtual ~model_matrix_updates() {}

				virtual void prepare(rynx::scheduler::context* ctx) override {
					ctx->add_task("model matrices", [this](rynx::scheduler::task& task_context, rynx::ecs& ecs) mutable {

						// update model matrices
						ecs.query().notIn<components::graphics::frustum_culled, components::graphics::invisible>()
							.for_each_parallel(task_context, [this](
								rynx::components::transform::position pos,
								rynx::components::transform::radius r,
								rynx::components::transform::matrix& transform_matrix)
								{
									auto& model = reinterpret_cast<rynx::matrix4&>(transform_matrix);
									model.discardSetTranslate(pos.value);
									model.rotate_2d(pos.angle);
									model.scale(r.r);
								}
						);
					});
				}

				virtual void execute() {}
			};
		}
	}
}