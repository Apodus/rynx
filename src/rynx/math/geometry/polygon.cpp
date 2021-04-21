
#include <rynx/math/geometry/polygon.hpp>
#include <rynx/math/geometry/polygon_editor.hpp>

#include <rynx/math/spline.hpp>
#include <rynx/math/geometry/bounding_sphere.hpp>

float rynx::polygon::max_component_value() const {
	float max_d = 0;
	for (auto vertice : m_vertices) {
		rynx::vec3f p = vertice * vertice;
		if (p.x > max_d) {
			max_d = p.x;
		}
		if (p.y> max_d) {
			max_d = p.y;
		}
		if (p.z > max_d) {
			max_d = p.z;
		}
	}
	return sqrt(max_d);
}

float rynx::polygon::radius() const {
	float max_d = 0;
	for (auto vertice : m_vertices) {
		float distance_sqr = vertice.length_squared();
		if (distance_sqr > max_d) {
			max_d = distance_sqr;
		}
	}
	return sqrt(max_d);
}

rynx::vec3<std::pair<float, float>> rynx::polygon::extents() const {
	vec3f max_v;
	vec3f min_v;

	for (auto vertice : m_vertices) {
		rynx::vec3f p = vertice;
		if (p.x > max_v.x) max_v.x = p.x;
		if (p.x < min_v.x) min_v.x = p.x;

		if (p.y > max_v.y) max_v.y = p.y;
		if (p.y < min_v.y) min_v.y = p.y;

		if (p.z > max_v.z) max_v.z = p.z;
		if (p.z < min_v.z) min_v.z = p.z;
	}
	return { {min_v.x, max_v.x}, {min_v.y, max_v.y}, {min_v.z, max_v.z} };
}

std::vector<rynx::vec3f> rynx::polygon::as_vertex_vector() const {
	return m_vertices.as_vector();
}

rynx::math::spline rynx::polygon::as_spline(float alpha) const {
	auto vertices = m_vertices.as_vector();
	vertices.pop_back();
	return rynx::math::spline(std::move(vertices), alpha);
}

float rynx::polygon::normalize() {
	float r = radius();
	scale(1.0f / r);
	return r;
}

rynx::polygon& rynx::polygon::scale(float s) {
	for (auto& vertice : m_vertices) {
		vertice *= s;
	}
	return *this;
}

rynx::polygon& rynx::polygon::scale(vec3f ranges) {
	for (auto& vertice : m_vertices) {
		vertice *= ranges;
	}
	recompute_normals();
	return *this;
}

std::pair<rynx::vec3f, float> rynx::polygon::bounding_sphere() const {
	return rynx::math::bounding_sphere(m_vertices.as_vector()); // todo, no conversion to vector should be required
}

rynx::polygon_editor rynx::polygon::edit() {
	return polygon_editor(*this);
}

bool rynx::polygon::isConvex(int vertex) const {
	rynx::vec3<float> left;
	rynx::vec3<float> right;
	if (vertex == 0)
		left = m_vertices.back();
	else
		left = m_vertices[vertex - 1];

	left -= m_vertices[vertex];

	if (vertex == m_vertices.size() - 1)
		right = m_vertices.front();
	else
		right = m_vertices[vertex + 1];
	right -= m_vertices[vertex];
	return left.cross2d(right) < 0.0f;
}