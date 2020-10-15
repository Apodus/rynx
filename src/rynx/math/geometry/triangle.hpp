#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/math/geometry/math.hpp>

#include <vector>

namespace rynx {
	class triangle {
	public:
		triangle(rynx::vec3f a, rynx::vec3f b, rynx::vec3f c) : a(a), b(b), c(c) {}
		
		rynx::vec3f centre_of_mass() const;
		float area() const;
		bool point_in_triangle(rynx::vec3f p) const;

	private:
		rynx::vec3f a, b, c;
	};

	class triangles {
	public:
		bool point_in_polygon(rynx::vec3f point) const;
		rynx::vec3f centre_of_mass() const;

		std::vector<rynx::triangle>& data() { return tris; }
		const std::vector<rynx::triangle>& data() const { return tris; }

	private:
		std::vector<rynx::triangle> tris;
	};
}