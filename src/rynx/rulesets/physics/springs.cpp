

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

				mot_a = &entity_a.get<components::motion>();
				mot_b = &entity_b.get<components::motion>();

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
		auto broken_ropes = rynx::make_accumulator_shared_ptr<rynx::ecs::id>();
		auto generate_springs_work = ecs.query().for_each_parallel(task, [broken_ropes, dt, stress_decay, ecs](rynx::ecs::id id, components::phys::joint& rope) mutable {
			common_values joint_data;
			joint_data.init(rope, ecs);

			bool connector_spring = rope.connector == components::phys::joint::connector_type::Spring;
			bool connector_rubber_band = rope.connector == components::phys::joint::connector_type::RubberBand;
			bool connector_rod = rope.connector == components::phys::joint::connector_type::Rod;
			float over_extension = joint_data.get_distance() - rope.length;
			if (connector_rubber_band | connector_spring) {
				
				// rubber band doesn't push back. spring does.
				over_extension -= (connector_rubber_band & (over_extension < 0)) * over_extension;
			
				float force = 0.5f * 540.0f * rope.strength * over_extension;
				rope.cumulative_stress += force * dt;

				vec3f force_vector = joint_data.get_force_direction() * force;
				joint_data.apply_force(force_vector, rope.m_joint);
			}
			else if (connector_rod) {
				auto vel_a = joint_data.mot_a->velocity; // joint_data.mot_a->velocity_at_point(joint_data.relative_pos_a);
				auto vel_b = joint_data.mot_b->velocity; // joint_data.mot_b->velocity_at_point(joint_data.relative_pos_b);
				auto force_dir = joint_data.get_force_direction();
				auto rel_vel = vel_a - vel_b;
				float current_agreement = rel_vel.dot(force_dir);
				float multiplier = 10450.0f * (over_extension - 0.0055f * current_agreement);
				rope.cumulative_stress += multiplier * dt;
				joint_data.apply_force(force_dir * multiplier, rope.m_joint);
			}

			rope.cumulative_stress *= stress_decay;

			// way too much extension == snap instantly
			if (over_extension > 3.0f * rope.length) {
				broken_ropes->emplace_back(id);
			}
			// Remove rope if too much strain
			else if (rope.cumulative_stress > 700.0f * rope.strength) {
				broken_ropes->emplace_back(id);
			}

		});

		auto check_broken_ropes_task = task.extend_task_independent([broken_ropes](rynx::ecs& ecs) {
			broken_ropes->for_each([&ecs](std::vector<rynx::ecs::id>& id_vector) mutable {
				for (auto&& id : id_vector) {
					ecs.attachToEntity(id, components::dead());

					components::phys::joint& rope = ecs[id].get<components::phys::joint>();
					auto pos1 = ecs[rope.id_a].get<components::position>().value;
					auto pos2 = ecs[rope.id_b].get<components::position>().value;

					auto v1 = ecs[rope.id_a].get<components::motion>().velocity;
					auto v2 = ecs[rope.id_b].get<components::motion>().velocity;

					math::rand64 random;

					constexpr int num_particles = 40;

					std::vector<rynx::components::position> pos_v(num_particles);
					std::vector<rynx::components::radius> radius_v(num_particles);
					std::vector<rynx::components::motion> motion_v(num_particles);
					std::vector<rynx::components::lifetime> lifetime_v(num_particles);
					std::vector<rynx::components::color> color_v(num_particles);
					std::vector<rynx::components::dampening> dampening_v(num_particles);
					std::vector<rynx::components::particle_info> particle_info_v(num_particles);

					for (int i = 0; i < 40; ++i) {
						float value = static_cast<float>(i) / 10.0f;
						auto pos = pos1 + (pos2 - pos1) * value;

						rynx::components::particle_info p_info;
						float weight = random(1.0f, 5.0f);

						auto v = v1 + (v2 - v1) * value;
						v += vec3<float>(random(-0.5f, +0.5f) / weight, random(-0.5f, +0.5f) / weight, 0);

						float end_c = random(0.6f, 0.99f);
						float start_c = random(0.0f, 0.3f);

						p_info.color.begin = vec4<float>(start_c, start_c, start_c, 1);
						p_info.color.end = vec4<float>(end_c, end_c, end_c, 0);
						p_info.radius.begin = weight * 0.25f;
						p_info.radius.end = weight * 0.50f;

						pos_v[i] = rynx::components::position(pos);
						radius_v[i] = rynx::components::radius(p_info.radius.begin);
						motion_v[i] = rynx::components::motion(v, 0);
						lifetime_v[i] = rynx::components::lifetime(0.33f + 0.33f * weight);
						color_v[i] = rynx::components::color(p_info.color.begin);
						dampening_v[i] = rynx::components::dampening{ 0.95f + 0.03f * (1.0f / weight), 1 };
						particle_info_v[i] = p_info;
					}

					ecs.create_n<rynx::components::translucent>(
						pos_v,
						radius_v,
						motion_v,
						lifetime_v,
						color_v,
						dampening_v,
						particle_info_v
					);
				}
			});
		});

		check_broken_ropes_task.depends_on(generate_springs_work);
	});
}