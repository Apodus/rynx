
#include <rynx/math/geometry/sphere.hpp>

rynx::sphere::sphere(vec3f origin, float radius) : pos(origin), radius(radius) {}

bool rynx::sphere::intersect(sphere other) const {
	float combined_radius = radius + other.radius;
	return (pos - other.pos).length_squared() < combined_radius * combined_radius;
}
