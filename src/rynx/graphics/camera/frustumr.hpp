
#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/graphics/plane/plane.hpp>
#include <array>

namespace rynx {
	class matrix4;
}

class frustum
{
public:
	void set_viewprojection(const rynx::matrix4& viewProjection);
	bool is_point_inside(vec3<float> p) const;
	bool is_sphere_inside(vec3<float> p, float radius) const;
	bool is_sphere_not_outside(vec3f p, float radius) const;

private:
	std::array<plane, 6> m_planes;
};
