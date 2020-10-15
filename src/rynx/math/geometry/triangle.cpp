
#include <rynx/math/geometry/triangle.hpp>

rynx::vec3f rynx::triangle::centre_of_mass() const {
	return (a + b + c) / 3.0f;
};

float rynx::triangle::area() const {
	rynx::vec3f a2b = (b - a);
	rynx::vec3f a2c = (c - a);

	auto n = a2b.normal2d();
	n.normalize();
	float h = a2c.dot(n);
	float w = a2b.length();
	return h * w * 0.5f;
};

bool rynx::triangle::point_in_triangle(rynx::vec3f p) const {
	return rynx::math::pointInTriangle(p, a, b, c);
}



bool rynx::triangles::point_in_polygon(rynx::vec3f point) const {
	for (const auto& t : tris)
		if (t.point_in_triangle(point))
			return true;
	return false;
};

rynx::vec3f rynx::triangles::centre_of_mass() const {
	rynx::vec3f cumulative_pos;
	float weight = 0.0f;
	for (auto triangle : tris) {
		float area = triangle.area();
		rynx::vec3f tri_cm = triangle.centre_of_mass();

		cumulative_pos += tri_cm * area;
		weight += area;
	}
	return cumulative_pos / weight;
}