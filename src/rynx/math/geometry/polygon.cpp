
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

float rynx::polygon::normalize() {
	float r = radius();
	scale(1.0f / r);
	return r;
}

void rynx::polygon::scale(float s) {
	for (auto& vertice : vertices) {
		vertice *= s;
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