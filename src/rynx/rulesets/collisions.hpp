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
		class collisions : public application::logic::iruleset {
		public:
			collisions() {}
			virtual ~collisions() {}

			virtual void onFrameProcess(rynx::scheduler::context& context) override {

				rynx::scheduler::barrier position_updates_barrier("position updates");
				rynx::scheduler::barrier collisions_find_barrier("find collisions prereq");

				{
					rynx::scheduler::scoped_barrier_after(context, position_updates_barrier);
					
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

				context.add_task("Apply dampening", [](rynx::ecs::view<components::motion, const components::dampening> ecs) {
					ecs.for_each([](components::motion& m, components::dampening d) {
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
						ecs.query().notIn<const rynx::components::projectile>().in<const added_to_sphere_tree>().execute_parallel([&](rynx::ecs::id id, const components::position& pos, const components::radius& r, const components::collision_category& category) {
							detection.get(category.value)->updateEntity(pos.value, r.r, id.value);
						}, task.parallel());

						ecs.query().in<const rynx::components::projectile, const added_to_sphere_tree>().execute_parallel([&](rynx::ecs::id id, const rynx::components::motion& m, const rynx::components::position& p, const components::collision_category& category) {
							detection.get(category.value)->updateEntity(p.value - m.velocity * 0.5f, m.velocity.lengthApprox() * 0.5f, id.value);
						}, task.parallel());
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
				})->then("Update sphere tree", [](collision_detection& detection) {
					detection.update();
				})->required_for(collisions_find_barrier);;
				
				positionDataToSphereTree_task.depends_on(position_updates_barrier);
				context.add_task(std::move(positionDataToSphereTree_task));

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

				
				auto collision_resolution_first_stage = [](rynx::ecs::view<const components::frame_collisions, components::motion> ecs, rynx::scheduler::task& task) {
					ecs.for_each_parallel([ecs](components::motion& m, const components::frame_collisions& collisionsComponent) {
						std::vector<int> collisionCounts(collisionsComponent.collisions.size(), 1);
						
						for (size_t i = 0; i < collisionsComponent.collisions.size(); ++i) {
							for (size_t k = i+1; k < collisionsComponent.collisions.size(); ++k) {
								int count = collisionsComponent.collisions[i].idOfOther == collisionsComponent.collisions[k].idOfOther;
								collisionCounts[i] += count;
								collisionCounts[k] += count;
							}
						}

						for (int i = 0; i < 5; ++i) {
							for (size_t k = 0; k < collisionsComponent.collisions.size(); ++k) {
								const auto& collision = collisionsComponent.collisions[k];
								float impact_power = -(collision.collisionPointRelativeVelocity + m.acceleration).dot(collision.collisionNormal);
								float local_mul = !collision.other_has_collision_response * 1.3f + collision.other_has_collision_response * 0.35f;
								local_mul /= collisionCounts[k];
								auto proximity_force = collision.collisionNormal * collision.penetration * local_mul * (impact_power > 0);
								rynx_assert(collision.collisionNormal.lengthSquared() < 1.1f, "normal should be unit length");
								m.acceleration += proximity_force * (1.0f - collision.other_has_collision_response * 0.5f);

								float collision_velocity_fix = impact_power * local_mul;
								auto collision_resolution_force = collision.collisionNormal * (0.40f * collision_velocity_fix) * (collision_velocity_fix > 0);
								m.acceleration += collision_resolution_force;
							}
						}
					}, task.parallel());
				};

				auto apply_accelerations = [](rynx::ecs::view<components::motion> ecs) {
					rynx_profile("Motion", "apply acceleration");
					ecs.for_each([](components::motion& m) {
						m.velocity += m.acceleration;
						m.acceleration.set(0, 0, 0);
						m.angularVelocity += m.angularAcceleration;
						m.angularAcceleration = 0;
					});
				};

				context.add_task("Gravity", [](rynx::ecs::view<components::motion> ecs, rynx::scheduler::task& task) {
					ecs.for_each_parallel([](components::motion& m) {
						m.acceleration.y -= 0.03f;
					}, task.parallel());
					})->depends_on(*findCollisionsTask)
						.then(collision_resolution_first_stage);
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
