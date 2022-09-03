
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
				components::transform::radius,
				components::graphics::color,
				const components::graphics::particle_info,
				const components::entity::lifetime> ecs,
				rynx::scheduler::task& task_context) {
			ecs.query().for_each_parallel(task_context, [](components::transform::radius& r, components::graphics::color& c, components::entity::lifetime lt, const components::graphics::particle_info& pi) {
				r.r = pi.radius_range(lt.quadratic());
				c.value = pi.color_range(lt.quadratic());
			});
		});

		context.add_task("particle emitter update",
			[dt](rynx::ecs::edit_view<
				const rynx::components::graphics::particle_emitter,
				rynx::components::graphics::particle_info,
				rynx::components::transform::position,
				rynx::components::transform::radius,
				rynx::components::transform::motion,
				rynx::components::entity::lifetime,
				rynx::components::graphics::color,
				rynx::components::transform::dampening,
				rynx::components::graphics::translucent,
				rynx::components::transform::ignore_gravity,
				rynx::components::transform::constant_force,
				rynx::components::transform::matrix> ecs) {
			ecs.query().for_each([&ecs, dt](rynx::components::transform::position pos, const rynx::components::graphics::particle_emitter& emitter) {
				int count = emitter.get_spawn_count(dt);

				for (int i = 0; i < count; ++i) {
					rynx::components::graphics::particle_info p_info;
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
						rynx::components::transform::radius(p_info.radius_range.begin),
						rynx::components::transform::motion(velocity, emitter.get_random()(-1.0f, +1.0f)),
						rynx::components::entity::lifetime(emitter.get_lifetime()),
						rynx::components::graphics::color(p_info.color_range.begin),
						rynx::components::transform::dampening{ emitter.get_linear_damping() },
						rynx::components::graphics::translucent(),
						rynx::components::transform::ignore_gravity(),
						rynx::components::transform::matrix(),
						rynx::components::transform::constant_force{ target_direction * upness }
					);
				}
				});
			});
		}