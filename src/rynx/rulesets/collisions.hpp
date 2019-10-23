#pragma once

#include <rynx/tech/object_storage.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/application/components.hpp>
#include <rynx/tech/collision_detection.hpp>

#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <rynx/tech/parallel_accumulator.hpp>

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
				vec3<float> a_pos;
				vec3<float> b_pos;
				vec3<float> c_pos; // collision point in world space
				vec3<float> normal;
				float penetration;
			};
			
			physics_2d(vec3<float> gravity = vec3<float>(0, 0, 0)) : m_gravity(gravity) {}
			virtual ~physics_2d() {}

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
				
				auto collision_resolution_first_stage = [collisions_accumulator](rynx::ecs::view<const components::frame_collisions, components::motion> ecs, rynx::scheduler::task& task) {

					std::vector<collision_event> overlaps = collisions_accumulator->combine_and_clear();
					for(int i=0; i<10; ++i)
					for (auto&& collision : overlaps) {
						auto entity_a = ecs[collision.a_id];
						auto entity_b = ecs[collision.b_id];

						components::motion& motion_a = entity_a.get<components::motion>();
						components::motion& motion_b = entity_b.get<components::motion>();

						auto velocity_at_point = [](vec3<float> relative_pos, const components::motion& m) {
							return m.velocity + m.acceleration - relative_pos.lengthApprox() * (m.angularVelocity + m.angularAcceleration) * relative_pos.normal2d().normalizeApprox();
						};

						vec3<float> rel_pos_a = collision.a_pos - collision.c_pos;
						vec3<float> rel_pos_b = collision.b_pos - collision.c_pos;

						vec3<float> rel_v_a = velocity_at_point(rel_pos_a, motion_a);
						vec3<float> rel_v_b = velocity_at_point(rel_pos_b, motion_b);
						vec3<float> total_rel_v = rel_v_a - rel_v_b;

						float impact_power = -total_rel_v.dot(collision.normal);
						if (impact_power < 0)
							continue;

						auto proximity_force = collision.normal * collision.penetration * collision.penetration;
						rynx_assert(collision.normal.lengthSquared() < 1.1f, "normal should be unit length");
						
						float inertia1 = sqr(collision.normal.cross2d(rel_pos_a)) * collision.a_body.inv_moment_of_inertia;
						float inertia2 = sqr(collision.normal.cross2d(rel_pos_b)) * collision.b_body.inv_moment_of_inertia;
						float collision_elasticity = (collision.a_body.collision_elasticity + collision.b_body.collision_elasticity) * 0.5f; // combine some fun way

						float top = (1.0f + collision_elasticity) * impact_power;
						float bot = collision.a_body.inv_mass + collision.b_body.inv_mass + inertia1 + inertia2;

						float bias = 0.99f + collision.penetration * collision.penetration;
						float soft_j = bias * top / bot;
						
						vec3<float> normal = collision.normal;
						auto soft_impact_force = normal * soft_j;
						
						float mu = (collision.a_body.friction_multiplier + collision.b_body.friction_multiplier) * 0.5f;
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

						motion_a.acceleration += (proximity_force + soft_impact_force + tangent) * collision.a_body.inv_mass; // *0.016666f;
						motion_b.acceleration -= (proximity_force + soft_impact_force + tangent) * collision.b_body.inv_mass; // *0.016666f;

						{
							float rotation_force_friction = tangent.cross2d(rel_pos_a);
							float rotation_force_linear = soft_impact_force.cross2d(rel_pos_a);
							motion_a.angularAcceleration += (rotation_force_linear + rotation_force_friction) * collision.a_body.inv_moment_of_inertia;
						}

						{
							float rotation_force_friction = tangent.cross2d(rel_pos_b);
							float rotation_force_linear = soft_impact_force.cross2d(rel_pos_b);
							motion_b.angularAcceleration -= (rotation_force_linear + rotation_force_friction) * collision.b_body.inv_moment_of_inertia;
						}
					}
				};

				context.add_task("Gravity", [this](rynx::ecs::view<components::motion, const components::frame_collisions> ecs, rynx::scheduler::task& task) {
					if (m_gravity.lengthSquared() > 0) {
						ecs.query().in<components::frame_collisions>().execute_parallel(task.parallel(), [this](components::motion& m) {
							m.acceleration += m_gravity;
						});
					}
				})->depends_on(*findCollisionsTask).then(collision_resolution_first_stage);
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
