
#pragma once

#include <rynx/application/components.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/math/vector.hpp>

#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/barrier.hpp>

namespace rynx {
	namespace ruleset {
		class particle_system : public application::logic::iruleset {
		public:
			virtual ~particle_system() {}
			virtual void onFrameProcess(rynx::scheduler::context& context, float /* dt */) override {
				context.add_task("particle update",
					[](rynx::ecs::view<
						components::radius, components::color,
						const components::particle_info,
						const components::lifetime> ecs,
						rynx::scheduler::task& task_context) {
					ecs.query().for_each_parallel(task_context, [](components::radius& r, components::color& c, components::lifetime lt, const components::particle_info& pi) {
						r.r = pi.radius.linear(lt);
						c.value = pi.color.linear(lt.inv_quadratic_inv());
					});
				});
			}
		};
	}
}