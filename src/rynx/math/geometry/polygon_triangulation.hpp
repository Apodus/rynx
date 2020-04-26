
#pragma once

#include <rynx/math/geometry/polygon.hpp>
#include <rynx/math/vector.hpp>
#include <rynx/graphics/mesh/mesh.hpp>

#include <vector>
#include <memory>

namespace rynx {
	class mesh;
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

		std::unique_ptr<mesh> triangulate(rynx::polygon polygon_, floats4 uvLimits = floats4(0.0f, 0.0f, 1.0f, 1.0f));
		void makeTriangles(rynx::polygon& polygon_);
	};
}