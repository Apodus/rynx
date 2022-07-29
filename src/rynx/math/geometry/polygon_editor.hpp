#pragma once

#include <rynx/math/geometry/polygon.hpp>

namespace rynx {
	class MathDLL polygon_editor {
		rynx::polygon& polygon;

	public:
		polygon_editor(rynx::polygon& polygon);
		~polygon_editor();
		
		class vertex_t {
		public:
			vertex_t(rynx::polygon& host, int32_t index) : host(host), index(index) {}

			vertex_t& position(rynx::vec3f v) {
				host.m_vertices[index] = v;
				host.recompute_normals(index);
				return *this;
			}

			vertex_t& set(rynx::vec3f pos, rynx::vec3f segment_normal, rynx::vec3f vertex_normal) {
				host.m_vertices[index] = pos;
				host.m_segment_normal[index] = segment_normal;
				host.m_vertex_normal[index] = vertex_normal;
				return *this;
			}

		private:
			rynx::polygon& host;
			int32_t index;
		};

		vertex_t vertex(int index);
		polygon_editor& erase(int index);
		polygon_editor& insert(int index, rynx::vec3<float> v);
		polygon_editor& push_back(rynx::vec3<float> v);
		polygon_editor& emplace_back(rynx::vec3f v) { push_back(v); return *this; }
		polygon_editor& scale(float xMultiply, float yMultiply);
		polygon_editor& translate(rynx::vec3f v);
		polygon_editor& reverse();
		polygon_editor& smooth(int factor = 3, float alpha = 1.0f);
	};
}
