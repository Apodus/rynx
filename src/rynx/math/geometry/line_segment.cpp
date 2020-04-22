
#include <rynx/math/geometry/line_segment.hpp>

#include <rynx/math/geometry/math.hpp>
#include <rynx/math/vector.hpp>

rynx::line_segment::line_segment() {}

rynx::line_segment::line_segment(vec3<float> a, vec3<float> b) {
	p1 = a;
	p2 = b;
	computeNormalXY();
}

rynx::line_segment::line_segment(const line_segment& other) {
	p1 = other.p1;
	p2 = other.p2;
	normal = other.normal;
}

float rynx::line_segment::length() const { return (p1 - p2).length(); }

// planar normal in xy plane.
rynx::vec3<float>& rynx::line_segment::computeNormalXY() {
	normal.x = p2.y - p1.y;
	normal.y = p1.x - p2.x;
	normal.normalize();
	return normal;
}

void rynx::line_segment::invert_normal() {
	normal *= -1.0f;
}

rynx::vec3<float> rynx::line_segment::computeNormalXY() const {
	vec3<float> normal_;
	normal_.x = p2.y - p1.y;
	normal_.y = p1.x - p2.x;
	normal_.normalize();
	return normal_;
}

const rynx::vec3<float>& rynx::line_segment::getNormalXY() const {
	return normal;
}

// in XY plane
bool rynx::line_segment::intersects(const line_segment& other) const {
	vec3<float> p = other.p1;
	vec3<float> r = other.p2 - other.p1;

	const vec3<float>& q = p1;
	vec3<float> s = p2 - p1;

	vec3<float> qp = (q - p);
	float rxs = r.cross2d(s);
	float qps = qp.cross2d(s);
	float qpr = qp.cross2d(r);

	if (rxs == 0) {
		if (qpr == 0) {
			// lines are colinear.
			float val_r = qp.x * r.x + qp.y * r.y;
			if (val_r > 0 && val_r < r.x * r.x + r.y * r.y)
				return true;

			float val_s = qp.x * s.x + qp.y * s.y;
			if (val_s > 0 && val_s < s.x * s.x + s.y * s.y)
				return true;
		}

		// parallel but disjoint
		return false;
	}

	float t = qps / rxs;
	float u = qpr / rxs;

	bool t_ok = t > 0 && t < 1;
	bool u_ok = u > 0 && u < 1;
	return t_ok && u_ok;
}

bool rynx::line_segment::approxEquals(const line_segment& lineSegment) { return math::distanceSquared(p1, lineSegment.p1) + math::distanceSquared(p2, lineSegment.p2) < 0.01f; }