
#pragma once

#include <rynx/tech/ecs.hpp>
#include <rynx/tech/object_storage.hpp>
#include <rynx/graphics/mesh/polygon.hpp>
#include <rynx/tech/math/vector.hpp>
#include <rynx/tech/collision_detection.hpp>

#include <string>

class Mesh;

namespace rynx {
	template<typename T> struct value_segment {
		T begin;
		T end;

		T linear(float v) const { return begin * v + end * (1.0f - v); }
	};

	namespace components {
		struct color {
			color() {}
			color(floats4 value) : value(value) {}
			floats4 value = { 1, 1, 1, 1 };
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
			lifetime() : value(0), max_value(0) {}
			lifetime(float seconds) : value(seconds), max_value(seconds) {}
			
			float operator()() {
				return value / max_value;
			}
			
			float value;
			float max_value;
		};

		struct particle_info {
			value_segment<vec4<float>> color;
			value_segment<float> radius;
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
				return velocity + angularVelocity * relative_point.length() * vec3<float>(-relative_point.y, +relative_point.x, 0).normalize();
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
		struct ignore_gravity {};

		struct translucent {}; // tag for partially see-through objects. graphics needs to know.
		struct frustum_culled {}; // object is not visible due to frustum culling.

		struct frame_collisions {
			// values used to index a collision detection structure to access the actual data.
			std::vector<uint32_t> collision_indices;
		};

		struct light_omni {
			// fourth value in color encodes light intensity.
			floats4 color;
			
			// light strength at point is clamp(0, 1, object normal dot light direction + ambient) / sqr(dist)
			// when ambient value = 0, the backside of objects are not illuminated.
			// when ambient value = 1, objects are evenly illuminated.
			float ambient = 0;
		};

		struct light_directed : public light_omni {
			vec3f direction;
			float angle = math::PI_float;
			float edge_softness = 0.1f;
		};

		struct rope {
			rynx::ecs::id id_a;
			rynx::ecs::id id_b;

			vec3<float> point_a;
			vec3<float> point_b;

			float length;
			float strength;

			float cumulative_stress = 0;
		};

		struct dead {}; // mark entity for cleanup.

		struct collision_category {
			collision_category() : value(-1) {}
			collision_category(rynx::collision_detection::category_id category) : value(category) {}
			rynx::collision_detection::category_id value;
		};

		struct mesh {
			Mesh* m;
		};

		// TODO: Remove.
		struct texture {
			std::string tex;
		};
	}
}
