
#pragma once

#include <rynx/tech/ecs.hpp>
#include <rynx/tech/object_storage.hpp>
#include <rynx/tech/sphere_tree.hpp>
#include <rynx/graphics/mesh/polygon.hpp>

#include <rynx/tech/collision_detection.hpp>
#include <rynx/tech/math/vector.hpp>

#include <string>

class Mesh;

namespace rynx {
	namespace components {
		struct color {
			color() {}
			color(vec4<float> value) : value(value) {}
			vec4<float> value = { 1, 1, 1, 1 };
		};

		struct position {
			position() = default;
			position(vec3<float> pos, float angle = 0) : value(pos), angle(angle) {}
			vec3<float> value;
			float angle;
		};

		struct radius {
			radius() = default;
			radius(float r) : r(r) {}
			float r = 0;
		};

		struct lifetime {
			lifetime() : value(0) {}
			lifetime(int frames) : value(frames) {}
			int value;
		};

		struct physical_body {
			physical_body() = default;
			physical_body(float mass, float moment_of_inertia, float elasticity, float friction)
				: inv_mass(1.0f / mass)
				, inv_moment_of_inertia(1.0f / moment_of_inertia)
				, collision_elasticity(elasticity)
				, friction_multiplier(friction)
			{}
			
			float inv_mass = 1.0f;
			float inv_moment_of_inertia = 1.0f;
			float collision_elasticity = 0.5f; // [0, 1[
			float friction_multiplier = 1.0f; // [0, 1]
		};

		struct dampening {
			float linearDampening = 1.0f;
			float angularDampening = 1.0f;
		};

		struct motion {
			motion() = default;
			motion(vec3<float> v, float av) : velocity(v), angularVelocity(av) {}

			vec3<float> velocity;
			float angularVelocity = 0;
			
			vec3<float> acceleration;
			float angularAcceleration = 0;

			vec3<float> velocity_at_point(vec3<float> relative_point) const {
				return velocity + angularVelocity * relative_point.lengthApprox() * vec3<float>(-relative_point.y, +relative_point.x, 0).normalizeApprox();
			}
		};

		struct boundary {
			using boundary_t = decltype(Polygon<vec3<float>>().generateBoundary_Outside());
			boundary(boundary_t&& b) : segments_local(std::move(b)) {
				segments_world.resize(segments_local.size());
			}
			boundary_t segments_local;
			boundary_t segments_world;
		};

		struct projectile {}; // tag for fast moving items in collision detection.

		struct frame_collisions {
			struct entry {
				entry() = default;
				rynx::ecs::id idOfOther = 0;
				rynx::collision_detection::category_id categoryOfOther = 0;
				rynx::collision_detection::shape_type shapeOfOther = rynx::collision_detection::shape_type::Sphere;
				vec3<float> collisionNormal;
				vec3<float> collisionPointRelative;
				vec3<float> collisionPointRelativeVelocity;
				float penetration;
				bool other_has_collision_response;
			};
			std::vector<entry> collisions;
		};

		struct rope {
			rynx::ecs::id id_a;
			rynx::ecs::id id_b;

			vec3<float> point_a;
			vec3<float> point_b;

			float length;
			float strength;
		};

		struct collision_category {
			collision_category() : value(-1) {}
			collision_category(rynx::collision_detection::category_id category) : value(category) {}
			rynx::collision_detection::category_id value;
		};

		struct mesh {
			Mesh* m;
		};

		struct texture {
			std::string tex;
		};
	}
}