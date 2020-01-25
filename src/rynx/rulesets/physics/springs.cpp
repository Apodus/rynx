

#include <rynx/rulesets/physics/springs.hpp>
#include <rynx/application/components.hpp>
#include <rynx/scheduler/context.hpp>

void rynx::ruleset::physics::springs::onFrameProcess(rynx::scheduler::context& context, float dt) {

	context.add_task("physical springs", [dt](rynx::ecs::view<components::rope, const components::physical_body, const components::position, components::motion> ecs, rynx::scheduler::task& task) {
		auto broken_ropes = rynx::make_accumulator_shared_ptr<rynx::ecs::id>();
		ecs.query().for_each_parallel(task, [broken_ropes, dt, ecs](rynx::ecs::id id, components::rope& rope) mutable {
			auto entity_a = ecs[rope.id_a];
			auto entity_b = ecs[rope.id_b];

			auto pos_a = entity_a.get<components::position>();
			auto pos_b = entity_b.get<components::position>();

			auto& mot_a = entity_a.get<components::motion>();
			auto& mot_b = entity_b.get<components::motion>();

			const auto& phys_a = entity_a.get<components::physical_body>();
			const auto& phys_b = entity_b.get<components::physical_body>();

			auto relative_pos_a = math::rotatedXY(rope.point_a, pos_a.angle);
			auto relative_pos_b = math::rotatedXY(rope.point_b, pos_b.angle);
			auto world_pos_a = pos_a.value + relative_pos_a;
			auto world_pos_b = pos_b.value + relative_pos_b;

			auto length = (world_pos_a - world_pos_b).length();
			float over_extension = (length - rope.length);

			over_extension -= (over_extension < 0) * over_extension;

			// way too much extension == snap instantly
			if (over_extension > 3.0f * rope.length) {
				broken_ropes->emplace_back(id);
				return;
			}

			float force = 540.0f * rope.strength * over_extension;

			rope.cumulative_stress += force * dt;
			rope.cumulative_stress *= std::pow(0.3f, dt);

			// Remove rope if too much strain
			if (rope.cumulative_stress > 700.0f * rope.strength)
				broken_ropes->emplace_back(id);

			auto direction_a_to_b = world_pos_b - world_pos_a;
			direction_a_to_b.normalize();
			direction_a_to_b *= force;

			mot_a.acceleration += direction_a_to_b * phys_a.inv_mass;
			mot_b.acceleration -= direction_a_to_b * phys_b.inv_mass;

			mot_a.angularAcceleration += direction_a_to_b.dot(relative_pos_a.normal2d()) * phys_a.inv_moment_of_inertia;
			mot_b.angularAcceleration += -direction_a_to_b.dot(relative_pos_b.normal2d()) * phys_b.inv_moment_of_inertia;
		});

		if (!broken_ropes->empty()) {
			task.extend_task([broken_ropes](rynx::ecs& ecs) {
				broken_ropes->for_each([&ecs](std::vector<rynx::ecs::id>& id_vector) mutable {
					for (auto&& id : id_vector) {
						ecs.attachToEntity(id, components::dead());


						components::rope& rope = ecs[id].get<components::rope>();
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
		}
	});
}