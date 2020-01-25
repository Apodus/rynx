
#include <rynx/rulesets/collisions.hpp>

#include <rynx/application/components.hpp>
#include <rynx/tech/collision_detection.hpp>

#include <rynx/scheduler/task_scheduler.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <rynx/tech/parallel_accumulator.hpp>
#include <rynx/tech/unordered_set.hpp>

namespace {
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

	using ecs_view = rynx::ecs::view<
		const rynx::components::position,
		const rynx::components::radius,
		const rynx::components::boundary,
		const rynx::components::projectile,
		const rynx::components::collision_category,
		const rynx::components::motion,
		const rynx::components::physical_body>;

	[[nodiscard]] collision_event store_collision(
		rynx::ecs::entity<ecs_view>& a,
		rynx::ecs::entity<ecs_view>& b,
		vec3<float> normal,
		vec3<float> collisionPoint,
		vec3<float> pos_a,
		vec3<float> pos_b,
		float penetration
	) {
		rynx_assert(normal.lengthSquared() > 0.9f, "normal must be unit length");
		rynx_assert(normal.lengthSquared() < 1.1f, "normal must be unit length");

		collision_event event;
		event.a_id = a.id();
		event.b_id = b.id();
		event.normal = normal;
		event.a_pos = pos_a;
		event.b_pos = pos_b;
		event.c_pos = collisionPoint;
		event.penetration = penetration;
		
		auto* a_body = a.try_get<const rynx::components::physical_body>();
		auto* b_body = b.try_get<const rynx::components::physical_body>();
		if(a_body)
			event.a_body = *a_body;
		if (b_body)
			event.b_body = *b_body;
		return event;
	}

	void check_polygon_ball(
		std::vector<collision_event>& collisions_accumulator, 
		rynx::ecs::entity<ecs_view> polygon,
		rynx::ecs::entity<ecs_view> ball,
		const vec3<float> polygon_position,
		const vec3<float> ball_position,
		const float ball_radius
	) {
		const auto& boundaryA = polygon.get<const rynx::components::boundary>();
		for (auto&& segment : boundaryA.segments_world) {
			const auto pointToLineSegment = math::pointDistanceLineSegmentSquared(segment.p1, segment.p2, ball_position);
			const float distSqr = pointToLineSegment.first;
			if (distSqr < ball_radius * ball_radius) {
				auto normal = (pointToLineSegment.second - ball_position).normalize();
				normal *= 2.0f * (normal.dot(polygon_position - pointToLineSegment.second) > 0) - 1.0f;

				collisions_accumulator.emplace_back(store_collision(
					polygon,
					ball,
					normal,
					pointToLineSegment.second,
					polygon_position,
					ball_position,
					ball_radius - math::sqrt_approx(distSqr)
				));
			}
		}
	}

	void check_polygon_polygon(
		std::vector<collision_event>& collisions_accumulator,
		rynx::ecs::entity<ecs_view> poly1,
		rynx::ecs::entity<ecs_view> poly2
	) {
		const auto& posA = poly1.get<const rynx::components::position>();
		const auto& posB = poly2.get<const rynx::components::position>();
		const auto& boundaryA = poly1.get<const rynx::components::boundary>();
		const auto& boundaryB = poly2.get<const rynx::components::boundary>();

		const float radiusB = poly2.get<const rynx::components::radius>().r;

		for (auto&& segmentA : boundaryA.segments_world) {
			const auto p1 = segmentA.p1;
			const auto p2 = segmentA.p2;
			const float dist = math::pointDistanceLineSegment(p1, p2, posB.value).first;
			if (dist < radiusB) {
				for (auto&& segmentB : boundaryB.segments_world) {
					const auto q1 = segmentB.p1;
					const auto q2 = segmentB.p2;

					const auto collisionPoint = math::lineSegmentIntersectionPoint(p1, p2, q1, q2);
					if (collisionPoint) {

						// how to choose normal?
						const float b1 = math::pointDistanceLineSegment(p1, p2, q1).first;
						const float b2 = math::pointDistanceLineSegment(p1, p2, q2).first;

						const float a1 = math::pointDistanceLineSegment(q1, q2, p1).first;
						const float a2 = math::pointDistanceLineSegment(q1, q2, p2).first;

						vec3<float> normal;
						if (((a1 < b1) & (a1 < b2)) | ((a2 < b1) & (a2 < b2))) {
							normal = segmentB.getNormalXY();
						}
						else {
							normal = segmentA.getNormalXY();
							normal *= -1.f;
						}
						
						float penetration1 = a1 < a2 ? a1 : a2;
						float penetration2 = b1 < b2 ? b1 : b2;
						float penetration = penetration1 < penetration2 ? penetration1 : penetration2;

						collisions_accumulator.emplace_back(store_collision(
							poly1,
							poly2,
							normal,
							collisionPoint.point(),
							posA.value,
							posB.value,
							penetration
						));
					}
				}
			}
		}
	}

	void check_projectile_ball(
		std::vector<collision_event>& collisions_accumulator,
		rynx::ecs::entity<ecs_view>& bulletEntity,
		rynx::ecs::entity<ecs_view>& dynamicEntity
	) {
		const auto& bulletPos = bulletEntity.get<const rynx::components::position>();
		const auto& bulletMotion = bulletEntity.get<const rynx::components::motion>();

		auto ballPos = dynamicEntity.get<const rynx::components::position>().value;
		auto pointDistanceResult = math::pointDistanceLineSegment(bulletPos.value, bulletPos.value - bulletMotion.velocity, ballPos);

		if (pointDistanceResult.first < dynamicEntity.get<const rynx::components::radius>().r + bulletEntity.get<const rynx::components::radius>().r) {
			collisions_accumulator.emplace_back(store_collision(
				bulletEntity,
				dynamicEntity,
				((bulletPos.value - bulletMotion.velocity) - ballPos).normalize(),
				pointDistanceResult.second,
				bulletPos.value,
				ballPos,
				0 // NOTE: penetration values don't really mean anything in projectile case.
			));
		}
	}

	// TODO: optimize if necessary.
	void check_projectile_polygon(
		std::vector<collision_event>& collisions_accumulator,
		rynx::ecs::entity<ecs_view>& bulletEntity,
		rynx::ecs::entity<ecs_view>& dynamicEntity
	) {
		const auto& bulletPos = bulletEntity.get<const rynx::components::position>();
		const auto& bulletMotion = bulletEntity.get<const rynx::components::motion>();

		const auto& polygonPositionComponent = dynamicEntity.get<const rynx::components::position>();
		auto polyPos = polygonPositionComponent.value;
		auto pointDistanceResult = math::pointDistanceLineSegment(bulletPos.value, bulletPos.value - bulletMotion.velocity, polyPos);

		if (pointDistanceResult.first < dynamicEntity.get<const rynx::components::radius>().r + bulletEntity.get<const rynx::components::radius>().r) {
			const auto& boundary = dynamicEntity.get<const rynx::components::boundary>();
			for (auto&& segment : boundary.segments_local) {
				auto intersectionTest = math::lineSegmentIntersectionPoint(
					bulletPos.value,
					bulletPos.value - bulletMotion.velocity,
					math::rotatedXY(segment.p1, polygonPositionComponent.angle) + polygonPositionComponent.value,
					math::rotatedXY(segment.p2, polygonPositionComponent.angle) + polygonPositionComponent.value
				);

				if (intersectionTest) {
					collisions_accumulator.emplace_back(store_collision(
						bulletEntity,
						dynamicEntity,
						math::rotatedXY(segment.getNormalXY(), polygonPositionComponent.angle),
						intersectionTest.point(),
						bulletPos.value,
						polyPos,
						0 // NOTE: penetration values don't really mean anything in projectile case.
					));
				}
			}
		}
	}


	void check_all(
		std::vector<collision_event>& collisions_accumulator,
		ecs_view ecs,
		uint64_t entityA,
		uint64_t entityB,
		vec3<float> a_pos,
		float a_radius,
		vec3<float> b_pos,
		float b_radius,
		vec3<float> normal,
		float penetration)
	{
		auto entA = ecs[entityA];
		auto entB = ecs[entityB];

		bool hasBoundaryA = entA.has<rynx::components::boundary>();
		bool hasBoundaryB = entB.has<rynx::components::boundary>();
		bool hasProjectileA = entA.has<rynx::components::projectile>();
		bool hasProjectileB = entB.has<rynx::components::projectile>();

		bool anyProjectile = hasProjectileA | hasProjectileB;
		bool anyBoundary = hasBoundaryA | hasBoundaryB;

		if (anyProjectile | anyBoundary) {
			if (anyProjectile) {
				if (anyBoundary) {
					// projectiles do not have boundary shapes. this must be projectile vs polygon check.
					if (hasProjectileA) {
						check_projectile_polygon(collisions_accumulator, entA, entB);
					}
					else {
						check_projectile_polygon(collisions_accumulator, entB, entA);
					}
				}
				else if (hasProjectileA & hasProjectileB) {
					// projectile vs projectile check.
					// check_projectile_projectile(entA, entB);
				}
				else {
					// projectile vs ball check.
					if (hasProjectileA) {
						check_projectile_ball(collisions_accumulator, entA, entB);
					}
					else {
						check_projectile_ball(collisions_accumulator, entB, entA);
					}
				}
			}
			else {
				if (hasBoundaryA & hasBoundaryB) {
					check_polygon_polygon(collisions_accumulator, entA, entB);
				}
				else if (hasBoundaryA) {
					check_polygon_ball(collisions_accumulator, entA, entB, a_pos, b_pos, b_radius);
				}
				else {
					check_polygon_ball(collisions_accumulator, entB, entA, b_pos, a_pos, a_radius);
				}
			}
		}
		else {
			// is ball vs ball collision. no further check needed, sphere tree said the balls collided.
			collisions_accumulator.emplace_back(store_collision(
				entA,
				entB,
				normal,
				(a_pos + b_pos) * 0.5f + normal * (a_radius - b_radius),
				a_pos,
				b_pos,
				penetration
			));
		}
	}

}


void rynx::ruleset::physics_2d::onFrameProcess(rynx::scheduler::context& context, float dt) {
	rynx::scheduler::barrier position_updates_barrier("position updates");
	rynx::scheduler::barrier collisions_find_barrier("find collisions prereq");

	{
		rynx::scheduler::scoped_barrier_after scope(context, position_updates_barrier);

		context.add_task("Remove frame prev frame collisions", [](rynx::ecs::view<rynx::components::frame_collisions> ecs) {
			ecs.query().for_each([&](rynx::components::frame_collisions& collisions) {
				collisions.collisions.clear();
				});
			});

		context.add_task("Apply acceleration and reset", [dt](
			rynx::ecs::view<
			components::motion,
			components::position,
			const components::radius,
			const components::collision_category> ecs, rynx::scheduler::task& task_context)
			{
				{
					rynx_profile("Motion", "apply acceleration");
					ecs.query().notIn<components::collision_category>().for_each_parallel(task_context, [dt](components::motion& m) {
						m.velocity += m.acceleration * dt;
						m.acceleration.set(0, 0, 0);
						m.angularVelocity += m.angularAcceleration * dt;
						m.angularAcceleration = 0;
					});

					ecs.query().in<components::collision_category>().for_each_parallel(task_context, [dt](components::motion& m) {
						m.velocity += m.acceleration * dt;
						m.acceleration.set(0, 0, 0);
						m.angularVelocity += m.angularAcceleration * dt;
						m.angularAcceleration = 0;

						// turns out that limiting object velocities is a really bad idea
						// if you allow other objects to move faster than the limit of another object.
						// case in point: quickly rotating bar has a very high linear velocity at the tip
						//                if this tip hits a small object which has a low maximum velocity
						//                then the object will simply sink through the rotating bar. this is not good.
						if constexpr (false) {
							// cap velocity to be proportional to radius of self.
							// imagine the collision detection implications.
							if (m.velocity.lengthSquared() > sqr(r.r * 1.0f / dt)) {
								m.velocity.normalize();
								m.velocity *= r.r * 1.0f / dt;
							}

							if (sqr(m.angularVelocity) > sqr(0.10f / dt)) {
								m.angularVelocity = m.angularVelocity > 0 ? +1.0f : -1.0f;
								m.angularVelocity *= 0.10f / dt;
							}
						}
					});
				}

				{
					rynx_profile("Motion", "apply velocity");
					ecs.query().for_each_parallel(task_context, [dt](components::position& p, const components::motion& m) {
						p.value += m.velocity * dt;
						p.angle += m.angularVelocity * dt;
					});
				}
			}
		);
	}

	context.add_task("Apply dampening", [dt](rynx::scheduler::task& task, rynx::ecs::view<components::motion, const components::dampening> ecs) {
		ecs.query().for_each_parallel(task, [dt](components::motion& m, components::dampening d) {
			m.velocity *= std::powf(d.linearDampening, dt);
			m.angularVelocity *= std::powf(d.angularDampening, dt);
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
					.for_each_parallel(task,
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
					.for_each_parallel(task, [&](rynx::ecs::id id, const rynx::components::motion& m, const rynx::components::position& p, const components::collision_category& category) {
					detection.get(category.value)->updateEntity(p.value - m.velocity * 0.5f, m.velocity.length() * 0.5f, id.value);
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
				ecs.query().notIn<const rynx::components::projectile, const added_to_sphere_tree>().for_each([&](rynx::ecs::id id, const components::position& pos, const components::radius& r, const components::collision_category& category) {
					detection.get(category.value)->updateEntity(pos.value, r.r, id.value);
					ids.emplace_back(id);
					});

				ecs.query().in<const rynx::components::projectile>().notIn<const added_to_sphere_tree>().for_each([&](rynx::ecs::id id, const rynx::components::motion& m, const rynx::components::position& p, const components::collision_category& category) {
					detection.get(category.value)->updateEntity(p.value - m.velocity * 0.5f, m.velocity.length() * 0.5f, id.value);
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
						ecs.query().for_each_parallel(task_context, [](components::position pos, components::boundary& boundary) {
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

				auto update_ropes = [dt](rynx::ecs::view<components::rope, const components::physical_body, const components::position, components::motion> ecs, rynx::scheduler::task& task) {
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
				};

				auto collision_resolution_first_stage = [dt = dt, collisions_accumulator](
					rynx::ecs::view<components::motion> ecs,
					rynx::scheduler::task& task)
				{

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

							barriers.emplace_back(task.parallel().for_each(0, overlaps->size(), 32).for_each([overlaps, extras, ecs](int64_t index) mutable {
								auto& collision = overlaps->operator[](index);
								collision.a_motion = ecs[collision.a_id].try_get<components::motion>();
								collision.b_motion = ecs[collision.b_id].try_get<components::motion>();

								auto& extra = extras->operator[](index);
								const vec3<float> rel_pos_a = collision.a_pos - collision.c_pos;
								const vec3<float> rel_pos_b = collision.b_pos - collision.c_pos;

								const float rel_pos_len_a = rel_pos_a.length();
								const float rel_pos_len_b = rel_pos_b.length();
								extra.relative_position_length_a = rel_pos_len_a;
								extra.relative_position_length_b = rel_pos_len_b;
								extra.relative_position_tangent_a = rel_pos_a.normal2d() / (rel_pos_len_a + std::numeric_limits<float>::epsilon());
								extra.relative_position_tangent_b = rel_pos_b.normal2d() / (rel_pos_len_b + std::numeric_limits<float>::epsilon());
								}));
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
							task& task.parallel().for_each(0, overlaps->size(), 2048).deferred_work().for_each([overlaps, extras, ecs, dt](int64_t index) mutable {
								const auto& collision = overlaps->operator[](index);
								components::motion& motion_a = *collision.a_motion;
								components::motion& motion_b = *collision.b_motion;

								auto velocity_at_point = [](float rel_pos_len, vec3<float> rel_pos_tangent, const components::motion& m, float dt) {
									return m.velocity + m.acceleration * dt - rel_pos_len * (m.angularVelocity + m.angularAcceleration * dt) * rel_pos_tangent;
								};

								const auto& extra = extras->operator[](index);
								const vec3<float> rel_v_a = velocity_at_point(extra.relative_position_length_a, extra.relative_position_tangent_a, motion_a, dt);
								const vec3<float> rel_v_b = velocity_at_point(extra.relative_position_length_b, extra.relative_position_tangent_b, motion_b, dt);
								const vec3<float> total_rel_v = rel_v_a - rel_v_b;

								const float impact_power = -total_rel_v.dot(collision.normal);
								if (impact_power < 0)
									return;

								// This *60 is not the same as dt. Just a random constant.
								const auto proximity_force = collision.normal * collision.penetration * collision.penetration * 60.0f;
								rynx_assert(collision.normal.lengthSquared() < 1.1f, "normal should be unit length");

								const vec3<float> rel_pos_a = collision.a_pos - collision.c_pos;
								const vec3<float> rel_pos_b = collision.b_pos - collision.c_pos;

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
								tangent *= ((tangent.dot(total_rel_v) > 0) * 2.0f - 1.0f);

								float friction_power = -tangent.dot(total_rel_v) * mu;

								// if we want to limit friction according to physics, this would do the trick.
								if constexpr (false) {
									while (friction_power * friction_power > soft_j* soft_j)
										friction_power *= 0.9f;
								}

								tangent *= friction_power;

								float inv_dt = 1.0f / dt;
								motion_a.acceleration += (proximity_force + soft_impact_force + tangent) * collision.a_body.inv_mass * inv_dt;
								motion_b.acceleration -= (proximity_force + soft_impact_force + tangent) * collision.b_body.inv_mass * inv_dt;

								{
									float rotation_force_friction = tangent.cross2d(rel_pos_a);
									float rotation_force_linear = soft_impact_force.cross2d(rel_pos_a);
									motion_a.angularAcceleration += (rotation_force_linear + rotation_force_friction) * collision.a_body.inv_moment_of_inertia * inv_dt;
								}

								{
									float rotation_force_friction = tangent.cross2d(rel_pos_b);
									float rotation_force_linear = soft_impact_force.cross2d(rel_pos_b);
									motion_b.angularAcceleration -= (rotation_force_linear + rotation_force_friction) * collision.b_body.inv_moment_of_inertia * inv_dt;
								}
								});
						}
				};

				context.add_task("Gravity", [this](rynx::ecs::view<components::motion, const components::ignore_gravity> ecs, rynx::scheduler::task& task) {
					if (m_gravity.lengthSquared() > 0) {
						ecs.query().notIn<components::ignore_gravity>().for_each_parallel(task, [this](components::motion& m) {
							m.acceleration += m_gravity;
							});
					}
					})->depends_on(*findCollisionsTask).then("update ropes", update_ropes)->then("collision resolve", collision_resolution_first_stage);
}

