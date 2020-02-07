
#include <rynx/rulesets/motion.hpp>

#include <rynx/application/components.hpp>

#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/barrier.hpp>

void rynx::ruleset::motion_updates::onFrameProcess(rynx::scheduler::context& context, float dt) {
	auto position_updates = context.add_task("Apply acceleration and reset", [dt](
		rynx::ecs::view<components::motion, components::position> ecs,
		rynx::scheduler::task& task_context)
		{
			{
				rynx_profile("Motion", "update velocity");
				ecs.query().for_each_parallel(task_context, [dt](components::motion& m) {
					m.velocity += m.acceleration * dt;
					m.acceleration.set(0, 0, 0);
					m.angularVelocity += m.angularAcceleration * dt;
					m.angularAcceleration = 0;
				});
			}

			{
				rynx_profile("Motion", "update position");
				ecs.query().for_each_parallel(task_context, [dt](components::position& p, const components::motion& m) {
					p.value += m.velocity * dt;
					p.angle += m.angularVelocity * dt;
				});
			}
		}
	);

	context.add_task("Apply dampening", [dt](rynx::scheduler::task& task, rynx::ecs::view<components::motion, const components::dampening> ecs) {
		ecs.query().for_each_parallel(task, [dt](components::motion& m, components::dampening d) {
			m.velocity *= ::powf(d.linearDampening, dt);
			m.angularVelocity *= ::powf(d.angularDampening, dt);
			});
		}
	)->depends_on(position_updates);

	context.add_task("Gravity", [this](rynx::ecs::view<components::motion, const components::ignore_gravity> ecs, rynx::scheduler::task& task) {
		if (m_gravity.lengthSquared() > 0) {
			ecs.query().notIn<components::ignore_gravity>().for_each_parallel(task, [this](components::motion& m) {
				m.acceleration += m_gravity;
			});
		}
	});
}
