#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/math/math.hpp>

namespace rynx {
	namespace components {
		struct position {
			position() = default;
			position(vec3<float> pos, float angle = 0) : value(pos), angle(angle) {}
			vec3<float> value;
			float angle;
		};

		struct scale {
			operator float() const {
				return value;
			}

			scale(float v) : value(v) {}
			float value;
		};

		struct radius {
			radius() = default;
			radius(float r) : r(r) {}
			float r = 0;
		};

		struct color {
			color() {}
			color(floats4 value) : value(value) {}
			floats4 value = { 1, 1, 1, 1 };
		};

		struct lifetime {
			lifetime() : value(0), max_value(0) {}
			lifetime(float seconds) : value(seconds), max_value(seconds) {}

			operator float() const {
				return linear();
			}

			float operator()() const {
				return linear();
			}

			float linear() const {
				return value / max_value;
			}

			float linear_inv() const {
				return 1.0f - linear();
			}

			float quadratic() const {
				float f = linear();
				return f * f;
			}

			float quadratic_inv() const {
				return 1.0f - quadratic();
			}

			float inv_quadratic() const {
				float f = linear_inv();
				return f * f;
			}

			// square of the inverted linear, inverted. gives similar round shape as quadratic.
			// just goes from 0 to 1 with a steep rise and slows down towards the end.
			float inv_quadratic_inv() const {
				float f = linear_inv();
				return 1.0f - f * f;
			}

			float value;
			float max_value;
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
				// return velocity + angularVelocity * relative_point.length() * vec3<float>(-relative_point.y, +relative_point.x, 0).normalize();
				return velocity + angularVelocity * vec3<float>(-relative_point.y, +relative_point.x, 0);
			}
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
			uint64_t collision_id = 0; // if two colliding objects have the same collision id (!= 0) then the collision is ignored.
		};

		struct projectile {}; // tag for fast moving items in collision detection.
		struct ignore_gravity {};


		struct light_omni {
			// fourth value in color encodes light intensity.
			floats4 color;

			// light strength at point is clamp(0, 1, object normal dot light direction + ambient) / sqr(dist)
			// when ambient value = 0, the backside of objects are not illuminated.
			// when ambient value = 1, objects are evenly illuminated.
			float ambient = 0;

			float attenuation_linear = 10000.0f; // linear attenuation component fades out immediately
			float attenuation_quadratic = 1.0f; // linear attenuation component fades out immediately
		};

		struct light_directed : public light_omni {
			vec3f direction;
			float angle = math::pi;
			float edge_softness = 0.1f;
		};

		struct collisions {
			int category = 0;
		};
	}
}