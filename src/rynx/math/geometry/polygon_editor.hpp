#pragma once

#include <rynx/math/geometry/polygon.hpp>

namespace rynx {
	class polygon_editor {
		rynx::polygon& polygon;

	public:
		polygon_editor(rynx::polygon& polygon);
		void deleteVertex(int index);
		void insertVertexAfter(int index, rynx::vec3<float> v);
		void insertVertex(rynx::vec3<float> v);
		void vertexMultiply(float xMultiply, float yMultiply);
		void reverse();
	};
}
