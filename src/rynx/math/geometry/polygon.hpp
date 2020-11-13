
#pragma once

#include <rynx/math/geometry/line_segment.hpp>
#include <rynx/math/geometry/math.hpp>

#include <rynx/tech/dynamic_buffer.hpp>

#include <vector>

namespace rynx {
	namespace math {
		class spline;
	}

	class polygon_editor;

	class polygon {
	public:
		friend class polygon_editor;

		polygon() = default;
		polygon(const polygon& other) = default;
		polygon(polygon&& other) = default;

		polygon(std::vector<rynx::vec3f> verts) : m_vertices(std::move(verts)) { recompute_normals(); }
		polygon(std::vector<rynx::vec3f>&& verts) : m_vertices(std::move(verts)) { recompute_normals(); }

		polygon& operator = (const std::vector<rynx::vec3f>& verts) { m_vertices = verts; recompute_normals(); return *this; }
		polygon& operator = (std::vector<rynx::vec3f>&& verts) { m_vertices = std::move(verts); recompute_normals(); return *this; }

		polygon& operator = (const polygon& other) = default;
		polygon& operator = (polygon&& other) = default;

		rynx::math::spline as_spline(float alpha = 1.0f) const;

		bool isConvex(int vertex) const;
		float normalize();
		rynx::polygon& scale(float s);
		rynx::polygon& scale(vec3f ranges);
		
		rynx::vec3<std::pair<float, float>> extents() const;

		float radius() const;
		float max_component_value() const;
		std::pair<rynx::vec3f, float> bounding_sphere() const;

		rynx::vec3f vertex_position(size_t i) const { return m_vertices[i]; }
		rynx::vec3f segment_normal(size_t i) const { return m_segment_normal[i]; }
		rynx::vec3f vertex_normal(size_t i) const { return m_vertex_normal[i]; }

		rynx::line_segment segment(size_t i) const {
			return rynx::line_segment(vertex_position(i), vertex_position(i + 1), segment_normal(i));
		}

		size_t size() const { return m_vertex_normal.size(); }

		void invert() {
			std::reverse(m_vertices.begin(), m_vertices.end());
			recompute_normals();
		}

		rynx::polygon& recompute_normals() {
			m_vertex_normal.resize(m_vertices.size() - 1);
			m_segment_normal.resize(m_vertices.size() - 1);

			for (size_t i = 0; i < m_segment_normal.size(); ++i) {
				m_segment_normal[i] = (m_vertices[i] - m_vertices[i + 1]).normal2d().normalize();
			}

			for (size_t i = 1; i < m_vertex_normal.size(); ++i) {
				m_vertex_normal[i] = (m_segment_normal[i] + m_segment_normal[i - 1]).normalize();
			}
			m_vertex_normal[0] = (m_segment_normal.front() + m_segment_normal.back()).normalize();
			return *this;
		}

		rynx::polygon& recompute_normals(int32_t vertex) {
			m_segment_normal[vertex] = (m_vertices[vertex] - m_vertices[(vertex + 1) % m_segment_normal.size()]).normal2d().normalize();
			size_t prev = (vertex + m_vertices.size() - 2) % (m_vertices.size() - 1);
			m_segment_normal[prev] = (m_vertices[prev] - m_vertices[prev + 1]).normal2d().normalize();
			m_vertex_normal[vertex] = (m_segment_normal[vertex] + m_segment_normal[(vertex - 1 + m_segment_normal.size()) % m_segment_normal.size()]).normalize();
			return *this;
		}

		polygon_editor edit();

	private:
		rynx::pod_vector<rynx::vec3f> m_vertices;
		rynx::pod_vector<rynx::vec3f> m_vertex_normal;
		rynx::pod_vector<rynx::vec3f> m_segment_normal;
	};
}