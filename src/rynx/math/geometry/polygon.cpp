
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


	std::vector<rynx::polygon::segment> rynx::polygon::generateBoundary_Outside() const {
		std::vector<segment> boundary;
		if (vertices.size() < 3) {
			return boundary;
		}

		for (size_t i = 0; i < vertices.size() - 1; ++i) {
			boundary.emplace_back(segment(vertices[i], vertices[i + 1], isConvex(int(i)), isConvex(int(i + 1))));
		}
		boundary.emplace_back(segment(vertices[vertices.size() - 1], vertices[0], isConvex(int(vertices.size() - 1)), isConvex(0)));
		return boundary;
	}

std::vector<rynx::polygon::segment> rynx::polygon::generateBoundary_Inside() const {
	std::vector<segment> boundary = generateBoundary_Outside();
	for (auto&& plop : boundary) {
		plop.invert_normal();
	}
	return boundary;
}