
#include <rynx/rulesets/collisions.hpp>

namespace {
	using ecs_view = rynx::ecs::view<
		const rynx::components::position,
		const rynx::components::radius,
		const rynx::components::boundary,
		const rynx::components::projectile,
		const rynx::components::collision_category,
		const rynx::components::motion,
		rynx::components::frame_collisions>;

	void store_collision(
		rynx::ecs::entity<ecs_view>& a,
		rynx::ecs::entity<ecs_view>& b,
		rynx::collision_detection::shape_type shapeA,
		rynx::collision_detection::shape_type shapeB,
		vec3<float> normal,
		vec3<float> collisionPoint,
		vec3<float> pos_a,
		vec3<float> pos_b,
		float penetration1,
		float penetration2
	) {
		rynx_assert(normal.lengthSquared() > 0.9f, "normal must be unit length");
		rynx_assert(normal.lengthSquared() < 1.1f, "normal must be unit length");

		auto* collisions_of_a = a.try_get<rynx::components::frame_collisions>();
		auto* collisions_of_b = b.try_get<rynx::components::frame_collisions>();

		const auto* motion_a = a.try_get<const rynx::components::motion>();
		const auto* motion_b = b.try_get<const rynx::components::motion>();
		
		vec3<float> relative_velocity;
		const vec3<float> relative_pos_a = collisionPoint - pos_a;
		const vec3<float> relative_pos_b = collisionPoint - pos_b;

		if (motion_a) {
			if (motion_b) {
				relative_velocity =
					(motion_a->velocity + math::velocity_at_point_2d(relative_pos_a, motion_a->angularVelocity)) -
					(motion_b->velocity + math::velocity_at_point_2d(relative_pos_b, motion_b->angularVelocity));
			}
			else {
				relative_velocity = (motion_a->velocity + math::velocity_at_point_2d(relative_pos_a, motion_a->angularVelocity));
			}
		}
		else {
			if (motion_b) {
				relative_velocity = -(motion_b->velocity + math::velocity_at_point_2d(relative_pos_b, motion_b->angularVelocity));
			}
		}

		if (collisions_of_a) {
			rynx::components::frame_collisions::entry collision_for_a;
			collision_for_a.categoryOfOther = b.get<const rynx::components::collision_category>().value;
			collision_for_a.collisionNormal = normal;
			collision_for_a.idOfOther = b.id();
			collision_for_a.shapeOfOther = shapeB;
			collision_for_a.collisionPointRelative = relative_pos_a;
			collision_for_a.penetration = penetration1;
			collision_for_a.other_has_collision_response = (collisions_of_b != nullptr);
			collision_for_a.collisionPointRelativeVelocity = relative_velocity;
			collisions_of_a->collisions.emplace_back(std::move(collision_for_a));
		}

		if (collisions_of_b) {
			rynx::components::frame_collisions::entry collision_for_b;
			collision_for_b.categoryOfOther = a.get<const rynx::components::collision_category>().value;
			collision_for_b.collisionNormal = -normal;
			collision_for_b.idOfOther = a.id();
			collision_for_b.shapeOfOther = shapeA;
			collision_for_b.collisionPointRelative = relative_pos_b;
			collision_for_b.penetration = penetration2;
			collision_for_b.other_has_collision_response = (collisions_of_a != nullptr);
			collision_for_b.collisionPointRelativeVelocity = -relative_velocity;
			collisions_of_b->collisions.emplace_back(std::move(collision_for_b));
		}
	}

	void check_polygon_ball(
		rynx::ecs::entity<ecs_view>& polygon,
		rynx::ecs::entity<ecs_view>& ball
	) {
		const auto& posA = polygon.get<const rynx::components::position>();
		const auto& posB = ball.get<const rynx::components::position>();
		const auto& boundaryA = polygon.get<const rynx::components::boundary>();
		const float radiusB = ball.get<const rynx::components::radius>().r;

		for (auto&& segment : boundaryA.segments_world) {
			const auto pointToLineSegment = math::pointDistanceLineSegment(segment.p1, segment.p2, posB.value);
			const float dist = pointToLineSegment.first;
			if (dist < radiusB) {
				store_collision(
					polygon,
					ball,
					rynx::collision_detection::shape_type::Boundary,
					rynx::collision_detection::shape_type::Sphere,
					-segment.getNormalXY(),
					pointToLineSegment.second,
					posA.value,
					posB.value,
					radiusB - dist,
					radiusB - dist
				);
			}
		}
	}

	void check_polygon_polygon(
		rynx::ecs::entity<ecs_view>& poly1,
		rynx::ecs::entity<ecs_view>& poly2
	) {
		const auto& posA = poly1.get<const rynx::components::position>();
		const auto& posB = poly2.get<const rynx::components::position>();
		const auto& boundaryA = poly1.get<const rynx::components::boundary>();
		const auto& boundaryB = poly2.get<const rynx::components::boundary>();

		const float radiusB = poly2.get<const rynx::components::radius>().r;

		// NOTE: this could be optimized by having static boundary objects individual segments in a sphere_tree and testing against that.
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
						const float d1_a = (collisionPoint.point() - p1).lengthSquared();
						const float d2_a = (collisionPoint.point() - p2).lengthSquared();

						const float d1_b = (collisionPoint.point() - q1).lengthSquared();
						const float d2_b = (collisionPoint.point() - q2).lengthSquared();

						vec3<float> normal;
						if (((d1_a < d1_b) & (d1_a < d2_b)) | ((d2_a < d1_b) & (d2_a < d2_b))) {
							normal = segmentB.getNormalXY();
						}
						else {
							normal = segmentA.getNormalXY();
						}

						store_collision(
							poly1,
							poly2,
							rynx::collision_detection::shape_type::Boundary,
							rynx::collision_detection::shape_type::Boundary,
							normal,
							collisionPoint.point(),
							posA.value,
							posB.value,
							d1_b < d2_b ? d1_b : d2_b,
							d1_a < d2_a ? d1_a : d2_a
						);
					}
				}
			}
		}
	}

	void check_projectile_ball(
		rynx::ecs::entity<ecs_view>& bulletEntity,
		rynx::ecs::entity<ecs_view>& dynamicEntity
	) {
		const auto& bulletPos = bulletEntity.get<const rynx::components::position>();
		const auto& bulletMotion = bulletEntity.get<const rynx::components::motion>();

		auto ballPos = dynamicEntity.get<const rynx::components::position>().value;
		auto pointDistanceResult = math::pointDistanceLineSegment(bulletPos.value, bulletPos.value - bulletMotion.velocity, ballPos);

		if (pointDistanceResult.first < dynamicEntity.get<const rynx::components::radius>().r + bulletEntity.get<const rynx::components::radius>().r) {
			store_collision(
				bulletEntity,
				dynamicEntity,
				rynx::collision_detection::shape_type::Projectile,
				rynx::collision_detection::shape_type::Sphere,
				((bulletPos.value - bulletMotion.velocity) - ballPos).normalizeApprox(),
				pointDistanceResult.second,
				bulletPos.value,
				ballPos,
				0,
				0 // NOTE: penetration values don't really mean anything in projectile case.
			);
		}
	}

	// TODO: optimize if necessary.
	void check_projectile_polygon(
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
					store_collision(
						bulletEntity,
						dynamicEntity,
						rynx::collision_detection::shape_type::Projectile,
						rynx::collision_detection::shape_type::Boundary,
						math::rotatedXY(segment.getNormalXY(), polygonPositionComponent.angle),
						intersectionTest.point(),
						bulletPos.value,
						polyPos,
						0,
						0 // NOTE: penetration values don't really mean anything in projectile case.
					);
				}
			}
		}
	}
}

void rynx::ruleset::collisions::check_all(ecs_view ecs, uint64_t entityA, uint64_t entityB, vec3<float> normal, float penetration) {
	auto entA = ecs[entityA];
	auto entB = ecs[entityB];

	bool hasBoundaryA = entA.has<components::boundary>();
	bool hasBoundaryB = entB.has<components::boundary>();
	bool hasProjectileA = entA.has<components::projectile>();
	bool hasProjectileB = entB.has<components::projectile>();

	bool anyProjectile = hasProjectileA | hasProjectileB;
	bool anyBoundary = hasBoundaryA | hasBoundaryB;

	if (anyProjectile) {
		if (anyBoundary) {
			// projectiles do not have boundary shapes. this must be projectile vs polygon check.
			if (hasProjectileA) {
				check_projectile_polygon(entA, entB);
			}
			else {
				check_projectile_polygon(entB, entA);
			}
		}
		else if (hasProjectileA & hasProjectileB) {
			// projectile vs projectile check.
			// check_projectile_projectile(entA, entB);
		}
		else {
			// projectile vs ball check.
			if (hasProjectileA) {
				check_projectile_ball(entA, entB);
			}
			else {
				check_projectile_ball(entB, entA);
			}
		}
	}
	else if (anyBoundary) {
		if (hasBoundaryA & hasBoundaryB) {
			check_polygon_polygon(entA, entB);
		}
		else if (hasBoundaryA) {
			check_polygon_ball(entA, entB);
		}
		else {
			check_polygon_ball(entB, entA);
		}
	}
	else {
		// is ball vs ball collision. no further check needed, sphere tree said the balls collided.
		vec3<float> pos_a = entA.get<const components::position>().value;
		vec3<float> pos_b = entB.get<const components::position>().value;
		float r_a = entA.get<const components::radius>().r;
		float r_b = entB.get<const components::radius>().r;

		store_collision(
			entA,
			entB,
			collision_detection::shape_type::Sphere,
			collision_detection::shape_type::Sphere,
			normal,
			(pos_a + pos_b) * 0.5f + normal * (r_a - r_b),
			pos_a,
			pos_b,
			penetration,
			penetration
		);
	}
}

