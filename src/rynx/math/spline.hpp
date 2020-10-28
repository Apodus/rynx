
#pragma once

#include <rynx/math/vector.hpp>
#include <vector>

namespace rynx {
	namespace math {
		class spline {
		public:
			struct control_point {
				control_point() = default;
				control_point(rynx::vec3f p, float alpha = 1.0f) : alpha(alpha), position(p) {}

				float alpha = 1.0f;
				rynx::vec3f position;
			};

			spline() {}
			spline(const std::vector<rynx::vec3f>& points, float alpha = 1.0f);
			spline& operator = (const std::vector<rynx::vec3f>& points);
			std::vector<rynx::vec3f> generate(int32_t num_segments) const;

			std::vector<control_point> m_points;
		};
	}
}
