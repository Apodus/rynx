
#pragma once

#include <rynx/math/geometry/line_segment.hpp>
#include <rynx/math/geometry/math.hpp>
#include <vector>

namespace rynx {

	class polygon {
	public:
		class segment : public line_segment {
		public:
			bool convex1;
			bool convex2;

			segment();
			segment(segment&&) = default;
			segment(const segment&) = default;

			segment& operator = (segment&&) = default;
			segment& operator = (const segment&) = default;
			
			segment(const line_segment& lineSegment);
			segment(rynx::vec3<float> a, rynx::vec3<float> b, bool c1, bool c2);
		};

		std::vector<rynx::vec3<float>> vertices;

		rynx::vec3<float>& operator [](int index);
		const rynx::vec3<float>& operator [](int index) const;

		bool isConvex(int vertex) const;
		float normalize();
		void scale(float s);
		void scale(vec3f ranges);
		
		rynx::vec3<std::pair<float, float>> extents() const;

		float radius() const;
		float max_component_value() const;

		std::vector<segment> generateBoundary_Outside(float scale) const;
		std::vector<segment> generateBoundary_Inside(float scale) const;
	};
}