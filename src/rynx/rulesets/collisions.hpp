#pragma once

#include <rynx/tech/object_storage.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/application/components.hpp>
#include <rynx/tech/collision_detection.hpp>

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
						ecs.for_each([](components::motion& m) {
							m.velocity += m.acceleration;
							m.acceleration.set(0, 0, 0);
							m.angularVelocity += m.angularAcceleration;
							m.angularAcceleration = 0;
						});
					});

					context.add_task("Apply motion to position", [](rynx::ecs::view<components::position, components::motion> ecs) {
						ecs.for_each([](components::position& p, components::motion& m) {
							p.value += m.velocity;
							p.angle += m.angularVelocity;
						});
					});
				}
				
				context.add_task("Apply dampening", [](rynx::ecs::view<components::motion, components::dampening> ecs) {
					ecs.for_each([](components::motion& m, components::dampening& d) {
						m.velocity *= d.linearDampening;
						m.angularVelocity *= d.angularDampening;
					});
				})->after(position_updates_barrier);

				/*
				context.add_task("Clear collisions from previous frame", [](rynx::ecs::view<components::frame_collisions> ecs, collision_detection& detection) {
					detection.collidedThisFrame().forEachOne([&ecs](uint64_t id) {
						ecs[uint32_t(id)].get<components::frame_collisions>().collisions.clear();
					});
					detection.collidedThisFrame().clear();
				})->before(collisions_find_barrier);
				*/

				auto positionDataToSphereTree_task = context.make_task("Update position info to collision detection.", [](
					rynx::ecs::view<const components::position,
					const components::radius,
					const components::collision_category,
					const components::projectile,
					const components::motion> ecs,
					collision_detection& detection) {
						ecs.query().notIn<const rynx::components::projectile>().execute([&](rynx::ecs::id id, const components::position& pos, const components::radius& r, const components::collision_category& category) {
							detection.get(category.value)->updateEntity(pos.value, r.r, id.value);
						});

						ecs.query().in<const rynx::components::projectile>().execute([&](rynx::ecs::id id, const rynx::components::motion& m, const rynx::components::position& p, const components::collision_category& category) {
							detection.get(category.value)->updateEntity(p.value - m.velocity * 0.5f, m.velocity.lengthApprox() * 0.5f, id.value);
						});
				});
				
				positionDataToSphereTree_task.after(position_updates_barrier);
				positionDataToSphereTree_task.then("Update sphere tree", [](collision_detection& detection) {
					detection.update();
				})->before(collisions_find_barrier);
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

				findCollisionsTask->after(collisions_find_barrier);

				context.add_task("Collision resolution", [](rynx::ecs::view<const components::frame_collisions, components::motion> ecs) {
					ecs.for_each([](components::motion& m, const components::frame_collisions& collisionsComponent) {
						for (const auto& collision : collisionsComponent.collisions) {
							// TODO: Some smarter collision resolution would be nice.
							// TODO: Collision resolution should be more harsh when colliding against static objects (walls).
							m.acceleration += collision.collisionNormal * collision.penetration * 0.12f;
						}
					});
				})->dependsOn(*findCollisionsTask);
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