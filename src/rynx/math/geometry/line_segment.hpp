
#pragma once

#include <rynx/math/geometry/math.hpp>
#include <rynx/math/vector.hpp>

namespace rynx {
	class line_segment {
	public:
		line_segment();
		line_segment(vec3<float> a, vec3<float> b);
		line_segment(const line_segment& other);

		float length() const;

		// planar normal in xy plane.
		vec3<float>& computeNormalXY();
		vec3<float> computeNormalXY() const;
		const vec3<float>& getNormalXY() const;

		// in XY plane
		bool intersects(const line_segment& other) const;
		bool approxEquals(const line_segment& lineSegment);
		void invert_normal();

		vec3<float> p1;
		vec3<float> p2;
		vec3<float> normal;
	};
}