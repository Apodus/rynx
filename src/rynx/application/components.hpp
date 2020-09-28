
#pragma once

#include <rynx/tech/ecs.hpp>
#include <rynx/tech/object_storage.hpp>
#include <rynx/math/geometry/polygon.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/math/random.hpp>
#include <rynx/tech/collision_detection.hpp>
#include <rynx/tech/components.hpp>

#include <string>

namespace rynx {
	class mesh;

	namespace components {
		
		struct particle_emitter {
			rynx::math::value_range<rynx::floats4> start_color;
			rynx::math::value_range<rynx::floats4> end_color;
			rynx::math::value_range<float> start_radius;
			rynx::math::value_range<float> end_radius;

			rynx::math::value_range<float> lifetime_range;
			rynx::math::value_range<float> linear_dampening;

			rynx::math::value_range<float> initial_velocity;
			rynx::math::value_range<float> initial_angle;
			rynx::math::value_range<rynx::vec3f> constant_force;

			rynx::math::value_range<float> spawn_rate;

			bool rotate_with_host = false;
			vec3f position_offset;

			mutable float time_until_next_spawn = 0;
			mutable rynx::math::rand64 m_random;

			rynx::floats4 get_start_color() const { return start_color(m_random()); }
			rynx::floats4 get_end_color() const { return end_color(m_random()); }
			float get_start_radius() const { return start_radius(m_random()); }
			float get_end_radius() const { return end_radius(m_random()); }
			float get_lifetime() const { return lifetime_range(m_random()); }
			float get_linear_damping() const { return linear_dampening(m_random()); }
			float get_initial_velocity() const { return initial_velocity(m_random()); }
			float get_initial_angle() const { return initial_angle(m_random()); }
			rynx::vec3f get_constant_force() const { return constant_force(m_random()); }
			rynx::math::rand64& get_random() const { return m_random; }

			struct editor {
				particle_emitter& host;

				editor& color_ranges(rynx::math::value_range<rynx::floats4> start_color_, rynx::math::value_range<rynx::floats4> end_color_) {
					host.start_color = start_color_;
					host.end_color = end_color_;
					return *this;
				}

				editor& radius_ranges(rynx::math::value_range<float> start_radius_, rynx::math::value_range<float> end_radius_) {
					host.start_radius = start_radius_;
					host.end_radius = end_radius_;
					return *this;
				}

				editor& spawn_rate_range(rynx::math::value_range<float> spawn_rate_) {
					host.spawn_rate = spawn_rate_;
					return *this;
				}

				editor& linear_dampening_range(rynx::math::value_range<float> damp) {
					host.linear_dampening = damp;
					return *this;
				}

				editor& lifetime_range(rynx::math::value_range<float> lifetime) {
					host.lifetime_range = lifetime;
					return *this;
				}

				editor& initial_velocity_range(rynx::math::value_range<float> initial_vel) {
					host.initial_velocity = initial_vel;
					return *this;
				}

				editor& initial_angle_range(rynx::math::value_range<float> initial_angle_) {
					host.initial_angle = initial_angle_;
					return *this;
				}

				editor& constant_force_range(rynx::math::value_range<rynx::vec3f> constant_force_) {
					host.constant_force = constant_force_;
					return *this;
				}

				editor& rotate_with_host(bool value) {
					host.rotate_with_host = value;
					return *this;
				}

				editor& position_offset(vec3f offset) {
					host.position_offset = offset;
					return *this;
				}
			};

			editor edit() { return editor{*this}; }

			int get_spawn_count(float dt) const {
				int count = 0;
				time_until_next_spawn -= dt;
				while (time_until_next_spawn <= 0.0f) {
					++count;
					time_until_next_spawn += 1.0f / spawn_rate(m_random());
				}
				return count;
			}
		};

		struct particle_info {
			rynx::math::value_range<floats4> color_range;
			rynx::math::value_range<float> radius_range;
		};

		struct translucent {}; // tag for partially see-through objects. graphics needs to know.
		struct frustum_culled {}; // object is not visible due to frustum culling.
		struct invisible {}; // tag to prevent rendering of object.

		namespace phys {
			struct joint {
				
				// what kind of elastic behaviour we want from the connection type.
				// rod with length zero gives the "just a joint" behaviour.
				enum class connector_type {
					RubberBand,
					Spring,
					Rod,
				};

				// allow rotation relative to connector or not
				enum class joint_type {
					Fixed,
					Free
				};

				joint& connect_with_rod() {
					connector = connector_type::Rod;
					return *this;
				}

				joint& connect_with_rubberband() {
					connector = connector_type::RubberBand;
					return *this;
				}

				joint& connect_with_spring() {
					connector = connector_type::Spring;
					return *this;
				}

				joint& rotation_free() {
					m_joint = joint_type::Free;
					return *this;
				}

				joint& rotation_fixed() {
					m_joint = joint_type::Fixed;
					return *this;
				}

				rynx::ecs::id id_a;
				rynx::ecs::id id_b;

				vec3<float> point_a;
				vec3<float> point_b;

				float length;
				float strength;
				float softness = 1.0f;
				float response_time = 0.016f;

				float cumulative_stress = 0;

				connector_type connector;
				joint_type m_joint;
			};

			inline float compute_current_joint_length(const joint& rope, rynx::ecs::view<const rynx::components::position> ecs) {
				auto entity_a = ecs[rope.id_a];
				auto entity_b = ecs[rope.id_b];

				auto pos_a = entity_a.get<const components::position>();
				auto pos_b = entity_b.get<const components::position>();

				auto relative_pos_a = math::rotatedXY(rope.point_a, pos_a.angle);
				auto relative_pos_b = math::rotatedXY(rope.point_b, pos_b.angle);
				auto world_pos_a = pos_a.value + relative_pos_a;
				auto world_pos_b = pos_b.value + relative_pos_b;
				return (world_pos_a - world_pos_b).length();
			}
		}

		struct mesh : public rynx::ecs::value_segregated_component {
			mesh() = default;
			mesh(rynx::mesh* p) : m(p) {}
			size_t hash() const { return size_t(uintptr_t(m)); }
			bool operator == (const mesh& other) const { return m == other.m; }
			rynx::mesh* m;
		};
	}
}
