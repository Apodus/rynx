
#pragma once

#include <rynx/math/vector.hpp>
#include <utility>

namespace rynx {
	class plane {
	public:
		plane() = default;

		void set_coefficients(float a, float b, float c, float d);

		float distance(vec3f p) const;
		bool point_right_of_plane(vec3f p) const;
		bool sphere_right_of_plane(vec3f p, float radius) const;
		bool sphere_left_of_plane(vec3f p, float radius) const;
		bool sphere_not_left_of_plane(vec3f p, float radius) const;

		std::pair<vec3f, bool> collision_point(vec3f ray_origin, vec3f ray_direction) const;

	private:
		vec3<float> normal;
		float d = 0;
	};
}