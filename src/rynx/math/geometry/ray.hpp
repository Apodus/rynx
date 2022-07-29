#pragma once

#include <rynx/math/vector.hpp>
#include <utility>

namespace rynx {
	class plane;
	class sphere;
	class MathDLL ray {
		vec3f m_origin;
		vec3f m_direction;

	public:
		ray(vec3f origin, vec3f direction);
		vec3f origin() const;
		vec3f direction() const;

		std::pair<vec3f, bool> intersect(const rynx::plane&) const;
		std::pair<vec3f, bool> intersect(const rynx::sphere&) const;
	};
}