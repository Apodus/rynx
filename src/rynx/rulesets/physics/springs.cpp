

#include <rynx/rulesets/physics/springs.hpp>
#include <rynx/application/components.hpp>
#include <rynx/scheduler/context.hpp>

void rynx::ruleset::physics::springs::onFrameProcess(rynx::scheduler::context& context, float dt) {

	context.add_task("physical springs", [dt](
		rynx::ecs::view<
		components::phys::joint,
		const components::physical_body,
		const components::position,
		components::motion> ecs,
		rynx::scheduler::task& task)
		{
			struct common_values {

				rynx::components::position pos_a;
				rynx::components::position pos_b;

				components::motion* mot_a;
				components::motion* mot_b;

				const components::physical_body* phys_a;
				const components::physical_body* phys_b;

				vec3f relative_pos_a;
				vec3f relative_pos_b;

				vec3f world_pos_a;
				vec3f world_pos_b;

				void init(components::phys::joint& rope, rynx::ecs::view<const components::position, const components::physical_body, components::motion> ecs) {
					auto entity_a = ecs[rope.id_a];
					auto entity_b = ecs[rope.id_b];

					pos_a = entity_a.get<const components::position>();
					pos_b = entity_b.get<const components::position>();

					relative_pos_a = math::rotatedXY(rope.point_a, pos_a.angle);
					relative_pos_b = math::rotatedXY(rope.point_b, pos_b.angle);
					world_pos_a = pos_a.value + relative_pos_a;
					world_pos_b = pos_b.value + relative_pos_b;

					mot_a = entity_a.try_get<components::motion>();
					mot_b = entity_b.try_get<components::motion>();

					phys_a = &entity_a.get<const components::physical_body>();
					phys_b = &entity_b.get<const components::physical_body>();
				}

				void apply_force(vec3f force, components::phys::joint::joint_type joint_type) {
					apply_linear_force(force);
					if (joint_type == components::phys::joint::joint_type::Free) {
						apply_rotational_force(force);
					}
				}

				void apply_linear_force(vec3f force) {
					mot_a->acceleration += force * phys_a->inv_mass;
					mot_b->acceleration -= force * phys_b->inv_mass;
				}

				void apply_rotational_force(vec3f force) {
					mot_a->angularAcceleration += force.dot(relative_pos_a.normal2d()) * phys_a->inv_moment_of_inertia;
					mot_b->angularAcceleration += -force.dot(relative_pos_b.normal2d()) * phys_b->inv_moment_of_inertia;
				}

				vec3f get_force_direction() const {
					auto direction_a_to_b = world_pos_b - world_pos_a;
					return direction_a_to_b.normalize();
				}

				float get_distance() const {
					return (world_pos_a - world_pos_b).length();
				}

				float get_distance_sqr() const {
					return (world_pos_a - world_pos_b).length_squared();
				}
			};

			float stress_decay = std::pow(0.3f, dt);

			for (int i = 0; i < 10; ++i) {
				auto generate_springs_work = ecs.query().for_each_parallel(task, [dt, stress_decay, ecs](components::phys::joint& rope) mutable {
					common_values joint_data;
					joint_data.init(rope, ecs);

					bool connector_spring = rope.connector == components::phys::joint::connector_type::Spring;
					bool connector_rubber_band = rope.connector == components::phys::joint::connector_type::RubberBand;
					bool connector_rod = rope.connector == components::phys::joint::connector_type::Rod;
					float over_extension = joint_data.get_distance() - rope.length;
					if (connector_rubber_band | connector_spring) {

						// rubber band doesn't push back. spring does.
						over_extension -= (connector_rubber_band & (over_extension < 0)) * over_extension;

						float force = 0.5f * 1200.0f * rope.strength * over_extension;
						rope.cumulative_stress += force * dt;

						vec3f force_vector = joint_data.get_force_direction() * force;
						joint_data.apply_force(force_vector, rope.m_joint);
					}
					else if (connector_rod) {
						auto vel_a = joint_data.mot_a->velocity_at_point_predict(joint_data.relative_pos_a, dt);
						auto vel_b = joint_data.mot_b->velocity_at_point_predict(joint_data.relative_pos_b, dt);
						auto force_dir = joint_data.get_force_direction();
						auto rel_vel = vel_a - vel_b;
						float current_agreement = rel_vel.dot(force_dir);
						float multiplier = rope.strength * 20450.0f * (over_extension - 0.010805f * current_agreement);
						rope.cumulative_stress += multiplier * dt;
						joint_data.apply_force(force_dir * multiplier, rope.m_joint);
					}

					rope.cumulative_stress *= stress_decay;
				});
			}
		});
}