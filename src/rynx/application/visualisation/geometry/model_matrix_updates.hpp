#pragma once


#include <rynx/system/assert.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/application/components.hpp>

#include <rynx/tech/profiling.hpp>
#include <rynx/tech/ecs.hpp>
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
						ecs.query().notIn<components::frustum_culled, components::invisible>()
							.for_each_parallel(task_context, [this](
								rynx::components::position pos,
								rynx::components::radius r,
								rynx::matrix4& model)
								{
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