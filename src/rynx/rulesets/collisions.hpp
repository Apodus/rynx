#pragma once

#include <rynx/tech/object_storage.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/application/components.hpp>
#include <rynx/tech/collision_detection.hpp>

#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <rynx/tech/parallel_accumulator.hpp>
#include <rynx/tech/unordered_set.hpp>

namespace rynx {
	namespace ruleset {
		class physics_2d : public application::logic::iruleset {
			vec3<float> m_gravity;
		public:
			struct collision_event {
				uint64_t a_id;
				uint64_t b_id;
				rynx::components::physical_body a_body;
				rynx::components::physical_body b_body;
				rynx::components::motion* a_motion;
				rynx::components::motion* b_motion;
				vec3<float> a_pos;
				vec3<float> b_pos;
				vec3<float> c_pos; // collision point in world space
				vec3<float> normal;
				float penetration;
			};
			
			physics_2d(vec3<float> gravity = vec3<float>(0, 0, 0)) : m_gravity(gravity) {}
			virtual ~physics_2d() {}

			virtual void onFrameProcess(rynx::scheduler::context& context, float dt) override {
				rynx::scheduler::barrier position_updates_barrier("position updates");
				rynx::scheduler::barrier collisions_find_barrier("find collisions prereq");

				{
					rynx::scheduler::scoped_barrier_after scope(context, position_updates_barrier);
					
					context.add_task("Remove frame prev frame collisions", [](rynx::ecs::view<rynx::components::frame_collisions> ecs) {
						ecs.for_each([&](rynx::components::frame_collisions& collisions) {
							collisions.collisions.clear();
						});
					});

					context.add_task("Apply acceleration and reset", [dt](rynx::ecs::view<components::motion, const components::radius> ecs, rynx::scheduler::task& task_context) {
						{
							rynx_profile("Motion", "apply acceleration");
							ecs.query().notIn<components::collision_category>().execute_parallel(task_context, [dt](components::motion& m) {
								m.velocity += m.acceleration * dt;
								m.acceleration.set(0, 0, 0);
								m.angularVelocity += m.angularAcceleration * dt;
								m.angularAcceleration = 0;
							});
							
							ecs.query().in<components::collision_category>().execute_parallel(task_context, [dt](components::motion& m, const components::radius r) {
								m.velocity += m.acceleration * dt;
								m.acceleration.set(0, 0, 0);
								m.angularVelocity += m.angularAcceleration * dt;
								m.angularAcceleration = 0;

								if (m.velocity.lengthSquared() > sqr(r.r * 0.25f / dt)) {
									m.velocity.normalizeApprox();
									m.velocity *= r.r * 0.24f / dt;
								}

								if (sqr(m.angularVelocity) > sqr(0.10f / dt)) {
									m.angularVelocity = m.angularVelocity > 0 ? +1.0f : -1.0f;
									m.angularVelocity *= 0.09f / dt;
								}
							});
						}

						{
							rynx_profile("Motion", "apply velocity");
							ecs.for_each_parallel(task_context, [](components::position& p, const components::motion& m) {
								p.value += m.velocity;
								p.angle += m.angularVelocity;
							});
						}
					});
				}

				context.add_task("Apply dampening", [](rynx::scheduler::task& task, rynx::ecs::view<components::motion, const components::dampening> ecs) {
					ecs.for_each_parallel(task, [](components::motion& m, components::dampening d) {
						m.velocity *= d.linearDampening;
						m.angularVelocity *= d.angularDampening;
					});
				})->depends_on(position_updates_barrier);

				struct added_to_sphere_tree {};

				auto positionDataToSphereTree_task = context.make_task("Update position info to collision detection.", [](
					rynx::ecs::view<const components::position,
					const components::radius,
					const components::collision_category,
					const components::projectile,
					const components::motion,
					const added_to_sphere_tree> ecs,
					collision_detection& detection,
					rynx::scheduler::task& task) {
						// This will never insert. Always update existing. Can run parallel.
						// TODO: separate the updateEntity function -> update/insert versions.
						ecs.query()
							.notIn<const rynx::components::projectile>()
							.in<const added_to_sphere_tree>()
							.execute_parallel(task,
								[&](
									rynx::ecs::id id,
									const components::position& pos,
									const components::radius& r,
									const components::collision_category& category)
								{
									detection.get(category.value)->updateEntity(pos.value, r.r, id.value);
								}
						);

						ecs.query()
							.in<const rynx::components::projectile, const added_to_sphere_tree>()
							.execute_parallel(task, [&](rynx::ecs::id id, const rynx::components::motion& m, const rynx::components::position& p, const components::collision_category& category) {
							detection.get(category.value)->updateEntity(p.value - m.velocity * 0.5f, m.velocity.lengthApprox() * 0.5f, id.value);
						});
					});
				
				positionDataToSphereTree_task.then("sphere tree: add new entities", [](
					rynx::ecs::edit_view<const components::position,
						const components::radius,
						const components::collision_category,
						const components::projectile,
						const components::motion,
						added_to_sphere_tree>
					ecs,
					collision_detection& detection) {
						// This will always insert to detection datastructures. Can not parallel.
						// TODO: separate the function from the normal case of update.
						std::vector<rynx::ecs::id> ids;
						ecs.query().notIn<const rynx::components::projectile, const added_to_sphere_tree>().execute([&](rynx::ecs::id id, const components::position& pos, const components::radius& r, const components::collision_category& category) {
							detection.get(category.value)->updateEntity(pos.value, r.r, id.value);
							ids.emplace_back(id);
						});

						ecs.query().in<const rynx::components::projectile>().notIn<const added_to_sphere_tree>().execute([&](rynx::ecs::id id, const rynx::components::motion& m, const rynx::components::position& p, const components::collision_category& category) {
							detection.get(category.value)->updateEntity(p.value - m.velocity * 0.5f, m.velocity.lengthApprox() * 0.5f, id.value);
							ids.emplace_back(id);
						});

						for (auto&& id : ids) {
							ecs[id].add(added_to_sphere_tree());
						}

				})->then("Update sphere tree", [](collision_detection& detection, rynx::scheduler::task& task_context) {
					detection.update_parallel(task_context);
				})->required_for(collisions_find_barrier);
				
				positionDataToSphereTree_task.depends_on(position_updates_barrier);
				context.add_task(std::move(positionDataToSphereTree_task));

				auto updateBoundaryWorld = context.add_task(
					"Update boundary local -> boundary world",
					[](rynx::ecs::view<const components::position, components::boundary> ecs, rynx::scheduler::task& task_context) {
						ecs.for_each_parallel(task_context, [](components::position pos, components::boundary& boundary) {
							float sin_v = math::sin_approx(pos.angle);
							float cos_v = math::cos_approx(pos.angle);
							for (size_t i = 0; i < boundary.segments_local.size(); ++i) {
								boundary.segments_world[i].p1 = math::rotatedXY(boundary.segments_local[i].p1, sin_v, cos_v) + pos.value;
								boundary.segments_world[i].p2 = math::rotatedXY(boundary.segments_local[i].p2, sin_v, cos_v) + pos.value;
								boundary.segments_world[i].normal = math::rotatedXY(boundary.segments_local[i].normal, sin_v, cos_v);
							}
						});
					}
				);

				std::shared_ptr<rynx::parallel_accumulator<collision_event>> collisions_accumulator = std::make_shared<rynx::parallel_accumulator<collision_event>>();
				auto findCollisionsTask = context.add_task("Find collisions", [collisions_accumulator](
					rynx::ecs::view<
						const components::position,
						const components::radius,
						const components::boundary,
						const components::projectile,
						const components::collision_category,
						const components::motion,
						const components::physical_body> ecs,
					collision_detection& detection,
					rynx::scheduler::task& this_task) mutable
				{
					auto accumulator_copy = collisions_accumulator;
					detection.for_each_collision_parallel(collisions_accumulator, [ecs](
						std::vector<collision_event>& accumulator,
						uint64_t entityA,
						uint64_t entityB,
						vec3<float> a_pos,
						float a_radius,
						vec3<float> b_pos,
						float b_radius,
						vec3<float> normal,
						float penetration) {
						check_all(accumulator, ecs, entityA, entityB, a_pos, a_radius, b_pos, b_radius, normal, penetration);
					}, this_task);
				});

				findCollisionsTask->depends_on(collisions_find_barrier);
				findCollisionsTask->depends_on(updateBoundaryWorld);
				
				auto update_ropes = [](rynx::ecs::view<const components::rope, const components::physical_body, const components::position, components::motion> ecs, rynx::scheduler::task& task) {
					auto broken_ropes = ecs.for_each_parallel_accumulate<rynx::ecs::id>(task, [ecs](std::vector<rynx::ecs::id>& broken_ropes, rynx::ecs::id id, components::rope& rope) mutable {
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

						auto length = (world_pos_a - world_pos_b).lengthApprox();
						float over_extension = (length - rope.length) / rope.length;

						over_extension -= (over_extension < 0) * over_extension;
						float force = rope.strength * (over_extension * over_extension + over_extension);
						
						// Remove rope if too much strain
						if (force > 75.0f * rope.strength)
							broken_ropes.emplace_back(id);
						force = force > 1000.0f ? 1000.0f : force;

						auto direction_a_to_b = world_pos_b - world_pos_a;
						direction_a_to_b.normalizeApprox();
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

										ecs.create(
											rynx::components::position(pos),
											rynx::components::radius(p_info.radius.begin),
											rynx::components::motion(v, 0),
											rynx::components::lifetime(0.33f + 0.33f * weight),
											rynx::components::color(p_info.color.begin),
											rynx::components::dampening{ 0.95f + 0.03f * (1.0f / weight), 1 },
											p_info
										);
									}
								}
							});
						});
					}
				};

				auto collision_resolution_first_stage = [dt = dt, collisions_accumulator](rynx::ecs::view<components::frame_collisions, components::motion> ecs, rynx::scheduler::task& task) {

					std::vector<std::shared_ptr<std::vector<collision_event>>> overlaps_vector;
					collisions_accumulator->for_each([&overlaps_vector](std::vector<collision_event>& overlaps) mutable {
						overlaps_vector.emplace_back(std::make_shared<std::vector<collision_event>>(std::move(overlaps)));
					});

					struct event_extra_info {
						vec3<float> relative_position_tangent_a;
						float relative_position_length_a;
						vec3<float> relative_position_tangent_b;
						float relative_position_length_b;
					};

					std::vector<std::shared_ptr<std::vector<event_extra_info>>> extra_infos;
					extra_infos.resize(overlaps_vector.size());
					for (size_t i = 0; i < overlaps_vector.size(); ++i) {
						extra_infos[i] = std::make_shared<std::vector<event_extra_info>>();
						extra_infos[i]->resize(overlaps_vector[i]->size());
					}

					{
						rynx_profile("collisions", "fetch motion components");
						std::vector<rynx::scheduler::barrier> barriers;
						for (size_t i = 0; i < overlaps_vector.size(); ++i) {
							auto overlaps = overlaps_vector[i];
							auto extras = extra_infos[i];
							
							barriers.emplace_back(task.parallel().for_each(0, overlaps->size(), [overlaps, extras, ecs](int64_t index) mutable {
								auto& collision = overlaps->operator[](index);
								collision.a_motion = ecs[collision.a_id].try_get<components::motion>();
								collision.b_motion = ecs[collision.b_id].try_get<components::motion>();

								auto& extra = extras->operator[](index);
								const vec3<float> rel_pos_a = collision.a_pos - collision.c_pos;
								const vec3<float> rel_pos_b = collision.b_pos - collision.c_pos;

								const float rel_pos_len_a = rel_pos_a.lengthApprox();
								const float rel_pos_len_b = rel_pos_b.lengthApprox();
								extra.relative_position_length_a = rel_pos_len_a;
								extra.relative_position_length_b = rel_pos_len_b;
								extra.relative_position_tangent_a = rel_pos_a.normal2d() / (rel_pos_len_a + std::numeric_limits<float>::epsilon());
								extra.relative_position_tangent_b = rel_pos_b.normal2d() / (rel_pos_len_b + std::numeric_limits<float>::epsilon());
							}, 32));
						}

						rynx::spin_waiter() | barriers;
					}

					// NOTE TODO: The task is not thread-safe. We are now resolving all collision events in parallel, which means
					//            that we are resolving multiple collisions for the same entity in parallel, which will yield incorrect results.
					//            however, in practice this issue doesn't appear to be great. Might be that the iterative nature of approaching
					//            some global correct solution mitigates the damage the error cases cause.
					rynx_profile("collisions", "resolve 10x");
					for (int i = 0; i < 10; ++i)
						for (size_t k = 0; k < overlaps_vector.size(); ++k) {
							auto& overlaps = overlaps_vector[k];
							auto& extras = extra_infos[k];
							task& task.parallel().for_each(0, overlaps->size(), [overlaps, extras, ecs, dt](int64_t index) mutable {
								// for(size_t index=0; index < overlaps->size(); ++index) {
								const auto& collision = overlaps->operator[](index);
								components::motion& motion_a = *collision.a_motion;
								components::motion& motion_b = *collision.b_motion;

								auto velocity_at_point = [](float rel_pos_len, vec3<float> rel_pos_tangent, const components::motion& m, float dt) {
									return m.velocity + m.acceleration * dt - rel_pos_len * (m.angularVelocity + m.angularAcceleration * dt) * rel_pos_tangent;
								};

								const vec3<float> rel_pos_a = collision.a_pos - collision.c_pos;
								const vec3<float> rel_pos_b = collision.b_pos - collision.c_pos;

								const auto& extra = extras->operator[](index);
								const vec3<float> rel_v_a = velocity_at_point(extra.relative_position_length_a, extra.relative_position_tangent_a, motion_a, dt);
								const vec3<float> rel_v_b = velocity_at_point(extra.relative_position_length_b, extra.relative_position_tangent_b, motion_b, dt);
								const vec3<float> total_rel_v = rel_v_a - rel_v_b;

								const float impact_power = -total_rel_v.dot(collision.normal);
								if (impact_power < 0)
									// continue;
									return;

								const auto proximity_force = collision.normal * collision.penetration * collision.penetration;
								rynx_assert(collision.normal.lengthSquared() < 1.1f, "normal should be unit length");

								const float inertia1 = sqr(collision.normal.cross2d(rel_pos_a)) * collision.a_body.inv_moment_of_inertia;
								const float inertia2 = sqr(collision.normal.cross2d(rel_pos_b)) * collision.b_body.inv_moment_of_inertia;
								const float collision_elasticity = (collision.a_body.collision_elasticity + collision.b_body.collision_elasticity) * 0.5f; // combine some fun way

								const float top = (1.0f + collision_elasticity) * impact_power;
								const float bot = collision.a_body.inv_mass + collision.b_body.inv_mass + inertia1 + inertia2;

								const float soft_j = top / bot;

								const vec3<float> normal = collision.normal;
								const auto soft_impact_force = normal * soft_j;

								const float mu = (collision.a_body.friction_multiplier + collision.b_body.friction_multiplier) * 0.5f;
								vec3<float> tangent = normal.normal2d();
								if (tangent.dot(total_rel_v) < 0)
									tangent *= -1;

								float friction_power = -tangent.dot(total_rel_v) * mu;

								// if we want to limit friction according to physics, this would do the trick.
								if constexpr (false) {
									while (friction_power * friction_power > soft_j * soft_j)
										friction_power *= 0.9f;
								}

								tangent *= friction_power;

								motion_a.acceleration += (proximity_force + soft_impact_force + tangent) * collision.a_body.inv_mass / dt;
								motion_b.acceleration -= (proximity_force + soft_impact_force + tangent) * collision.b_body.inv_mass / dt;

								{
									float rotation_force_friction = tangent.cross2d(rel_pos_a);
									float rotation_force_linear = soft_impact_force.cross2d(rel_pos_a);
									motion_a.angularAcceleration += (rotation_force_linear + rotation_force_friction) * collision.a_body.inv_moment_of_inertia / dt;
								}

								{
									float rotation_force_friction = tangent.cross2d(rel_pos_b);
									float rotation_force_linear = soft_impact_force.cross2d(rel_pos_b);
									motion_b.angularAcceleration -= (rotation_force_linear + rotation_force_friction) * collision.b_body.inv_moment_of_inertia / dt;
								}
							});
						}
				};

				context.add_task("Gravity", [this](rynx::ecs::view<components::motion, const components::ignore_gravity> ecs, rynx::scheduler::task& task) {
					if (m_gravity.lengthSquared() > 0) {
						ecs.query().notIn<components::ignore_gravity>().execute_parallel(task, [this](components::motion& m) {
							m.acceleration += m_gravity;
						});
					}
				})->depends_on(*findCollisionsTask).then(update_ropes)->then(collision_resolution_first_stage);
			}

		private:
			static void check_all(
				std::vector<collision_event>& collisions_accumulator,
				rynx::ecs::view<
					const components::position,
					const components::radius,
					const components::boundary,
					const components::projectile,
					const components::collision_category,
					const components::motion,
					const components::physical_body> ecs,
				uint64_t entityA,
				uint64_t entityB,
				vec3<float> a_pos,
				float a_radius,
				vec3<float> b_pos,
				float b_radius,
				vec3<float> normal,
				float penetration);
		};
	}
}
