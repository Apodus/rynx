
#include <rynx/rulesets/motion.hpp>

#include <rynx/application/components.hpp>

#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/barrier.hpp>

void rynx::ruleset::motion_updates::onFrameProcess(rynx::scheduler::context& context, float dt) {
	auto position_updates = context.add_task("Motion update", [dt](
		rynx::ecs::view<const components::constant_force, components::motion, components::position, components::position_relative> ecs,
		rynx::scheduler::task& task_context)
		{
			auto apply_acceleration_to_velocity = ecs.query().for_each_parallel(task_context, [dt](components::position& p, components::motion& m) {

				auto delta_position = m.acceleration * dt * dt + m.velocity * dt;
				auto delta_orientation = m.angularAcceleration * dt * dt + m.angularVelocity * dt;

				// update velocity
				m.velocity += m.acceleration * dt;
				m.angularVelocity += m.angularAcceleration * dt;

				m.acceleration.set(0, 0, 0);
				m.angularAcceleration = 0;

				// update position
				p.value += delta_position;
				p.angle += delta_orientation;
			});

			auto apply_constant_forces = ecs.query().for_each_parallel(task_context, [dt](rynx::components::motion& m, const rynx::components::constant_force force) {
				m.acceleration += force.force * dt;
			});

			// NOTE: To prevent constant forces being applied before acceleration is reset by velocity update.
			// The order of the tasks doesn't really matter. They just cant run at the same time.
			apply_constant_forces.task()->depends_on(*apply_acceleration_to_velocity.task());

			ecs.query().for_each_parallel(task_context, [ecs](rynx::components::position& pos, rynx::components::position_relative relative_pos) {
				if (!ecs.exists(relative_pos.id))
					return;
				
				const auto& host_pos = ecs[relative_pos.id].get<rynx::components::position>();
				pos.value = host_pos.value + rynx::math::rotatedXY(relative_pos.pos, host_pos.angle);
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
