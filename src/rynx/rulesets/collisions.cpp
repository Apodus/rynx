
#include <rynx/rulesets/collisions.hpp>

namespace {
	using ecs_view = rynx::ecs::view<
		const rynx::components::position,
		const rynx::components::radius,
		const rynx::components::boundary,
		const rynx::components::projectile,
		const rynx::components::collision_category,
		const rynx::components::motion,
		const rynx::components::physical_body>;

	[[nodiscard]] rynx::ruleset::physics_2d::collision_event store_collision(
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

		rynx::ruleset::physics_2d::collision_event event;
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
		std::vector<rynx::ruleset::physics_2d::collision_event>& collisions_accumulator, 
		rynx::ecs::entity<ecs_view> polygon,
		rynx::ecs::entity<ecs_view> ball,
		const vec3<float> polygon_position,
		const vec3<float> ball_position,
		const float ball_radius
	) {
		const auto& boundaryA = polygon.get<const rynx::components::boundary>();
		for (auto&& segment : boundaryA.segments_world) {
			const auto pointToLineSegment = math::pointDistanceLineSegment(segment.p1, segment.p2, ball_position);
			const float dist = pointToLineSegment.first;
			if (dist < ball_radius) {
				auto normal = (pointToLineSegment.second - ball_position).normalize();
				normal *= 2.0f * (normal.dot(polygon_position - pointToLineSegment.second) > 0) - 1.0f;

				collisions_accumulator.emplace_back(store_collision(
					polygon,
					ball,
					normal,
					pointToLineSegment.second,
					polygon_position,
					ball_position,
					ball_radius - dist
				));
			}
		}
	}

	void check_polygon_polygon(
		std::vector<rynx::ruleset::physics_2d::collision_event>& collisions_accumulator,
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
		std::vector<rynx::ruleset::physics_2d::collision_event>& collisions_accumulator,
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
		std::vector<rynx::ruleset::physics_2d::collision_event>& collisions_accumulator,
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
}

void rynx::ruleset::physics_2d::check_all(
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

	bool hasBoundaryA = entA.has<components::boundary>();
	bool hasBoundaryB = entB.has<components::boundary>();
	bool hasProjectileA = entA.has<components::projectile>();
	bool hasProjectileB = entB.has<components::projectile>();

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

