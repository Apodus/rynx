
#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/graphics/mesh/mesh.hpp>
#include <rynx/math/geometry/polygon.hpp>

#include <vector>
#include <memory>

namespace rynx {
	namespace graphics {
		class mesh;
	}
	class triangles;
	class polygon;
	class polygon_triangulation {
	public:
		struct triangle {
			int a, b, c;
			triangle(int a, int b, int c) : a(a), b(b), c(c) {}
		};

	private:
		const rynx::polygon* polygon = nullptr;
		std::vector<int> unhandled;
		std::vector<polygon_triangulation::triangle> triangles;

		void resetUnhandledMarkers();
		void resetForPostProcess();

		rynx::vec3<float> getVertexUnhandled(int i);
		void addTriangle(int earNode, int t1, int t2);

		std::unique_ptr<rynx::graphics::mesh> buildMeshData(floats4 uvLimits, rynx::vec3<std::pair<float, float>> poly_extents);
		bool isEar(int t1, int t2, int t3);
		bool triangulateOneStep();
		void triangulate();

		void makeTriangles(const rynx::polygon& polygon_);

	public:
		polygon_triangulation() = default;

		std::vector<polygon_triangulation::triangle> make_triangle_indices(const rynx::polygon& polygon_);
		rynx::triangles make_triangles(const rynx::polygon& polygon_);
		
		std::unique_ptr<rynx::graphics::mesh> make_mesh(const rynx::polygon& polygon_, floats4 uvLimits = floats4(0.0f, 0.0f, 1.0f, 1.0f));
		std::unique_ptr<rynx::graphics::mesh> make_boundary_mesh(const rynx::polygon& polygon_, floats4 texCoords, float line_width = 1.0f);
	};
}