#pragma once

#include <rynx/tech/object_storage.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/application/components.hpp>
#include <rynx/tech/collision_detection.hpp>

#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/barrier.hpp>

namespace rynx {
	namespace ruleset {
		class physics_2d_sidescrolling : public application::logic::iruleset {
		public:
			physics_2d_sidescrolling() {}
			virtual ~physics_2d_sidescrolling() {}

			virtual void onFrameProcess(rynx::scheduler::context& context) override {

				rynx::scheduler::barrier position_updates_barrier("position updates");
				rynx::scheduler::barrier collisions_find_barrier("find collisions prereq");

				{
					rynx::scheduler::scoped_barrier_after scope(context, position_updates_barrier);
					
					context.add_task("Remove frame prev frame collisions", [](rynx::ecs::view<rynx::components::frame_collisions> ecs) {
						ecs.for_each([&](rynx::components::frame_collisions& collisions) {
							collisions.collisions.clear();
						});
					});

					context.add_task("Apply acceleration and reset", [](rynx::ecs::view<components::motion> ecs) {
						{
							rynx_profile("Motion", "apply acceleration");
							ecs.for_each([](components::motion& m) {
								m.velocity += m.acceleration;
								m.acceleration.set(0, 0, 0);
								m.angularVelocity += m.angularAcceleration;
								m.angularAcceleration = 0;
							});
						}

						{
							rynx_profile("Motion", "apply velocity");
							ecs.for_each([](components::position& p, const components::motion& m) {
								p.value += m.velocity;
								p.angle += m.angularVelocity;
							});
						}
					});
				}

				context.add_task("Apply dampening", [](rynx::scheduler::task& task, rynx::ecs::view<components::motion, const components::dampening> ecs) {
					ecs.for_each_parallel(task.parallel(), [](components::motion& m, components::dampening d) {
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
							.execute_parallel(task.parallel(),
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
							.execute_parallel(task.parallel(), [&](rynx::ecs::id id, const rynx::components::motion& m, const rynx::components::position& p, const components::collision_category& category) {
							detection.get(category.value)->updateEntity(p.value - m.velocity * 0.5f, m.velocity.lengthApprox() * 0.5f, id.value);
						});
					});
				
				positionDataToSphereTree_task.then("sphere tree: add new entities", [](rynx::ecs::edit_view<const components::position,
					const components::radius,
					const components::collision_category,
					const components::projectile,
					const components::motion,
					added_to_sphere_tree> ecs,
					collision_detection& detection) {
						// This will always insert to detection datastructures. Can not parallel.
						// TODO: separate the function from the normal case of update.
						ecs.query().notIn<const rynx::components::projectile, const added_to_sphere_tree>().execute([&](rynx::ecs::id id, const components::position& pos, const components::radius& r, const components::collision_category& category) {
							detection.get(category.value)->updateEntity(pos.value, r.r, id.value);
							ecs[id].add(added_to_sphere_tree());
						});

						ecs.query().in<const rynx::components::projectile>().notIn<const added_to_sphere_tree>().execute([&](rynx::ecs::id id, const rynx::components::motion& m, const rynx::components::position& p, const components::collision_category& category) {
							detection.get(category.value)->updateEntity(p.value - m.velocity * 0.5f, m.velocity.lengthApprox() * 0.5f, id.value);
							ecs[id].add(added_to_sphere_tree());
						});
				})->then("Update sphere tree", [](collision_detection& detection, rynx::scheduler::task& task_context) {
					detection.update_parallel(task_context);
				})->required_for(collisions_find_barrier);
				
				positionDataToSphereTree_task.depends_on(position_updates_barrier);
				context.add_task(std::move(positionDataToSphereTree_task));

				auto updateBoundaryWorld = context.add_task(
					"Update boundary local -> boundary world",
					[](rynx::ecs::view<const components::position, components::boundary> ecs, rynx::scheduler::task& task_context) {
						ecs.for_each_parallel(task_context.parallel(), [](components::position pos, components::boundary& boundary) {
							float sin_v = math::sin(pos.angle);
							float cos_v = math::cos(pos.angle);
							for (size_t i = 0; i < boundary.segments_local.size(); ++i) {
								boundary.segments_world[i].p1 = math::rotatedXY(boundary.segments_local[i].p1, sin_v, cos_v) + pos.value;
								boundary.segments_world[i].p2 = math::rotatedXY(boundary.segments_local[i].p2, sin_v, cos_v) + pos.value;
								boundary.segments_world[i].normal = math::rotatedXY(boundary.segments_local[i].normal, sin_v, cos_v);
							}
						});
					}
				);

				auto findCollisionsTask = context.add_task("Find collisions", [](
					rynx::ecs::view<
						const components::position,
						const components::radius,
						const components::boundary,
						const components::projectile,
						const components::collision_category,
						const components::motion,
						components::frame_collisions> ecs,
					collision_detection& detection,
					rynx::scheduler::task& this_task) {
					detection.for_each_collision_parallel([ecs](uint64_t entityA, uint64_t entityB, vec3<float> normal, float penetration) {
						check_all(ecs, entityA, entityB, normal, penetration);
					}, this_task);
				});

				findCollisionsTask->depends_on(collisions_find_barrier);
				findCollisionsTask->depends_on(updateBoundaryWorld);

				/*
				auto update_relative_velocity = [](rynx::ecs::view<components::frame_collisions, const components::position, const components::motion> ecs, rynx::scheduler::task& task) {
					ecs.for_each_parallel(task.parallel(), [ecs](components::motion& m, const components::position pos, components::frame_collisions& collisionsComponent) {
						for (auto&& entry : collisionsComponent.collisions) {
							auto other = ecs[entry.idOfOther];
							const auto& om = other.get<const components::motion>();
							const auto& op = other.get<const components::position>();
							entry.collisionPointRelativeVelocity = m.velocity_at_point(entry.collisionPointRelative) - om.velocity_at_point(pos.value + entry.collisionPointRelative - op.value);
						}
					});
				};
				*/
				
				auto collision_resolution_first_stage = [](rynx::ecs::view<const components::frame_collisions, components::motion> ecs, rynx::scheduler::task& task) {
					ecs.for_each_parallel(task.parallel(), [ecs](components::motion& m, const components::frame_collisions& collisionsComponent) {
						std::vector<int> collisionCounts(collisionsComponent.collisions.size(), 1);
						
						for (size_t i = 0; i < collisionsComponent.collisions.size(); ++i) {
							for (size_t k = i+1; k < collisionsComponent.collisions.size(); ++k) {
								int count = collisionsComponent.collisions[i].idOfOther == collisionsComponent.collisions[k].idOfOther;
								collisionCounts[i] += count;
								collisionCounts[k] += count;
							}
						}

						for (int i = 0; i < 3; ++i) {
							for (size_t k = 0; k < collisionsComponent.collisions.size(); ++k) {
								const auto& collision = collisionsComponent.collisions[k];
								// TODO: Rotation response
								// float linear_velocity_from_rotation = m.angularVelocity * (collision.collisionPoint)
								
								vec3<float> angular_response_effect = m.angularAcceleration * collision.collisionPointRelative.normal2d();
								auto relative_velocity = collision.collisionPointRelativeVelocity + m.acceleration + angular_response_effect;
								float impact_power = -relative_velocity.dot(collision.collisionNormal);
								
								if (impact_power < 0)
									continue;

								float local_mul = !collision.other_has_collision_response * 1.3f + collision.other_has_collision_response * 0.35f;
								local_mul /= collisionCounts[k];
								
								auto proximity_force = collision.collisionNormal * collision.penetration * local_mul * (impact_power > 0);
								rynx_assert(collision.collisionNormal.lengthSquared() < 1.1f, "normal should be unit length");
								m.acceleration += proximity_force * (1.0f - collision.other_has_collision_response * 0.5f);

								float inertia1 = sqr(collision.collisionNormal.cross2d(collision.collisionPointRelative)) / 8.0f; // divided by moment of inertia of the body.
								float inertia2 = sqr(collision.collisionNormal.cross2d(collision.collisionPointRelative)) / 20.0f; // divided by moment of inertia of the body.
								float collision_elasticity = 0.1f;
								
								float top = (1.0f + collision_elasticity) * impact_power;
								float bot = 1.0f / 5.0f + 1.0f / 50000000.0f + inertia1 + inertia2; // assume other has very high mass
								
								float bias = 0.05f * collision.penetration;
								float soft_j = local_mul * bias * top / bot;
								float hard_j = local_mul * (top / bot + bias * 0.1f);

								/*
								soft_j = (soft_j < bias) ? bias : soft_j;
								hard_j = (hard_j < bias) ? bias : hard_j;
								*/

								vec3<float> normal = collision.collisionNormal;
								normal.normalizeApprox();

								auto soft_impact_force = normal * soft_j * (impact_power > 0);
								auto hard_impact_force = normal * hard_j * (impact_power > 0);

								// TODO: Have friction factors stored in some components
								constexpr float mu = 1.00f; // just pick some constant until then.

								vec3<float> tangent = normal.normal2d();
								if (tangent.dot(relative_velocity) < 0)
									tangent *= -1;
								tangent.normalizeApprox();

								float friction_power = -tangent.dot(relative_velocity) * mu;
								// while (friction_power * friction_power > soft_j * soft_j)
								//	friction_power *= 0.9f;
								
								tangent *= friction_power;
								
								/*
								float collision_velocity_fix = impact_power * local_mul;
								auto collision_resolution_force = collision.collisionNormal * 0.40f * collision_velocity_fix + tangent;
								m.acceleration += collision_resolution_force;
								*/

								// A->velocity -= (1 / A->mass) * impulse
								m.acceleration += (soft_impact_force + tangent) * 1.0f / 5.0f; // divide by mass
								// m.acceleration += (hard_impact_force + tangent) * 1.0f / 5.0f;
								
								float rotation_force_friction = tangent.cross2d(collision.collisionPointRelative);
								float rotation_force_linear = soft_impact_force.cross2d(collision.collisionPointRelative);
								m.angularAcceleration -= (rotation_force_linear + rotation_force_friction) * 0.2f; // divide by moment of inertia.
							}
						}
					});
				};

				context.add_task("Gravity", [](rynx::ecs::view<components::motion, const components::frame_collisions> ecs, rynx::scheduler::task& task) {
					ecs.query().in<components::frame_collisions>().execute_parallel(task.parallel(), [](components::motion& m) {
						m.acceleration.y -= 0.03f;
					});
				})->depends_on(*findCollisionsTask).then(collision_resolution_first_stage);
			}

		private:
			static void check_all(
				rynx::ecs::view<
					const components::position,
					const components::radius,
					const components::boundary,
					const components::projectile,
					const components::collision_category,
					const components::motion,
					components::frame_collisions> ecs,
				uint64_t entityA, uint64_t entityB, vec3<float> normal, float penetration);
		};
	}
}
