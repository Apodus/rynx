
#include <rynx/math/geometry/plane.hpp>

void rynx::plane::set_coefficients(float a, float b, float c, float d_) {
	normal.set(a,b,c);
	float l = 1.0f / normal.length();
	normal *= l;
	d = d_ * l;
}

bool rynx::plane::point_right_of_plane(vec3f p) const {
	return distance(p) > 0;
}

bool rynx::plane::sphere_right_of_plane(vec3f p, float radius) const {
	return distance(p) > radius;
}

bool rynx::plane::sphere_left_of_plane(vec3f p, float radius) const {
	return distance(p) < -radius;
}

bool rynx::plane::sphere_not_left_of_plane(vec3f p, float radius) const {
	return distance(p) > -radius;
}

float rynx::plane::distance(vec3<float> p) const {
	return (d + normal.dot(p));
}

std::pair<rynx::vec3f, bool> rynx::plane::collision_point(vec3f ray_origin, vec3f ray_direction) const {
	vec3f plane_origin = normal * d;
	vec3f origin_delta = plane_origin - ray_origin;
	float co_direction = ray_direction.dot(normal);
	if (co_direction > 1e-7f) {
		float t = origin_delta.dot(normal) / co_direction;
		return { ray_direction * t + ray_origin, true };
	}
	return { vec3f(), false };
}
