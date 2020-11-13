
#include <rynx/math/geometry/polygon_editor.hpp>
#include <rynx/math/spline.hpp>

rynx::polygon_editor::polygon_editor(rynx::polygon& polygon) : polygon(polygon) {
	if (!polygon.m_vertices.empty())
		polygon.m_vertices.pop_back();
}

rynx::polygon_editor::~polygon_editor() {
	polygon.m_vertices.emplace_back(polygon.m_vertices.front());
	polygon.recompute_normals();
}

rynx::polygon_editor::vertex_t rynx::polygon_editor::vertex(int index) {
	return vertex_t(polygon, index);
}

rynx::polygon_editor& rynx::polygon_editor::erase(int index) {
	auto& vertices = polygon.m_vertices;
	vertices.erase(vertices.begin() + index);
	return *this;
}

rynx::polygon_editor& rynx::polygon_editor::insert(int index, rynx::vec3<float> v) {
	auto& vertices = polygon.m_vertices;
	vertices.insert(vertices.begin() + index, v);
	return *this;
}

rynx::polygon_editor& rynx::polygon_editor::push_back(rynx::vec3<float> v) {
	polygon.m_vertices.emplace_back(v);
	return *this;
}

rynx::polygon_editor& rynx::polygon_editor::scale(float xMultiply, float yMultiply) {
	polygon.scale({ xMultiply, yMultiply, 1.0f });
	return *this;
}

rynx::polygon_editor& rynx::polygon_editor::translate(rynx::vec3f v) {
	for (auto& vertex : polygon.m_vertices)
		vertex += v;
	return *this;
}

rynx::polygon_editor& rynx::polygon_editor::reverse() {
	polygon.invert();
	return *this;
}

rynx::polygon_editor& rynx::polygon_editor::smooth(int factor, float alpha) {
	polygon = polygon.as_spline(alpha).generate(factor);
	return *this;
}