
#pragma once

#include <rynx/rulesets/particles.hpp>
#include <rynx/application/components.hpp>
#include <rynx/math/vector.hpp>

#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/barrier.hpp>
#include <rynx/math/matrix.hpp>

rynx::ruleset::particle_system::~particle_system() {}

void rynx::ruleset::particle_system::onFrameProcess(rynx::scheduler::context& context, float dt) {
		context.add_task("particle update",
			[](rynx::ecs::view<
				components::radius, components::color,
				const components::particle_info,
				const components::lifetime> ecs,
				rynx::scheduler::task& task_context) {
			ecs.query().for_each_parallel(task_context, [](components::radius& r, components::color& c, components::lifetime lt, const components::particle_info& pi) {
				r.r = pi.radius_range(lt.quadratic());
				c.value = pi.color_range(lt.quadratic());
			});
		});

		context.add_task("particle emitter update",
			[dt](rynx::ecs::edit_view<
				const rynx::components::particle_emitter,
				rynx::components::particle_info,
				rynx::components::position,
				rynx::components::radius,
				rynx::components::motion,
				rynx::components::lifetime,
				rynx::components::color,
				rynx::components::dampening,
				rynx::components::translucent,
				rynx::components::ignore_gravity,
				rynx::components::constant_force,
				rynx::matrix4> ecs) {
			ecs.query().for_each([&ecs, dt](rynx::components::position pos, const rynx::components::particle_emitter& emitter) {
				int count = emitter.get_spawn_count(dt);

				for (int i = 0; i < count; ++i) {
					rynx::components::particle_info p_info;
					p_info.color_range.begin = emitter.get_start_color();
					p_info.color_range.end = emitter.get_end_color();
					p_info.radius_range.begin = emitter.get_start_radius();
					p_info.radius_range.end = emitter.get_end_radius();

					rynx::vec3f velocity{ emitter.get_initial_velocity(), 0, 0 };
							
					float emitter_angle = emitter.rotate_with_host * pos.angle;
					rynx::math::rotateXY(velocity, emitter_angle + emitter.get_initial_angle());

					rynx::vec3f offset = rynx::math::rotatedXY(emitter.position_offset, emitter_angle);

					rynx::vec3f target_direction = emitter.get_constant_force();
					float upness = velocity.dot(target_direction) / velocity.length();
					upness = upness * upness * upness * upness;

					auto particle_pos = pos;
					particle_pos.value += offset;

					// in general creating new entities is not safe inside a query, but now we know that we are not touching the same entity categories,
					// because we iterated over particle_emitter components, and we are only creating entities that do not have particle_emitter component.
					ecs.create(
						p_info,
						particle_pos,
						rynx::components::radius(p_info.radius_range.begin),
						rynx::components::motion(velocity, emitter.get_random()(-1.0f, +1.0f)),
						rynx::components::lifetime(emitter.get_lifetime()),
						rynx::components::color(p_info.color_range.begin),
						rynx::components::dampening{ emitter.get_linear_damping() },
						rynx::components::translucent(),
						rynx::components::ignore_gravity(),
						rynx::matrix4(),
						rynx::components::constant_force{ target_direction * upness }
					);
				}
				});
			});
		}