#pragma once

#include <rynx/graphics/mesh/polygon.hpp>

template<class T>
class PolygonEditor {

	Polygon<T>& polygon;

public:
	PolygonEditor(Polygon<T>& polygon) : polygon(polygon) {
	}

	void deleteVertex(int index) {
		auto& vertices = polygon.vertices;
		for (int i = index; i < vertices.size() - 1; ++i) {
			vertices[i] = vertices[i + 1];
		}
		vertices.pop_back();
	}

	void insertVertexAfter(int index, const T& v) {
		auto& vertices = polygon.vertices;
		vertices.push_back(v);
		for (int i = vertices.size() - 1; i > index; --i) {
			vertices[i] = vertices[i - 1];
		}
		vertices[index] = v;
	}

	void insertVertex(const T& v) {
		polygon.vertices.push_back(v);
	}

	void vertexMultiply(float xMultiply, float yMultiply) {
		for (int i = 0; i < polygon.vertices.size(); ++i) {
			polygon[i].x *= xMultiply;
			polygon[i].y *= yMultiply;
		}
	}

	void reverse() {
		int size = static_cast<int>(polygon.vertices.size());
		int bot = 0, top = size - 1;
		while (bot < top) {
			T tmp = polygon[top];
			polygon[top] = polygon[bot];
			polygon[bot] = tmp;
			++bot;
			--top;
		}
	}
};

