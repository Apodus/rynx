
#include <rynx/math/spline.hpp>

rynx::math::spline::spline(const std::vector<rynx::vec3f>& points, float alpha) {
	for (auto p : points)
		m_points.emplace_back(p, alpha);
}

rynx::math::spline& rynx::math::spline::operator = (const std::vector<rynx::vec3f>& points) {
	for (auto p : points)
		m_points.emplace_back(p);
	return *this;
}


std::vector<rynx::vec3f> rynx::math::spline::generate(int32_t num_segments) const {
	std::vector<rynx::vec3f> result;
	for (int32_t i = 0; i < m_points.size(); ++i) {
		auto p0 = m_points[(int32_t(m_points.size()) + i - 1) % m_points.size()];
		auto p1 = m_points[i % m_points.size()];
		auto p2 = m_points[(i + 1) % m_points.size()];
		auto p3 = m_points[(i + 2) % m_points.size()];

		auto compute_t = [](float t, control_point p0, control_point p1) {
			float a = (p0.position - p1.position).length_squared();
			float b = std::pow(a, (p0.alpha + p1.alpha) * 0.25f);
			return t + b;
		};

		float t0 = 0.0f;
		float t1 = compute_t(t0, p0, p1);
		float t2 = compute_t(t1, p1, p2);
		float t3 = compute_t(t2, p2, p3);

		const float step = (t2 - t1) / num_segments;
		for (float t = t1; t < t2 * 0.999f; t += step) {
			vec3f a1 =
				((t1 - t) / (t1 - t0)) * p0.position +
				((t - t0) / (t1 - t0)) * p1.position;

			vec3f a2 =
				((t2 - t) / (t2 - t1)) * p1.position +
				((t - t1) / (t2 - t1)) * p2.position;

			vec3f a3 =
				((t3 - t) / (t3 - t2)) * p2.position +
				((t - t2) / (t3 - t2)) * p3.position;

			vec3f b1 =
				((t2 - t) / (t2 - t0)) * a1 +
				((t - t0) / (t2 - t0)) * a2;

			vec3f b2 =
				((t3 - t) / (t3 - t1)) * a2 +
				((t - t1) / (t3 - t1)) * a3;

			vec3f combined =
				((t2 - t) / (t2 - t1)) * b1 +
				((t - t1) / (t2 - t1)) * b2;

			result.emplace_back(combined);
		}
	}

	return result;
}
