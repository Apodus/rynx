
#pragma once

#include <rynx/tech/ecs.hpp>
#include <rynx/tech/object_storage.hpp>
#include <rynx/math/geometry/polygon.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/tech/collision_detection.hpp>
#include <rynx/tech/components.hpp>

#include <string>

namespace rynx {
	class mesh;
	template<typename T> struct value_segment {
		T begin;
		T end;

		T linear(float v) const { return begin * v + end * (1.0f - v); }
	};

	namespace components {
		

		struct particle_info {
			value_segment<floats4> color;
			value_segment<float> radius;
		};
		
		struct boundary {
			using boundary_t = decltype(polygon().generateBoundary_Outside(1.0f));
			boundary(boundary_t&& b) : segments_local(std::move(b)) {
				segments_world.resize(segments_local.size());
			}
			boundary_t segments_local;
			boundary_t segments_world;
		};

		struct translucent {}; // tag for partially see-through objects. graphics needs to know.
		struct frustum_culled {}; // object is not visible due to frustum culling.

		struct frame_collisions {
			// values used to index a collision detection structure to access the actual data.
			std::vector<uint32_t> collision_indices;
		};

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

				float cumulative_stress = 0;

				connector_type connector;
				joint_type m_joint;
			};
		}

		struct dead {}; // mark entity for cleanup.

		struct mesh : public rynx::ecs::value_segregated_component {
			mesh() = default;
			mesh(rynx::mesh* p) : m(p) {}
			size_t hash() const { return size_t(uintptr_t(m)); }
			bool operator == (const mesh& other) const { return m == other.m; }
			rynx::mesh* m;
		};
	}
}
