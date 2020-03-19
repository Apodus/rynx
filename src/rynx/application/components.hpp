
#pragma once

#include <rynx/tech/ecs.hpp>
#include <rynx/tech/object_storage.hpp>
#include <rynx/graphics/mesh/polygon.hpp>
#include <rynx/tech/math/vector.hpp>
#include <rynx/tech/collision_detection.hpp>
#include <rynx/tech/components.hpp>

#include <string>

class Mesh;

namespace rynx {
	template<typename T> struct value_segment {
		T begin;
		T end;

		T linear(float v) const { return begin * v + end * (1.0f - v); }
	};

	namespace components {
		

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

		/*
		struct collision_category {
			collision_category() : value(-1) {}
			collision_category(rynx::collision_detection::category_id category) : value(category) {}
			rynx::collision_detection::category_id value;
		};
		*/

		struct mesh {
			Mesh* m;
		};

		// TODO: Remove.
		struct texture {
			std::string tex;
		};
	}
}
