
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
		
		struct boundary {
			using boundary_t = decltype(Polygon<vec3<float>>().generateBoundary_Outside());
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

		struct mesh {
			Mesh* m;
		};
	}
}
