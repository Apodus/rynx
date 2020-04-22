
#include <rynx/math/geometry/frustum.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/math/vector.hpp>
#include <cmath>
#include <cstdio>


void rynx::frustum::set_viewprojection(const rynx::matrix4& viewProjection)
{
	auto m = [&viewProjection](int x, int y) {
		return viewProjection.m[x + 4 * y];
	};

	{
		// left plane
		float a = m(3, 0) + m(0, 0);
		float b = m(3, 1) + m(0, 1);
		float c = m(3, 2) + m(0, 2);
		float d = m(3, 3) + m(0, 3);
		m_planes[0].set_coefficients(a, b, c, d);
	}

	{
		// right plane
		float a = m(3, 0) - m(0, 0);
		float b = m(3, 1) - m(0, 1);
		float c = m(3, 2) - m(0, 2);
		float d = m(3, 3) - m(0, 3);
		m_planes[1].set_coefficients(a, b, c, d);
	}

	{
		// top plane
		float a = m(3, 0) - m(1, 0);
		float b = m(3, 1) - m(1, 1);
		float c = m(3, 2) - m(1, 2);
		float d = m(3, 3) - m(1, 3);
		m_planes[2].set_coefficients(a, b, c, d);
	}

	{
		// top plane
		float a = m(3, 0) + m(1, 0);
		float b = m(3, 1) + m(1, 1);
		float c = m(3, 2) + m(1, 2);
		float d = m(3, 3) + m(1, 3);
		m_planes[3].set_coefficients(a, b, c, d);
	}

	{
		// near
		float a = m(3, 0) + m(2, 0);
		float b = m(3, 1) + m(2, 1);
		float c = m(3, 2) + m(2, 2);
		float d = m(3, 3) + m(2, 3);
		m_planes[4].set_coefficients(a, b, c, d);
	}

	{
		// far
		float a = m(3, 0) - m(2, 0);
		float b = m(3, 1) - m(2, 1);
		float c = m(3, 2) - m(2, 2);
		float d = m(3, 3) - m(2, 3);
		m_planes[5].set_coefficients(a, b, c, d);
	}
}

bool rynx::frustum::is_point_inside(vec3<float> p) const
{
	bool in = true;
	for (auto&& plane : m_planes)
		in &= plane.point_right_of_plane(p);
	return in;
}

bool rynx::frustum::is_sphere_inside(vec3<float> p, float radius) const
{
	bool in = true;
	for (const auto& plane : m_planes)
		in &= plane.sphere_right_of_plane(p, radius);
	return in;
}

bool rynx::frustum::is_sphere_outside(vec3f p, float radius) const {
	bool out = false;
	for (const auto& plane : m_planes)
		out |= plane.sphere_left_of_plane(p, radius);
	return out;
}

bool rynx::frustum::is_sphere_not_outside(vec3f p, float radius) const {
	bool in = true;
	for (const auto& plane : m_planes)
		in &= plane.sphere_not_left_of_plane(p, radius);
	return in;
}