
#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/graphics/mesh/mesh.hpp>
#include <rynx/math/geometry/polygon.hpp>

#include <vector>
#include <memory>

namespace rynx {
	class mesh;
	class polygon;
	class polygon_triangulation {
		rynx::polygon* polygon;
		std::vector<int> unhandled;

		std::vector<typename rynx::polygon::triangle>& getTriangles();
		void resetUnhandledMarkers();
		void resetForPostProcess();

		rynx::vec3<float>& getVertexUnhandled(int i);
		void addTriangle(int earNode, int t1, int t2);

		std::unique_ptr<mesh> buildMeshData(floats4 uvLimits);
		bool isEar(int t1, int t2, int t3);
		bool triangulateOneStep();
		void triangulate();


	public:
		polygon_triangulation();

		std::unique_ptr<mesh> triangulate(const rynx::polygon& polygon_, floats4 uvLimits = floats4(0.0f, 0.0f, 1.0f, 1.0f));
		std::unique_ptr<mesh> generate_polygon_boundary(const rynx::polygon& polygon_, floats4 texCoords);
		void makeTriangles(rynx::polygon& polygon_);
	};
}