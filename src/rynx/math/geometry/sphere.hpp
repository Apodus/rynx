#pragma once

#include <rynx/math/vector.hpp>

namespace rynx {
	class sphere {
	public:
		vec3f pos;
		float radius;
		sphere(vec3f origin, float radius);
		bool intersect(sphere other) const;
	};
}