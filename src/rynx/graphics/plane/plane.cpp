
#include <rynx/graphics/plane/plane.hpp>

void plane::set_coefficients(float a, float b, float c, float d_)
{
	normal.set(a,b,c);
	float l = 1.0f / normal.length();
	normal *= l;
	d = d_ * l;
}

bool plane::point_right_of_plane(vec3f p) const
{
	return distance(p) > 0;
}

bool plane::sphere_right_of_plane(vec3f p, float radius) const
{
	return distance(p) > radius;
}

bool plane::sphere_left_of_plane(vec3f p, float radius) const
{
	return distance(p) < -radius;
}

bool plane::sphere_not_left_of_plane(vec3f p, float radius) const
{
	return distance(p) > -radius;
}

float plane::distance(vec3<float> p) const
{
	return (d + normal.dot(p));
}

