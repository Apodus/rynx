
#include <rynx/math/geometry/sphere.hpp>

rynx::sphere::sphere(vec3f origin, float radius) : origin(origin), radius(radius) {}

bool rynx::sphere::intersect(sphere other) const {
	float combined_radius = radius + other.radius;
	return (origin - other.origin).length_squared() < combined_radius * combined_radius;
}
