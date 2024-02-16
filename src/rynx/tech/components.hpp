#pragma once

#include <rynx/system/annotate.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/math/math.hpp>
#include <rynx/math/geometry/polygon.hpp>
#include <rynx/math/geometry/polygon_editor.hpp>
#include <rynx/math/geometry/triangle.hpp>
#include <rynx/ecs/id.hpp>
#include <rynx/ecs/scenes.hpp>

namespace rynx {
	namespace components {

		namespace logic {
			struct interval_generator {
				float time_until_next = 1.0f;
				rynx::scene_id target;
			};

			struct generate_on_start {
				rynx::scene_id target;
			};
		}

		namespace transform {
			struct position_relative {
				rynx::id id;
				rynx::vec3f pos;
			};

			struct dampening {
				float ANNOTATE("range 0 1") linearDampening = 0.0f;
				float ANNOTATE("range 0 1") angularDampening = 0.0f;
			};

			struct constant_force {
				rynx::vec3f force;
			};

			struct motion {
				motion() = default;
				motion(rynx::vec3<float> v, float av) : velocity(v), angularVelocity(av) {}

				rynx::vec3<float> velocity;
				float angularVelocity = 0;

				rynx::vec3<float> acceleration;
				float angularAcceleration = 0;

				rynx::vec3<float> velocity_at_point(rynx::vec3<float> relative_point) const {
					return velocity + angularVelocity * vec3<float>(-relative_point.y, +relative_point.x, 0);
				}

				rynx::vec3<float> velocity_at_point_predict(rynx::vec3<float> relative_point, float dt) const {
					return velocity + acceleration * dt + (angularVelocity + dt * angularAcceleration) * rynx::vec3<float>(-relative_point.y, +relative_point.x, 0);
				}
			};

			struct ignore_gravity {};
		}

		namespace entity {
			struct ANNOTATE("hidden") dead{}; // mark entity for cleanup.
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

				float ANNOTATE(">=0") value;
				float ANNOTATE(">=0") max_value;
			};
		}

		namespace phys {
			struct body {
				body(uint64_t collision_id = 0)
					: collision_id(collision_id)
				{}

				body& bias(float value) {
					rynx_assert(value > 0, "bias must be positive value");
					bias_multiply = value;
					return *this;
				}

				// TODO: move implementations to cpp
				body& mass(float m) {
					inv_mass = 1.0f / m;
					return *this;
				}

				body& elasticity(float e) {
					collision_elasticity = e;
					return *this;
				}

				body& friction(float frict) {
					friction_multiplier = frict;
					return *this;
				}

				body& moment_of_inertia(float momentOfInertia) {
					inv_moment_of_inertia = 1.0f / momentOfInertia;
					return *this;
				}
				
				float ANNOTATE(">=0") bias_multiply = 1.0f; // how strongly this body rejects other bodies. static terrain should have higher value than your basic dynamic object.
				float ANNOTATE(">=0") ANNOTATE("inverse") inv_mass = 0.100f; // 10kg default
				float ANNOTATE(">=0") ANNOTATE("inverse") inv_moment_of_inertia = 0.0002f; // Should be computed from mass + shape kgm^2 ?
				float ANNOTATE("range 0 1") collision_elasticity = 0.5f; // [0, 1[
				float ANNOTATE(">=0") friction_multiplier = 1.0f; // [0, 1]
				uint64_t collision_id = 0; // if two colliding objects have the same collision id (!= 0) then the collision is ignored.
			};

			struct boundary {
				boundary() {}
				boundary(rynx::polygon b, rynx::vec3f pos = rynx::vec3f(), float angle = 0.0f) : segments_local(std::move(b)) {
					segments_world = segments_local;
					update_world_positions(pos, angle);
				}

				boundary(boundary&& other) = default;
				boundary& operator=(boundary&& other) = default;

				// to support ecs cloning, copying components must be allowed.
				// boundary(const boundary& other) = delete;
				// boundary& operator=(const boundary& other) = delete;
				boundary(const boundary& other) = default;
				boundary& operator=(const boundary& other) = default;

				void update_world_positions(rynx::vec3f pos, float angle) {
					const float sin_v = math::sin(angle);
					const float cos_v = math::cos(angle);
					const size_t num_segments = segments_local.size();

					{
						auto editor = segments_world.edit();
						for (size_t i = 0; i < num_segments; ++i) {
							rynx::vec3f new_position = math::rotatedXY(segments_local.vertex_position(i), sin_v, cos_v) + pos;
							rynx::vec3f new_segment_normal = math::rotatedXY(segments_local.segment_normal(i), sin_v, cos_v);
							rynx::vec3f new_vertex_normal = math::rotatedXY(segments_local.vertex_normal(i), sin_v, cos_v);
							editor.vertex(int(i)).set(new_position, new_segment_normal, new_vertex_normal);
						}
					}

					rynx_assert(segments_local.size() == num_segments, "boundary edited during update");
				}

				rynx::polygon segments_local;
				rynx::polygon ANNOTATE("transient") segments_world;
			};

			struct collisions {
				int category = 0;
			};

			struct collision_events {
				struct ANNOTATE("hidden") event : public ecs_no_serialize_tag {
					bool operator ==(const event&) const { return true; }

					rynx::id id; // collided with what
					rynx::components::phys::body body;
					rynx::vec3f relative_velocity;
					rynx::vec3f normal;
				};

				std::vector<event> ANNOTATE("transient") events;
				bool operator ==(const collision_events&) const { return true; }
			};
		}

		namespace graphics {
			struct draw_always {};

			struct light_omni {
				// fourth value in color encodes light intensity.
				floats4
					ANNOTATE("rename x red")
					ANNOTATE("rename y green")
					ANNOTATE("rename z blue")
					ANNOTATE("rename w luminance")
					ANNOTATE("applies_to x y z")
					ANNOTATE("range 0 1")
					ANNOTATE("applies_to w")
					ANNOTATE(">=0") color;

				// light strength at point is clamp(0, 1, object normal dot light direction + ambient) / sqr(dist)
				// when ambient value = 0, the backside of objects are not illuminated.
				// when ambient value = 1, objects are evenly illuminated.
				float ANNOTATE("range 0 1") ambient = 0;

				float ANNOTATE(">=0") attenuation_linear = 0.0f; // no linear attenuation.
				float ANNOTATE(">=0") attenuation_quadratic = 1.0f; // 100% quadratic attenuation.
			};

			struct light_directed : public light_omni {
				rynx::vec3f ANNOTATE("len=1") direction;
				float ANNOTATE("range 0 6.28318530718") angle = math::pi;
				float ANNOTATE("range 0 1") edge_softness = 0.1f;
			};

			struct color {
				color() {}
				color(floats4 value) : value(value) {}
				floats4 ANNOTATE("range 0 1") value = { 1, 1, 1, 1 };
			};
		}

		struct projectile {}; // tag for fast moving items in collision detection.
	}
}

#include <rynx/tech/components_reflection.hpp>
#include <rynx/tech/components_serialization.hpp>