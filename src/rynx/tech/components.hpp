#pragma once

#include <rynx/tech/math/vector.hpp>

namespace rynx {
	namespace components {
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

		struct color {
			color() {}
			color(floats4 value) : value(value) {}
			floats4 value = { 1, 1, 1, 1 };
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

		struct collisions {
			int category = 0;
		};
	}
}