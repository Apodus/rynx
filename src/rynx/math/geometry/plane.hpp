
#pragma once

#include <rynx/math/vector.hpp>
#include <utility>

namespace rynx {
	class ray;
	class MathDLL plane {
	public:
		plane() = default;
		plane(float a, float b, float c, float d) { set_coefficients(a, b, c, d); }

		vec3f normal() const;
		float offset() const;

		void set_coefficients(float a, float b, float c, float d);
		
		float distance(vec3f p) const;
		bool point_right_of_plane(vec3f p) const;
		bool sphere_right_of_plane(vec3f p, float radius) const;
		bool sphere_left_of_plane(vec3f p, float radius) const;
		bool sphere_not_left_of_plane(vec3f p, float radius) const;

		std::pair<vec3f, bool> intersect(const rynx::ray&) const;

	private:
		vec3<float> m_normal;
		float d = 0;
	};
}