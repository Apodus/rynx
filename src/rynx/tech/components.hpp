#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/math/math.hpp>
#include <rynx/math/geometry/polygon.hpp>

#include <rynx/math/geometry/polygon_triangulation.hpp>
#include <rynx/math/geometry/triangle.hpp>

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
			physical_body(uint64_t collision_id = 0)
				: inv_mass(1.0f)
				, inv_moment_of_inertia(1.0f)
				, collision_elasticity(1.0f)
				, friction_multiplier(1.0f)
				, collision_id(collision_id)
			{}

			physical_body& bias(float value) {
				rynx_assert(value > 0, "bias must be positive value");
				bias_multiply = value;
				return *this;
			}

			// TODO: move implementations to cpp
			physical_body& mass(float m) {
				inv_mass = 1.0f / m;
				return *this;
			}

			physical_body& elasticity(float e) {
				collision_elasticity = e;
				return *this;
			}

			physical_body& friction(float frict) {
				friction_multiplier = frict;
				return *this;
			}

			physical_body& moment_of_inertia(float momentOfInertia) {
				inv_moment_of_inertia = 1.0f / momentOfInertia;
				return *this;
			}

			physical_body& moment_of_inertia(rynx::polygon& p, float scale = 1.0f) {
				rynx_assert(scale > 0.0f, "scale must be positive non-zero value");
				auto ext = p.extents();
				rynx_assert(ext.z.second - ext.z.first == 0, "assuming 2d xy-plane polys");
				float initial_scale = (ext.x.second - ext.x.first) * (ext.y.second - ext.y.first) / 10000.0f;

				auto tris = rynx::polygon_triangulation().make_triangles(p);
				auto centre_of_mass = tris.centre_of_mass();
				
				while (true) {
					std::vector<rynx::vec3f> points_in_p;

					for (float x = ext.x.first; x <= ext.x.second; x += initial_scale) {
						for (float y = ext.y.first; y <= ext.y.second; y += initial_scale) {
							if (tris.point_in_polygon({ x, y, 0 })) {
								points_in_p.emplace_back(rynx::vec3f(x, y, 0));
							}
						}
					}

					float obj_mass = 1.0f / inv_mass;
					float cumulative_inertia = 0.0f;

					if (points_in_p.size() > 10000) {
						for (auto point : points_in_p) {
							float point_mass = obj_mass / points_in_p.size();
							float point_radius = (point - centre_of_mass).length();
							cumulative_inertia += point_mass * point_radius * point_radius;
						}

						inv_moment_of_inertia = 1.0f / (cumulative_inertia * scale);
						return *this;
					}

					initial_scale *= 0.5f;
				}
			}

			float bias_multiply = 1.0f; // how strongly this body rejects other bodies. static terrain should have higher value than your basic dynamic object.
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

			boundary(boundary&& other) = default;
			boundary& operator=(boundary&& other) = default;

			boundary(const boundary& other) = delete;
			boundary& operator=(const boundary& other) = delete;

			void update_world_positions(vec3f pos, float angle) {
				float sin_v = math::sin(angle);
				float cos_v = math::cos(angle);
				const size_t num_segments = segments_local.size();
				rynx_assert(segments_local.size() == segments_world.size(), "boundary not initialized");
				for (size_t i = 0; i < num_segments; ++i) {
					segments_world[i].p1 = math::rotatedXY(segments_local[i].p1, sin_v, cos_v) + pos;
					segments_world[i].p2 = math::rotatedXY(segments_local[i].p2, sin_v, cos_v) + pos;
					segments_world[i].normal = math::rotatedXY(segments_local[i].normal, sin_v, cos_v);
				}
				rynx_assert(segments_local.size() == num_segments, "boundary edited during update");
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
			rynx::vec3f direction;
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