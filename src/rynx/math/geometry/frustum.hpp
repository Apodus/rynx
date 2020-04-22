
#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/math/geometry/plane.hpp>
#include <array>

namespace rynx {
	class matrix4;

	class frustum
	{
	public:
		void set_viewprojection(const rynx::matrix4& viewProjection);
		bool is_point_inside(vec3f p) const;
		bool is_sphere_inside(vec3f p, float radius) const;
		bool is_sphere_not_outside(vec3f p, float radius) const;
		bool is_sphere_outside(vec3f p, float radius) const;

	private:
		std::array<plane, 6> m_planes;
	};
}