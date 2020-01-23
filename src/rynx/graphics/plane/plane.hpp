
#pragma once

#include <rynx/tech/math/vector.hpp>

class plane
{
public:
	plane() = default;

	void set_coefficients(float a, float b, float c, float d);

	float distance(vec3f p) const;
	bool point_right_of_plane(vec3f p) const;
	bool sphere_right_of_plane(vec3f p, float radius) const;
	bool sphere_left_of_plane(vec3f p, float radius) const;
	bool sphere_not_left_of_plane(vec3f p, float radius) const;

private:
	vec3<float> normal;
	float d;
};
