
#include <rynx/math/geometry/polygon_editor.hpp>
#include <rynx/math/spline.hpp>

rynx::polygon_editor::polygon_editor(rynx::polygon& polygon) : polygon(polygon) {}

void rynx::polygon_editor::deleteVertex(int index) {
	auto& vertices = polygon.vertices;
	for (int i = index; i < static_cast<int>(vertices.size()) - 1; ++i) {
		vertices[i] = vertices[i + 1];
	}
	vertices.pop_back();
}

void rynx::polygon_editor::insertVertexAfter(int index, rynx::vec3<float> v) {
	auto& vertices = polygon.vertices;
	vertices.push_back(v);
	for (int i = static_cast<int>(vertices.size()) - 1; i > index; --i) {
		vertices[i] = vertices[i - 1];
	}
	vertices[index] = v;
}

void rynx::polygon_editor::insertVertex(rynx::vec3<float> v) {
	polygon.vertices.emplace_back(v);
}

void rynx::polygon_editor::vertexMultiply(float xMultiply, float yMultiply) {
	for (int i = 0; i < static_cast<int>(polygon.vertices.size()); ++i) {
		polygon[i].x *= xMultiply;
		polygon[i].y *= yMultiply;
	}
}

void rynx::polygon_editor::reverse() {
	int size = static_cast<int>(polygon.vertices.size());
	int bot = 0, top = size - 1;
	while (bot < top) {
		rynx::vec3<float> tmp = polygon[top];
		polygon[top] = polygon[bot];
		polygon[bot] = tmp;
		++bot;
		--top;
	}
}

void rynx::polygon_editor::smooth(int factor, float alpha) {
	polygon = polygon.as_spline(alpha).generate(factor);
}