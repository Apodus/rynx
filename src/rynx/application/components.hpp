
#pragma once

#include <rynx/ecs/ecs.hpp>
#include <rynx/math/geometry/polygon.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/math/random.hpp>
#include <rynx/system/annotate.hpp>
#include <rynx/graphics/mesh/id.hpp>
#include <rynx/tech/components.hpp>

namespace rynx {
	namespace graphics {
		class mesh;
	}

	namespace components {
		namespace graphics {
			struct particle_emitter {
				rynx::math::value_range<rynx::floats4> ANNOTATE("range 0 1") start_color = { rynx::floats4{1, 1, 1, 1}, rynx::floats4{1, 1, 1, 1} };
				rynx::math::value_range<rynx::floats4> ANNOTATE("range 0 1") end_color;
				rynx::math::value_range<float> ANNOTATE(">=0") start_radius;
				rynx::math::value_range<float> ANNOTATE(">=0") end_radius;

				rynx::math::value_range<float> ANNOTATE(">=0") lifetime_range;
				rynx::math::value_range<float> ANNOTATE(">=0") linear_dampening;

				rynx::math::value_range<float> ANNOTATE(">=0") initial_velocity;
				rynx::math::value_range<float> initial_angle;
				rynx::math::value_range<rynx::vec3f> constant_force;

				rynx::math::value_range<float> ANNOTATE(">=0") spawn_rate = { 1.0f, 1.0f };

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

				struct ANNOTATE("hidden") ANNOTATE("transient") editor : public ecs_no_serialize_tag {
				private:
					particle_emitter* host = nullptr;

				public:
					editor() { rynx_assert(false, "you should not instantiate this"); };
					editor(particle_emitter & host) : host(&host) {}

					editor& color_ranges(rynx::math::value_range<rynx::floats4> start_color_, rynx::math::value_range<rynx::floats4> end_color_) {
						host->start_color = start_color_;
						host->end_color = end_color_;
						return *this;
					}

					editor& radius_ranges(rynx::math::value_range<float> start_radius_, rynx::math::value_range<float> end_radius_) {
						host->start_radius = start_radius_;
						host->end_radius = end_radius_;
						return *this;
					}

					editor& spawn_rate_range(rynx::math::value_range<float> spawn_rate_) {
						host->spawn_rate = spawn_rate_;
						return *this;
					}

					editor& linear_dampening_range(rynx::math::value_range<float> damp) {
						host->linear_dampening = damp;
						return *this;
					}

					editor& lifetime_range(rynx::math::value_range<float> lifetime) {
						host->lifetime_range = lifetime;
						return *this;
					}

					editor& initial_velocity_range(rynx::math::value_range<float> initial_vel) {
						host->initial_velocity = initial_vel;
						return *this;
					}

					editor& initial_angle_range(rynx::math::value_range<float> initial_angle_) {
						host->initial_angle = initial_angle_;
						return *this;
					}

					editor& constant_force_range(rynx::math::value_range<rynx::vec3f> constant_force_) {
						host->constant_force = constant_force_;
						return *this;
					}

					editor& rotate_with_host(bool value) {
						host->rotate_with_host = value;
						return *this;
					}

					editor& position_offset(vec3f offset) {
						host->position_offset = offset;
						return *this;
					}
				};

				editor edit() { return editor{ *this }; }

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

			struct ANNOTATE("hidden") particle_info {
				rynx::math::value_range<floats4> ANNOTATE("range 0 1") color_range;
				rynx::math::value_range<float> ANNOTATE(">=0") radius_range;
			};

			struct mesh : public rynx::ecs_value_segregated_component_tag {
				mesh() = default;
				mesh(rynx::graphics::mesh_id p) : m(p) {}
				size_t hash() const { return m.value; }
				bool operator == (const mesh& other) const { return m == other.m; }
				rynx::graphics::mesh_id m;
			};

			struct texture {
				rynx::string textureName;
			};

			struct translucent {}; // tag for partially see-through objects. graphics needs to know.
			struct ANNOTATE("hidden") frustum_culled : public rynx::ecs_no_serialize_tag {}; // object is not visible due to frustum culling.
			struct invisible {}; // tag to prevent rendering of object.
		}

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

				rynx::components::transform::position_relative a;
				rynx::components::transform::position_relative b;

				float ANNOTATE(">=0") length = 0.0f;
				float ANNOTATE(">=0") strength = 1.0f;
				float ANNOTATE(">=0") softness = 1.0f;
				float ANNOTATE(">=0") response_time = 0.016f;

				float cumulative_stress = 0;

				connector_type connector;
				joint_type m_joint;
			};

			inline float compute_current_joint_length(const joint& rope, rynx::ecs::view<const rynx::components::transform::position> ecs) {
				if (!(ecs.exists(rope.a.id) & ecs.exists(rope.b.id)))
					return 0;

				auto entity_a = ecs[rope.a.id];
				auto entity_b = ecs[rope.b.id];

				auto pos_a = entity_a.get<const components::transform::position>();
				auto pos_b = entity_b.get<const components::transform::position>();

				auto relative_pos_a = math::rotatedXY(rope.a.pos, pos_a.angle);
				auto relative_pos_b = math::rotatedXY(rope.b.pos, pos_b.angle);
				auto world_pos_a = pos_a.value + relative_pos_a;
				auto world_pos_b = pos_b.value + relative_pos_b;
				return (world_pos_a - world_pos_b).length();
			}
		}
	}

	namespace serialization {
		template<> struct Serialize<rynx::components::graphics::particle_emitter::editor> {
			template<typename IOStream> void serialize(const rynx::components::graphics::particle_emitter::editor&, IOStream&) {}
			template<typename IOStream> void deserialize(rynx::components::graphics::particle_emitter::editor&, IOStream&) {}
		};
	}
}


#ifndef RYNX_CODEGEN
#include <rynx/application/components_reflection.hpp>
#include <rynx/application/components_serialization.hpp>
#endif