
#include <rynx/rulesets/collisions.hpp>

#include <rynx/application/components.hpp>
#include <rynx/tech/collision_detection.hpp>

#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <rynx/tech/sphere_tree.hpp>
#include <rynx/tech/parallel_accumulator.hpp>

namespace {
	struct collision_event {
		uint64_t a_id;
		uint64_t b_id;
		rynx::components::physical_body a_body;
		rynx::components::physical_body b_body;
		rynx::components::motion* a_motion;
		rynx::components::motion* b_motion;
		rynx::vec3<float> a_pos;
		rynx::vec3<float> b_pos;
		rynx::vec3<float> c_pos; // collision point in world space
		rynx::vec3<float> normal;
		float penetration;
	};

	using ecs_view = rynx::ecs::view<
		const rynx::components::position,
		const rynx::components::radius,
		const rynx::components::boundary,
		const rynx::components::projectile,
		const rynx::components::motion,
		const rynx::components::physical_body>;

	void create_collision_event(
		std::vector<collision_event>& storage,
		rynx::ecs::entity<ecs_view>& a,
		rynx::ecs::entity<ecs_view>& b,
		rynx::vec3<float> normal,
		rynx::vec3<float> collisionPoint,
		rynx::vec3<float> pos_a,
		rynx::vec3<float> pos_b,
		float penetration
	) {
		// rynx_assert(normal.length_squared() > 0.9f, "normal must be unit length");
		// rynx_assert(normal.length_squared() < 1.1f, "normal must be unit length");

		collision_event event;

		event.a_body = a.get<const rynx::components::physical_body>();
		event.b_body = b.get<const rynx::components::physical_body>();

		if (
			(event.a_body.collision_id == event.b_body.collision_id) &
			((event.a_body.collision_id | event.b_body.collision_id) != 0)
		) {
			return;
		}

		event.a_id = a.id();
		event.b_id = b.id();
		event.normal = normal;
		event.a_pos = pos_a;
		event.b_pos = pos_b;
		event.c_pos = collisionPoint;
		event.penetration = penetration;

		storage.emplace_back(event);
	}

	void check_polygon_ball(
		std::vector<collision_event>& collisions_accumulator, 
		rynx::ecs::entity<ecs_view> polygon,
		rynx::ecs::entity<ecs_view> ball,
		const rynx::vec3<float> polygon_position,
		const rynx::vec3<float> ball_position,
		const float ball_radius
	) {
		const auto& boundaryA = polygon.get<const rynx::components::boundary>();
		for (auto&& segment : boundaryA.segments_world) {
			const auto pointToLineSegment = rynx::math::pointDistanceLineSegmentSquared(segment.p1, segment.p2, ball_position);
			const float distSqr = pointToLineSegment.first;
			if (distSqr < ball_radius * ball_radius) {
				auto normal = (pointToLineSegment.second - ball_position).normalize();
				normal *= 2.0f * (normal.dot(polygon_position - pointToLineSegment.second) > 0) - 1.0f;

				create_collision_event(
					collisions_accumulator,
					polygon,
					ball,
					normal,
					pointToLineSegment.second,
					polygon_position,
					ball_position,
					ball_radius - rynx::math::sqrt_approx(distSqr)
				);
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
			const float dist = rynx::math::pointDistanceLineSegment(p1, p2, posB.value).first;
			if (dist < radiusB) {
				for (auto&& segmentB : boundaryB.segments_world) {
					const auto q1 = segmentB.p1;
					const auto q2 = segmentB.p2;

					const auto collisionPoint = rynx::math::lineSegmentIntersectionPoint(p1, p2, q1, q2);
					if (collisionPoint) {

						// how to choose normal?
						const float b1 = rynx::math::pointDistanceLineSegment(p1, p2, q1).first;
						const float b2 = rynx::math::pointDistanceLineSegment(p1, p2, q2).first;

						const float a1 = rynx::math::pointDistanceLineSegment(q1, q2, p1).first;
						const float a2 = rynx::math::pointDistanceLineSegment(q1, q2, p2).first;

						rynx::vec3<float> normal;
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

						
						create_collision_event(
							collisions_accumulator,
							poly1,
							poly2,
							normal,
							collisionPoint.point(),
							posA.value,
							posB.value,
							penetration
						);
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
		auto pointDistanceResult = rynx::math::pointDistanceLineSegment(bulletPos.value, bulletPos.value - bulletMotion.velocity, ballPos);

		if (pointDistanceResult.first < dynamicEntity.get<const rynx::components::radius>().r + bulletEntity.get<const rynx::components::radius>().r) {
			create_collision_event(
				collisions_accumulator,
				bulletEntity,
				dynamicEntity,
				((bulletPos.value - bulletMotion.velocity) - ballPos).normalize(),
				pointDistanceResult.second,
				bulletPos.value,
				ballPos,
				0 // NOTE: penetration values don't really mean anything in projectile case.
			);
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
		auto pointDistanceResult = rynx::math::pointDistanceLineSegment(bulletPos.value, bulletPos.value - bulletMotion.velocity, polyPos);

		if (pointDistanceResult.first < dynamicEntity.get<const rynx::components::radius>().r + bulletEntity.get<const rynx::components::radius>().r) {
			const auto& boundary = dynamicEntity.get<const rynx::components::boundary>();
			for (auto&& segment : boundary.segments_local) {
				auto intersectionTest = rynx::math::lineSegmentIntersectionPoint(
					bulletPos.value,
					bulletPos.value - bulletMotion.velocity,
					rynx::math::rotatedXY(segment.p1, polygonPositionComponent.angle) + polygonPositionComponent.value,
					rynx::math::rotatedXY(segment.p2, polygonPositionComponent.angle) + polygonPositionComponent.value
				);

				if (intersectionTest) {
					create_collision_event(
						collisions_accumulator,
						bulletEntity,
						dynamicEntity,
						rynx::math::rotatedXY(segment.getNormalXY(), polygonPositionComponent.angle),
						intersectionTest.point(),
						bulletPos.value,
						polyPos,
						0 // NOTE: penetration values don't really mean anything in projectile case.
					);
				}
			}
		}
	}


	void check_all(
		std::vector<collision_event>& collisions_accumulator,
		ecs_view ecs,
		uint64_t entityA,
		uint64_t entityB,
		rynx::vec3<float> a_pos,
		float a_radius,
		rynx::vec3<float> b_pos,
		float b_radius,
		rynx::vec3<float> normal,
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
			create_collision_event(
				collisions_accumulator,
				entA,
				entB,
				normal,
				(a_pos + b_pos) * 0.5f + normal * (a_radius - b_radius),
				a_pos,
				b_pos,
				penetration
			);
		}
	}

}


void rynx::ruleset::physics_2d::onFrameProcess(rynx::scheduler::context& context, float dt) {
	rynx::scheduler::barrier collisions_find_barrier("find collisions prereq");

	{
		context.add_task("Remove frame prev frame collisions", [](rynx::ecs::view<rynx::components::frame_collisions> ecs) {
			ecs.query().for_each([&](rynx::components::frame_collisions& collisions) {
				collisions.collision_indices.clear();
			});
		});
	}

	auto add_entities_sphere_tree = context.add_task("track collisions for new entities", [](rynx::scheduler::task& task_context, rynx::collision_detection& detection) {
		detection.track_entities(task_context);
	});

	auto update_entities_sphere_tree = context.add_task("update positions for spheretrees", [dt](rynx::scheduler::task& task_context, rynx::collision_detection& detection) {
		detection.update_entities(task_context, dt);
	});
	
	add_entities_sphere_tree.required_for(update_entities_sphere_tree);

	auto positionDataToSphereTree_task = context.add_task("Update position info to collision detection.", [](
		collision_detection& detection,
		rynx::scheduler::task& task) {
			detection.update_parallel(task);
		});

	update_entities_sphere_tree.required_for(positionDataToSphereTree_task);
	positionDataToSphereTree_task.required_for(collisions_find_barrier);

	auto updateBoundaryWorld = context.add_task(
		"Update boundary local -> boundary world",
		[](rynx::ecs::view<const components::position, components::boundary> ecs, rynx::scheduler::task& task_context) {
			ecs.query().for_each_parallel(task_context, [](components::position pos, components::boundary& boundary) {
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
				}, this_task
			);
		}
	);

	findCollisionsTask.depends_on(collisions_find_barrier);
	findCollisionsTask.depends_on(updateBoundaryWorld);

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

				barriers.emplace_back(task.parallel().for_each(0, overlaps->size(), 64).for_each([overlaps, extras, ecs](int64_t index) mutable {
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
				task.parallel().for_each(0, overlaps->size(), 2048).deferred_work().for_each([overlaps, extras, ecs, dt](int64_t index) mutable {
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
					rynx_assert(collision.normal.length_squared() < 1.1f, "normal should be unit length");

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

					const float inv_dt = 1.0f / dt;
					motion_a.acceleration += (proximity_force + soft_impact_force + tangent) * collision.a_body.inv_mass * inv_dt;
					motion_b.acceleration -= (proximity_force + soft_impact_force + tangent) * collision.b_body.inv_mass * inv_dt;

					{
						const float rotation_force_friction = tangent.cross2d(rel_pos_a);
						const float rotation_force_linear = soft_impact_force.cross2d(rel_pos_a);
						motion_a.angularAcceleration += (rotation_force_linear + rotation_force_friction) * collision.a_body.inv_moment_of_inertia * inv_dt;
					}

					{
						const float rotation_force_friction = tangent.cross2d(rel_pos_b);
						const float rotation_force_linear = soft_impact_force.cross2d(rel_pos_b);
						motion_b.angularAcceleration -= (rotation_force_linear + rotation_force_friction) * collision.b_body.inv_moment_of_inertia * inv_dt;
					}
				});
			}
	};

	context.add_task("collisions resolve", collision_resolution_first_stage).depends_on(findCollisionsTask);
}

