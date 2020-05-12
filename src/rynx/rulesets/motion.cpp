
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
			auto apply_acceleration_to_velocity = ecs.query().for_each_parallel(task_context, [dt](components::position& p, components::motion& m) {

				auto linear_acceleration_effect = m.acceleration * dt * dt + m.velocity * dt;
				auto angular_acceleration_effect = m.angularAcceleration * dt * dt + m.angularVelocity * dt;

				// update velocity
				m.velocity += m.acceleration * dt;
				m.angularVelocity += m.angularAcceleration * dt;

				m.acceleration.set(0, 0, 0);
				m.angularAcceleration = 0;

				// update position
				p.value += linear_acceleration_effect;
				p.angle += angular_acceleration_effect;
			});
		}
	);

	context.add_task("Apply dampening", [dt](rynx::scheduler::task& task, rynx::ecs::view<components::motion, const components::dampening> ecs) {
		ecs.query().for_each_parallel(task, [dt](components::motion& m, components::dampening d) {
			m.velocity *= ::powf(1.0f - d.linearDampening, dt);
			m.angularVelocity *= ::powf(1.0f - d.angularDampening, dt);
		});
	}).required_for(position_updates);

	context.add_task("Gravity", [this](rynx::ecs::view<components::motion, const components::ignore_gravity> ecs, rynx::scheduler::task& task) {
		if (m_gravity.length_squared() > 0) {
			ecs.query().notIn<components::ignore_gravity>().for_each_parallel(task, [this](components::motion& m) {
				m.acceleration += m_gravity;
			});
		}
	});
}
