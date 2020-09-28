
#include <rynx/math/geometry/polygon.hpp>

rynx::polygon::segment::segment() : line_segment(), convex1(true), convex2(true) {}
rynx::polygon::segment::segment(const line_segment& lineSegment) {
	static_cast<line_segment&>(*this) = lineSegment;
}

rynx::polygon::segment::segment(rynx::vec3<float> a, rynx::vec3<float> b, bool c1, bool c2) : line_segment(a, b) {
	convex1 = c1;
	convex2 = c2;
}

rynx::polygon::triangle::triangle(int a, int b, int c) {
	this->a = a;
	this->b = b;
	this->c = c;
}

rynx::vec3<float>& rynx::polygon::operator [](int index) {
	return vertices[index];
}

const rynx::vec3<float>& rynx::polygon::operator [](int index) const {
	return vertices[index];
}

float rynx::polygon::max_component_value() const {
	float max_d = 0;
	for (auto vertice : vertices) {
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
	for (auto vertice : vertices) {
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

	for (auto vertice : vertices) {
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

float rynx::polygon::normalize() {
	float r = max_component_value();
	scale(1.0f / r);
	return r;
}

void rynx::polygon::scale(float s) {
	for (auto& vertice : vertices) {
		vertice *= s;
	}
}

void rynx::polygon::scale(vec3f ranges) {
	for (auto& vertice : vertices) {
		vertice *= ranges;
	}
}

bool rynx::polygon::isConvex(int vertex) const {
	rynx::vec3<float> left;
	rynx::vec3<float> right;
	if (vertex == 0)
		left = vertices[vertices.size() - 1];
	else
		left = vertices[vertex - 1];

	left -= vertices[vertex];

	if (vertex == vertices.size() - 1)
		right = (*this)[0];
	else
		right = vertices[vertex + 1];
	right -= (*this)[vertex];
	return left.cross2d(right) < 0.0f;
}


std::vector<rynx::polygon::segment> rynx::polygon::generateBoundary_Outside(float scale) const {
	std::vector<segment> boundary;
	if (vertices.size() < 3) {
		return boundary;
	}

	for (size_t i = 0; i < vertices.size() - 1; ++i) {
		boundary.emplace_back(segment(vertices[i] * scale, vertices[i + 1] * scale, isConvex(int(i)), isConvex(int(i + 1))));
	}
	boundary.emplace_back(segment(vertices[vertices.size() - 1] * scale, vertices[0] * scale, isConvex(int(vertices.size() - 1)), isConvex(0)));
	return boundary;
}

std::vector<rynx::polygon::segment> rynx::polygon::generateBoundary_Inside(float scale) const {
	std::vector<segment> boundary = generateBoundary_Outside(scale);
	for (auto&& plop : boundary) {
		auto tmp = plop.p1;
		plop.p1 = plop.p2;
		plop.p2 = tmp;
		plop.computeNormalXY();
	}
	return boundary;
}