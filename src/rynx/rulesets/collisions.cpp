
#include <rynx/rulesets/collisions.hpp>

#include <rynx/application/components.hpp>
#include <rynx/tech/collision_detection.hpp>

#include <rynx/scheduler/task.hpp>
#include <rynx/scheduler/context.hpp>
#include <rynx/scheduler/barrier.hpp>

#include <rynx/tech/sphere_tree.hpp>
#include <rynx/tech/parallel/accumulator.hpp>

namespace {

	rynx::components::transform::motion g_dummy_motion;

	struct collision_event {
		uint64_t a_id;
		uint64_t b_id;
		rynx::components::phys::body a_body;
		rynx::components::phys::body b_body;
		rynx::components::transform::motion* a_motion;
		rynx::components::transform::motion* b_motion;
		rynx::vec3<float> a_pos;
		rynx::vec3<float> b_pos;
		rynx::vec3<float> c_pos; // collision point in world space
		rynx::vec3<float> normal;
		float penetration;
	};

	using ecs_view = rynx::ecs::view<
		const rynx::components::transform::position,
		const rynx::components::transform::radius,
		const rynx::components::projectile,
		const rynx::components::transform::motion,
		const rynx::components::phys::boundary,
		const rynx::components::phys::body>;

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

		event.a_body = a.get<const rynx::components::phys::body>();
		event.b_body = b.get<const rynx::components::phys::body>();

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
		const rynx::vec3f ball_position,
		const rynx::vec3f polygon_position,
		const float ball_radius
	) {
		const auto& boundaryA = polygon.get<const rynx::components::phys::boundary>();
		const float radiusSqr = ball_radius * ball_radius;
		
		int collisions_detected = 0;
		rynx::vec3f collision_normal;
		rynx::vec3f collision_point;
		float penetration = 0;

		for (size_t i = 0; i < boundaryA.segments_world.size(); ++i) {
			const auto segment = boundaryA.segments_world.segment(i);
			const auto pointToLineSegment = rynx::math::pointDistanceLineSegmentSquared(segment.p1, segment.p2, ball_position);
			const float distSqr = pointToLineSegment.first;

			if (distSqr < radiusSqr) {
				auto normal = (pointToLineSegment.second - ball_position).normalize();
				float penetration_current = ball_radius - rynx::math::sqrt_approx(distSqr);
				penetration = std::max(penetration, penetration_current);
				collision_normal += normal;
				collision_point += pointToLineSegment.second;
				collisions_detected += 1;
			}
		}

		if (collisions_detected > 0) {
			create_collision_event(
				collisions_accumulator,
				polygon,
				ball,
				(collision_normal / float(collisions_detected)).normalize(),
				collision_point / float(collisions_detected),
				polygon_position,
				ball_position,
				penetration
			);
		}
	}

	int clip_segment_to_line(std::array<rynx::vec3f, 2>& vOut, const std::array<rynx::vec3f, 2>& vIn, rynx::vec3f normal, float offset) {
		int numOut = 0;
		float d0 = normal.dot(vIn[0]) - offset;
		float d1 = normal.dot(vIn[1]) - offset;

		if (d0 <= 0.0f) vOut[numOut++] = vIn[0];
		if (d1 <= 0.0f) vOut[numOut++] = vIn[1];

		if (d0 * d1 < 0.0f) {
			float interp = d0 / (d0 - d1);
			vOut[numOut++] = vIn[0] + (vIn[1] - vIn[0]) * interp;
		}

		return numOut;
	}

	void check_polygon_polygon(
		std::vector<collision_event>& collisions_accumulator,
		rynx::ecs::entity<ecs_view> poly1,
		rynx::ecs::entity<ecs_view> poly2,
		rynx::vec3f pos_a,
		rynx::vec3f pos_b,
		[[maybe_unused]] float radius_a,
		[[maybe_unused]] float radius_b
	) {
		const auto& boundaryA_comp = poly1.get<const rynx::components::phys::boundary>();
		const auto& boundaryB_comp = poly2.get<const rynx::components::phys::boundary>();

		const rynx::polygon& polyA = boundaryA_comp.segments_world;
		const rynx::polygon& polyB = boundaryB_comp.segments_world;

		if (polyA.size() < 2 || polyB.size() < 2) return;

		struct EdgeInfo {
			float pen;
			rynx::vec3f normal;
			int index;
		};

		auto find_min_penetration = [](const rynx::polygon& a, const rynx::polygon& b) -> EdgeInfo {
			float min_pen = std::numeric_limits<float>::max();
			int min_index = -1;
			rynx::vec3f min_normal;

			for (size_t i = 0; i < a.size(); ++i) {
				rynx::vec3f normal = a.segment_normal(i);
				
				float min_proj_b = std::numeric_limits<float>::max();
				for (size_t j = 0; j < b.size(); ++j) {
					float proj = b.vertex_position(j).dot(normal);
					if (proj < min_proj_b) {
						min_proj_b = proj;
					}
				}
				
				float max_proj_a = a.vertex_position(i).dot(normal);
				
				float penetration = max_proj_a - min_proj_b;
				if (penetration <= 0) {
					return {penetration, normal, static_cast<int>(i)}; // Separating axis
				}
				
				if (penetration < min_pen) {
					min_pen = penetration;
					min_index = static_cast<int>(i);
					min_normal = normal;
				}
			}
			return {min_pen, min_normal, min_index};
		};

		EdgeInfo edgeA = find_min_penetration(polyA, polyB);
		if (edgeA.pen <= 0.0f) return;

		EdgeInfo edgeB = find_min_penetration(polyB, polyA);
		if (edgeB.pen <= 0.0f) return;

		bool flip = false;
		float pen = edgeA.pen;
		rynx::vec3f normal = edgeA.normal;
		int refIndex = edgeA.index;
		const rynx::polygon* refPoly = &polyA;
		const rynx::polygon* incPoly = &polyB;

		// We apply a slight bias to favor A to avoid flip-flopping edges
		if (edgeB.pen < edgeA.pen * 0.95f) {
			flip = true;
			pen = edgeB.pen;
			normal = edgeB.normal; // normal points OUT of B (towards A)
			refIndex = edgeB.index;
			refPoly = &polyB;
			incPoly = &polyA;
		}

		// Find incident edge
		int incIndex = 0;
		float min_dot = std::numeric_limits<float>::max();
		for (size_t i = 0; i < incPoly->size(); ++i) {
			float d = incPoly->segment_normal(i).dot(normal);
			if (d < min_dot) {
				min_dot = d;
				incIndex = static_cast<int>(i);
			}
		}

		std::array<rynx::vec3f, 2> incEdge = {
			incPoly->vertex_position(incIndex),
			incPoly->vertex_position(incIndex + 1)
		};

		rynx::vec3f ref_v1 = refPoly->vertex_position(refIndex);
		rynx::vec3f ref_v2 = refPoly->vertex_position(refIndex + 1);
		
		rynx::vec3f refEdgeDir = (ref_v2 - ref_v1).normalize();

		// Clip incident edge against ref edge side planes
		float offset1 = -refEdgeDir.dot(ref_v1);
		std::array<rynx::vec3f, 2> clip1;
		int numClip1 = clip_segment_to_line(clip1, incEdge, -refEdgeDir, offset1);
		if (numClip1 < 2) return;

		float offset2 = refEdgeDir.dot(ref_v2);
		std::array<rynx::vec3f, 2> clip2;
		int numClip2 = clip_segment_to_line(clip2, clip1, refEdgeDir, offset2);
		if (numClip2 < 2) return;

		// Now clip against ref edge front plane
		rynx::vec3f collision_normal = flip ? normal : -normal; // Normal from B to A
		
		float refOffset = normal.dot(ref_v1);
		
		for (int i = 0; i < 2; ++i) {
			float depth = normal.dot(clip2[i]) - refOffset;
			if (depth <= 0.0f) { // Penetrating
				create_collision_event(
					collisions_accumulator,
					poly1,
					poly2,
					collision_normal,
					clip2[i],
					pos_a,
					pos_b,
					-depth
				);
			}
		}
	}

	void check_projectile_ball(
		std::vector<collision_event>& collisions_accumulator,
		rynx::ecs::entity<ecs_view>& bulletEntity,
		rynx::ecs::entity<ecs_view>& dynamicEntity
	) {
		const auto& bulletPos = bulletEntity.get<const rynx::components::transform::position>();
		const auto& bulletMotion = bulletEntity.get<const rynx::components::transform::motion>();

		auto ballPos = dynamicEntity.get<const rynx::components::transform::position>().value;
		auto pointDistanceResult = rynx::math::pointDistanceLineSegment(bulletPos.value, bulletPos.value - bulletMotion.velocity, ballPos);

		if (pointDistanceResult.first < dynamicEntity.get<const rynx::components::transform::radius>().r + bulletEntity.get<const rynx::components::transform::radius>().r) {
			create_collision_event(
				collisions_accumulator,
				bulletEntity,
				dynamicEntity,
				((bulletPos.value - bulletMotion.velocity) - ballPos).normalize(), // normal
				pointDistanceResult.second, // collision point
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
		rynx::ecs::entity<ecs_view>& dynamicEntity,
		uint32_t partIndex
	) {
		const auto& bulletPos = bulletEntity.get<const rynx::components::transform::position>();
		const auto& bulletMotion = bulletEntity.get<const rynx::components::transform::motion>();

		const auto& polygonPositionComponent = dynamicEntity.get<const rynx::components::transform::position>();
		auto polyPos = polygonPositionComponent.value;
		auto pointDistanceResult = rynx::math::pointDistanceLineSegment(bulletPos.value, bulletPos.value - bulletMotion.velocity, polyPos);

		if (pointDistanceResult.first < dynamicEntity.get<const rynx::components::transform::radius>().r + bulletEntity.get<const rynx::components::transform::radius>().r) {
			const auto& boundary = dynamicEntity.get<const rynx::components::phys::boundary>();
			const auto segment = boundary.segments_world.segment(partIndex);
			
			float sin_v = rynx::math::sin(polygonPositionComponent.angle);
			float cos_v = rynx::math::cos(polygonPositionComponent.angle);
			
			auto intersectionTest = rynx::math::lineSegmentIntersectionPoint(
				bulletPos.value,
				bulletPos.value - bulletMotion.velocity,
				rynx::math::rotatedXY(segment.p1, sin_v, cos_v) + polygonPositionComponent.value,
				rynx::math::rotatedXY(segment.p2, sin_v, cos_v) + polygonPositionComponent.value
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


	void check_all(
		std::vector<collision_event>& collisions_accumulator,
		ecs_view ecs,
		const rynx::collision_detection::collision_params& params)
	{
		auto entityA = params.id1;
		auto entityB = params.id2;
		
		rynx::vec3f a_pos = params.pos1;
		rynx::vec3f b_pos = params.pos2;
		rynx::vec3f normal = params.normal;

		float a_radius = params.radius1;
		float b_radius = params.radius2;
		float penetration = params.penetration;

		auto entA = ecs[entityA];
		auto entB = ecs[entityB];

		bool hasBoundaryA = params.kind1 == rynx::collision_detection::kind::boundary2d;
		bool hasBoundaryB = params.kind2 == rynx::collision_detection::kind::boundary2d;
		bool hasProjectileA = params.kind1 == rynx::collision_detection::kind::projectile;
		bool hasProjectileB = params.kind2 == rynx::collision_detection::kind::projectile;

		bool anyProjectile = hasProjectileA | hasProjectileB;
		bool anyBoundary = hasBoundaryA | hasBoundaryB;

		if (anyProjectile | anyBoundary) {
			if (anyProjectile) {
				if (anyBoundary) {
					// projectiles do not have boundary shapes. this must be projectile vs polygon check.
					if (hasProjectileA) {
						check_projectile_polygon(collisions_accumulator, entA, entB, params.part2);
					}
					else {
						check_projectile_polygon(collisions_accumulator, entB, entA, params.part1);
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
					check_polygon_polygon(collisions_accumulator, entA, entB, a_pos, b_pos, a_radius, b_radius);
				}
				else if (hasBoundaryA) {
					check_polygon_ball(collisions_accumulator, entA, entB, b_pos, a_pos, b_radius);
				}
				else {
					check_polygon_ball(collisions_accumulator, entB, entA, a_pos, b_pos, a_radius);
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
				(a_pos + b_pos) * 0.5f + normal * (b_radius - a_radius),
				a_pos,
				b_pos,
				penetration
			);
		}
	}

}

void rynx::ruleset::physics_2d::clear(rynx::scheduler::context& ctx) {
	auto& detection = ctx.get_resource<rynx::collision_detection>();
	detection.clear();
}

void rynx::ruleset::physics_2d::onFrameProcess(rynx::scheduler::context& context, float dt) {
	rynx::scheduler::barrier collisions_find_barrier("find collisions prereq");

	{
		context.add_task("Remove frame prev frame collisions", [](rynx::ecs::view<rynx::components::phys::collision_events> ecs) {
			ecs.query().for_each([](rynx::components::phys::collision_events& collisions) {
				collisions.events.clear();
			});
		});
	}

	auto add_entities_sphere_tree = context.add_task("collisions: new entities", [](rynx::scheduler::task& task_context, rynx::collision_detection& detection) {
		detection.track_entities(task_context);
	});

	auto update_entities_sphere_tree = context.add_task("update positions for spheretrees", [dt](rynx::scheduler::task& task_context, rynx::collision_detection& detection) {
		detection.update_entities(task_context, dt);
	});
	
	add_entities_sphere_tree.required_for(update_entities_sphere_tree);

	auto positionDataToSphereTree_task = context.add_task("Update position info to collision detection.", [](
		collision_detection& detection,
		rynx::scheduler::task& task) {
			detection.update_sphere_trees_parallel(task);
		}
	);

	update_entities_sphere_tree.required_for(positionDataToSphereTree_task);
	positionDataToSphereTree_task.required_for(collisions_find_barrier);

	auto updateBoundaryWorld = context.add_task(
		"Update boundary local -> boundary world",
		[](rynx::ecs::view<const components::transform::position, rynx::components::phys::boundary> ecs, rynx::scheduler::task& task_context) {
			ecs.query().in<components::transform::motion>().for_each_parallel(task_context, [](components::transform::position pos, components::phys::boundary& boundary) {
				boundary.update_world_positions(pos.value, pos.angle);
			});
		}
	);

	rynx::shared_ptr<rynx::parallel_accumulator<collision_event>> collisions_accumulator = rynx::make_shared<rynx::parallel_accumulator<collision_event>>();
	auto findCollisionsTask = context.add_task("Find collisions", [collisions_accumulator](
		rynx::ecs::view<
		const components::transform::position,
		const components::transform::radius,
		const components::projectile,
		const components::transform::motion,
		const components::phys::boundary,
		const components::phys::body> ecs,
		collision_detection& detection,
		rynx::scheduler::task& this_task) mutable
		{
			auto accumulator_copy = collisions_accumulator;
			detection.for_each_collision_parallel(collisions_accumulator, [&detection, ecs](
				std::vector<collision_event>& accumulator,
				const rynx::collision_detection::collision_params& params) {
					check_all(accumulator, ecs, params);
				}, this_task
			);
		}
	);
	
	findCollisionsTask.depends_on(collisions_find_barrier);
	findCollisionsTask.depends_on(updateBoundaryWorld);

	auto collision_resolution_first_stage = [dt = dt, collisions_accumulator](
		rynx::ecs::view<components::transform::motion, rynx::components::phys::collision_events> ecs,
		rynx::scheduler::task& task)
	{
		struct event_extra_info {
			vec3<float> relative_position_tangent_a;
			float relative_position_length_a;
			vec3<float> relative_position_tangent_b;
			float relative_position_length_b;
		};

		auto overlaps_vector = rynx::make_shared<std::vector<rynx::shared_ptr<std::vector<collision_event>>>>();
		auto extra_infos = rynx::make_shared<std::vector<rynx::shared_ptr<std::vector<event_extra_info>>>>();

		collisions_accumulator->for_each([&overlaps_vector](std::vector<collision_event>& overlaps) mutable {
			overlaps_vector->emplace_back(rynx::make_shared<std::vector<collision_event>>(std::move(overlaps)));
		});
		extra_infos->resize(overlaps_vector->size());
		
		for (size_t i = 0; i < overlaps_vector->size(); ++i) {
			extra_infos->operator[](i) = rynx::make_shared<std::vector<event_extra_info>>();
			extra_infos->operator[](i)->resize(overlaps_vector->operator[](i)->size());
		}

		g_dummy_motion = rynx::components::transform::motion{};
		std::vector<rynx::scheduler::barrier> barriers;
		{
			rynx_profile("collisions", "fetch motion components");
			{
				auto parallel_ops = task.parallel();
				for (size_t i = 0; i < overlaps_vector->size(); ++i) {
					auto overlaps = overlaps_vector->operator[](i);
					auto extras = extra_infos->operator[](i);

					parallel_ops.range(0, overlaps->size(), 64).execute([overlaps, extras, ecs](int64_t index) mutable {
						auto& collision = overlaps->operator[](index);
						collision.a_motion = ecs[collision.a_id].try_get<components::transform::motion>();
						collision.b_motion = ecs[collision.b_id].try_get<components::transform::motion>();

						if (collision.a_motion == nullptr) {
							collision.a_motion = &g_dummy_motion;
						}

						if (collision.b_motion == nullptr) {
							collision.b_motion = &g_dummy_motion;
						}

						auto& extra = extras->operator[](index);
						const vec3<float> rel_pos_a = collision.c_pos - collision.a_pos;
						const vec3<float> rel_pos_b = collision.c_pos - collision.b_pos;

						const float rel_pos_len_a = rel_pos_a.length();
						const float rel_pos_len_b = rel_pos_b.length();
						extra.relative_position_length_a = rel_pos_len_a;
						extra.relative_position_length_b = rel_pos_len_b;
						extra.relative_position_tangent_a = rel_pos_a.normal2d() / (rel_pos_len_a + std::numeric_limits<float>::epsilon());
						extra.relative_position_tangent_b = rel_pos_b.normal2d() / (rel_pos_len_b + std::numeric_limits<float>::epsilon());

						if (auto* storage = ecs[collision.a_id].try_get<rynx::components::phys::collision_events>()) {
							rynx::components::phys::collision_events::event e;
							e.body = collision.b_body;
							e.id = collision.b_id;
							e.normal = collision.normal;

							auto velocity_at_point = [](float rel_pos_len, vec3<float> rel_pos_tangent, const components::transform::motion& m) {
								return m.velocity + rel_pos_len * (m.angularVelocity) * rel_pos_tangent;
							};

							const vec3<float> rel_v_a = velocity_at_point(extra.relative_position_length_a, extra.relative_position_tangent_a, *collision.a_motion);
							const vec3<float> rel_v_b = velocity_at_point(extra.relative_position_length_b, extra.relative_position_tangent_b, *collision.b_motion);
							const vec3<float> total_rel_v = rel_v_a - rel_v_b;
							e.relative_velocity = total_rel_v;
							storage->events.emplace_back(e);
						}

						if (auto* storage = ecs[collision.b_id].try_get<rynx::components::phys::collision_events>()) {
							rynx::components::phys::collision_events::event e;
							e.body = collision.a_body;
							e.id = collision.a_id;
							e.normal = -collision.normal;

							auto velocity_at_point = [](float rel_pos_len, vec3<float> rel_pos_tangent, const components::transform::motion& m) {
								return m.velocity + rel_pos_len * (m.angularVelocity) * rel_pos_tangent;
							};

							const vec3<float> rel_v_a = velocity_at_point(extra.relative_position_length_a, extra.relative_position_tangent_a, *collision.a_motion);
							const vec3<float> rel_v_b = velocity_at_point(extra.relative_position_length_b, extra.relative_position_tangent_b, *collision.b_motion);
							const vec3<float> total_rel_v = rel_v_a - rel_v_b;
							e.relative_velocity = -total_rel_v;
							storage->events.emplace_back(e);
						}
					});
				}
				barriers.emplace_back(parallel_ops.barrier());
			}
		}

		auto collisions_resolve_task = task.extend_task_execute_parallel("collision resolve", [ecs, overlaps_vector, extra_infos, dt](rynx::scheduler::task& task) {
			// NOTE TODO: The task is not thread-safe. We are now resolving all collision events in parallel, which means
			//            that we are resolving multiple collisions for the same entity in parallel, which will yield incorrect results.
			//            however, in practice this issue doesn't appear to be great. Might be that the iterative nature of approaching
			//            some global correct solution mitigates the damage the error cases cause.
			rynx_profile("collisions", "resolve 10x");
			for (int i = 0; i < 10; ++i)
				for (size_t k = 0; k < overlaps_vector->size(); ++k) {
					auto& overlaps = overlaps_vector->operator[](k);
					auto& extras = extra_infos->operator[](k);
					task.parallel().range(0, overlaps->size(), 2048).deferred_work().execute([overlaps, extras, ecs, dt](int64_t index) mutable {
						const auto& collision = overlaps->operator[](index);
						components::transform::motion& motion_a = *collision.a_motion;
						components::transform::motion& motion_b = *collision.b_motion;

						auto velocity_at_point = [](float rel_pos_len, vec3<float> rel_pos_tangent, const components::transform::motion& m, float dt) {
							return m.velocity + m.acceleration * dt + rel_pos_len * (m.angularVelocity + m.angularAcceleration * dt) * rel_pos_tangent;
						};

						const auto& extra = extras->operator[](index);
						const vec3<float> rel_v_a = velocity_at_point(extra.relative_position_length_a, extra.relative_position_tangent_a, motion_a, dt);
						const vec3<float> rel_v_b = velocity_at_point(extra.relative_position_length_b, extra.relative_position_tangent_b, motion_b, dt);
						const vec3<float> total_rel_v = rel_v_a - rel_v_b;

						const float impact_power = -total_rel_v.dot(collision.normal);
						if (impact_power < 0)
							return;

						constexpr float bias_start = 0.05f;
						const float overlap_value = collision.penetration - bias_start;
						rynx_assert(collision.normal.length_squared() < 1.1f, "normal should be unit length");

						const vec3<float> rel_pos_a = collision.c_pos - collision.a_pos;
						const vec3<float> rel_pos_b = collision.c_pos - collision.b_pos;

						const float inertia1 = sqr(rel_pos_a.cross2d(collision.normal)) * collision.a_body.inv_moment_of_inertia;
						const float inertia2 = sqr(rel_pos_b.cross2d(collision.normal)) * collision.b_body.inv_moment_of_inertia;
						const float collision_elasticity = (collision.a_body.collision_elasticity + collision.b_body.collision_elasticity) * 0.5f; // combine some fun way

						const float top = (1.0f + collision_elasticity) * impact_power;
						const float bot = collision.a_body.inv_mass + collision.b_body.inv_mass + inertia1 + inertia2;

						const float soft_j = top / bot;

						// This *60 is not the same as dt. Just a random constant.
						const auto proximity_force =
							collision.normal * (overlap_value > 0) * overlap_value * 0.5f *
							(collision.a_body.bias_multiply + collision.b_body.bias_multiply) / (collision.a_body.inv_mass + collision.b_body.inv_mass);

						const vec3<float> normal = collision.normal;
						const auto impact_linear_force = normal * soft_j;

						const float mu = (collision.a_body.friction_multiplier + collision.b_body.friction_multiplier) * 0.5f;
						vec3<float> tangent = normal.normal2d();
						tangent *= ((tangent.dot(total_rel_v) > 0) * 2.0f - 1.0f);

						float friction_power = -tangent.dot(total_rel_v) * mu;

						// if we want to limit friction according to physics, this would do the trick.
						if constexpr (false) {
							while (friction_power * friction_power > soft_j * soft_j)
								friction_power *= 0.9f;
						}

						const vec3<float> friction_linear_force = tangent * friction_power;


						// proximity_force = bias (to keep the simulation stable, and prevent objects from sinking into each other over time)
						// impact_linear_force = linear collision response along the collision normal
						// friction_linear_force = friction response along the collision tangent
						const vec3<float> total_force = proximity_force + impact_linear_force + friction_linear_force;
						const float inv_dt = 1.0f / dt;
						const vec3f force_mask_xy(1.0f, 1.0f, 0.0f);
						motion_a.acceleration += force_mask_xy * total_force * collision.a_body.inv_mass * inv_dt;
						motion_b.acceleration -= force_mask_xy * total_force * collision.b_body.inv_mass * inv_dt;

						motion_a.angularAcceleration += rel_pos_a.cross2d(total_force) * collision.a_body.inv_moment_of_inertia * inv_dt;
						motion_b.angularAcceleration -= rel_pos_b.cross2d(total_force) * collision.b_body.inv_moment_of_inertia * inv_dt;
					});
				}
		});
		for (auto&& bar : barriers)
			collisions_resolve_task.depends_on(bar);
	};

	context.add_task("collisions resolve", collision_resolution_first_stage).depends_on(findCollisionsTask);
}

void rynx::ruleset::physics_2d::on_entities_erased(rynx::scheduler::context& context, const std::vector<rynx::ecs::id>& ids) {
	auto& ecs = context.get_resource<rynx::ecs>();
	auto& collision_detection = context.get_resource<rynx::collision_detection>();
	for (auto id : ids) {
		if (ecs[id].has<rynx::components::phys::collisions>()) {
			auto collisions = ecs[id].get<rynx::components::phys::collisions>();
			collision_detection.erase(ecs, id.value, collisions.category);
		}
	}
}