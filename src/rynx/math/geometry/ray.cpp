
#include <rynx/math/geometry/ray.hpp>
#include <rynx/math/geometry/plane.hpp>
#include <rynx/math/geometry/sphere.hpp>

rynx::ray::ray(vec3f origin, vec3f direction) : m_origin(origin), m_direction(direction) {}

rynx::vec3f rynx::ray::origin() const {
	return m_origin;
}

rynx::vec3f rynx::ray::direction() const {
	return m_direction;
}

std::pair<rynx::vec3f, bool> rynx::ray::intersect(const rynx::plane& p) const {
    return p.intersect(*this);
}

std::pair<rynx::vec3f, bool> rynx::ray::intersect(const rynx::sphere& s) const {
    vec3 relative_origin = m_origin - s.origin;
    float a = m_direction.length_squared();
    float b = 2.0f * relative_origin.dot(m_direction);
    float c = relative_origin.length_squared() - s.radius * s.radius;
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0) {
        return { vec3f(), false };
    }
    else {
        return { m_origin + m_direction * (-b - sqrt(discriminant)) / (2.0f * a), true };
    }
}
