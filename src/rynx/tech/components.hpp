#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/math/math.hpp>
#include <rynx/math/geometry/polygon.hpp>

namespace rynx {
	namespace components {
		struct position {
			position() = default;
			position(vec3<float> pos, float angle = 0) : value(pos), angle(angle) {}
			vec3<float> value;
			float angle = 0;
		};

		struct position_relative {
			uint64_t host; // TODO: explicit entity id type.
			vec3f relative_pos;
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

			// 0 -> 1
			float linear() const {
				return 1.0f - value / max_value;
			}

			// 1 -> 0
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
			float linearDampening = 0.0f;
			float angularDampening = 0.0f;
		};

		struct constant_force {
			vec3f force;
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

			vec3<float> velocity_at_point_predict(vec3<float> relative_point, float dt) const {
				// return velocity + angularVelocity * relative_point.length() * vec3<float>(-relative_point.y, +relative_point.x, 0).normalize();
				return velocity + acceleration * dt + (angularVelocity + dt * angularAcceleration) * vec3<float>(-relative_point.y, +relative_point.x, 0);
			}
		};

		struct physical_body {
			physical_body() = default;
			physical_body(float mass, float moment_of_inertia, float elasticity, float friction, uint64_t collision_id = 0)
				: inv_mass(1.0f / mass)
				, inv_moment_of_inertia(1.0f / moment_of_inertia)
				, collision_elasticity(elasticity)
				, friction_multiplier(friction)
				, collision_id(collision_id)
			{}

			float inv_mass = 1.0f;
			float inv_moment_of_inertia = 1.0f;
			float collision_elasticity = 0.5f; // [0, 1[
			float friction_multiplier = 1.0f; // [0, 1]
			uint64_t collision_id = 0; // if two colliding objects have the same collision id (!= 0) then the collision is ignored.
		};

		struct boundary {
			using boundary_t = decltype(rynx::polygon().generateBoundary_Outside(1.0f));
			boundary(boundary_t&& b, vec3f pos = vec3f(), float angle = 0.0f) : segments_local(std::move(b)) {
				segments_world.resize(segments_local.size());
				update_world_positions(pos, angle);
			}

			void update_world_positions(vec3f pos, float angle) {
				float sin_v = math::sin(angle);
				float cos_v = math::cos(angle);
				const size_t num_segments = segments_local.size();
				for (size_t i = 0; i < num_segments; ++i) {
					segments_world[i].p1 = math::rotatedXY(segments_local[i].p1, sin_v, cos_v) + pos;
					segments_world[i].p2 = math::rotatedXY(segments_local[i].p2, sin_v, cos_v) + pos;
					segments_world[i].normal = math::rotatedXY(segments_local[i].normal, sin_v, cos_v);
				}
			}

			boundary_t segments_local;
			boundary_t segments_world;
		};

		struct draw_always {};
		struct projectile {}; // tag for fast moving items in collision detection.
		struct ignore_gravity {};
		struct dead {}; // mark entity for cleanup.

		struct light_omni {
			// fourth value in color encodes light intensity.
			floats4 color;

			// light strength at point is clamp(0, 1, object normal dot light direction + ambient) / sqr(dist)
			// when ambient value = 0, the backside of objects are not illuminated.
			// when ambient value = 1, objects are evenly illuminated.
			float ambient = 0;

			float attenuation_linear = 0.0f; // no linear attenuation.
			float attenuation_quadratic = 1.0f; // 100% quadratic attenuation.
		};

		struct light_directed : public light_omni {
			vec3f direction;
			float angle = math::pi;
			float edge_softness = 0.1f;
		};

		struct collisions {
			int category = 0;
		};

		struct collision_custom_reaction {
			struct event {
				rynx::ecs::id id; // collided with what
				rynx::components::physical_body body;
				vec3f relative_velocity;
				vec3f normal;
			};
			std::vector<event> events;
		};
	}
}