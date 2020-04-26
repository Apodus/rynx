
#include <rynx/math/geometry/plane.hpp>
#include <rynx/math/geometry/ray.hpp>

rynx::vec3f rynx::plane::normal() const {
	return m_normal;
}

float rynx::plane::offset() const {
	return d;
}

void rynx::plane::set_coefficients(float a, float b, float c, float d_) {
	m_normal.set(a,b,c);
	float l = 1.0f / m_normal.length();
	m_normal *= l;
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
	return (d + m_normal.dot(p));
}

std::pair<rynx::vec3f, bool> rynx::plane::intersect(const rynx::ray& r) const {
	vec3f plane_origin = m_normal * d;
	vec3f origin_delta = plane_origin - r.origin();
	float co_direction = r.direction().dot(m_normal);
	if (co_direction < -1e-7f) {
		float t = origin_delta.dot(m_normal) / co_direction;
		return { r.direction() * t + r.origin(), true };
	}
	return { vec3f(), false };
}
