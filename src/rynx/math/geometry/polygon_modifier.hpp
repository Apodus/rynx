
#pragma once

#include <rynx/math/geometry/polygon.hpp>
#include <utility> // for std::pair

#ifdef min
#undef min
#endif


	class PolygonModifier {
	public:
		PolygonModifier(rynx::Polygon& polygon);
		PolygonModifier& roundEdges(float amount, int iterationCount = 1);
		PolygonModifier& bend_S(float amount);
		PolygonModifier& bend_U(float amount);

		PolygonModifier& align(rynx::vec3<float>& pos1, float rot1, rynx::vec3<float> pos2, float rot2, rynx::vec3<float> other);
		PolygonModifier& merge(const rynx::Polygon& other);

	private:
		rynx::Polygon& m_polygon;
		std::vector<rynx::vec3<float>> m_modifiers;

		static std::pair<float, rynx::vec3<float>> minimumDistance(rynx::vec3<float> point, const rynx::Polygon& polygon, rynx::vec3<float> pos, float rot);
		rynx::vec3<float>& getPrev(int index) { if (index == 0) return m_polygon.vertices.back(); return m_polygon.vertices[index - 1]; }
		rynx::vec3<float>& getNext(int index) { if (index + 1 == m_polygon.vertices.size()) return m_polygon.vertices.front(); return m_polygon.vertices[index + 1];}
	};

PolygonModifier::PolygonModifier(rynx::Polygon& polygon) : m_polygon(polygon) { m_modifiers.resize(polygon.vertices.size()); }

PolygonModifier& PolygonModifier::roundEdges(float amount, int iterationCount) {
	auto& verts = m_polygon.vertices;

	for (int count = 0; count < iterationCount; ++count) {
		for (int i = 0; i < static_cast<int>(verts.size()); ++i) {
			auto& currentVertice = m_polygon.vertices[i];
			auto midPoint = (getPrev(i) + getNext(i)) * 0.5f;
			auto direction = midPoint - currentVertice;
			m_modifiers[i] = direction * amount;
		}

		for (size_t i = 0; i < verts.size(); ++i) {
			m_polygon.vertices[i] += m_modifiers[i];
		}
	}

	return *this;
}

PolygonModifier& PolygonModifier::bend_U(float amount) {
	float top_x = -1000.0f;
	for (auto& v : m_polygon.vertices) {
		if (v.x > top_x)
			top_x = v.x;
	}

	// For now just assume an AA rectangle, with width as the long side.
	float c = amount * rynx::math::pi;
	auto& verts = m_polygon.vertices;
	for (int i = 0; i < static_cast<int>(verts.size()); ++i) {
		auto& v = verts[i];
		float x_trig = c * v.x / top_x;
		
		rynx::vec3<float> rot_x(v.x, 0.0f, 0.0f);
		rynx::math::rotateXY(rot_x, -x_trig);

		rynx::vec3<float> rot_addition(0.0f, v.y, 0.0f);
		rynx::math::rotateXY(rot_addition, -x_trig * 2.0f);

		v.y = rot_x.y + rot_addition.y;
		v.x = rot_x.x + rot_addition.x;
	}
	return *this;
}

PolygonModifier& PolygonModifier::bend_S(float amount) {
	float top_x = -1000;
	for (auto& v : m_polygon.vertices) {
		if (v.x > top_x)
			top_x = v.x;
	}

	// For now just assume an AA rectangle, with width as the long side.
	float c = amount * rynx::math::pi;
	auto& verts = m_polygon.vertices;
	for (int i = 0; i < static_cast<int>(verts.size()); ++i) {
		auto& v = verts[i];
		float x_trig = c * v.x / top_x;

		if (v.x > 0.0f)
			x_trig *= -1;

		rynx::vec3<float> rot_x(v.x, 0.0f, 0.0f);
		rynx::math::rotateXY(rot_x, -x_trig);

		rynx::vec3<float> rot_addition(0.0f, v.y, 0.0f);
		rynx::math::rotateXY(rot_addition, -x_trig * 2.0f);

		v.y = rot_x.y + rot_addition.y;
		v.x = rot_x.x + rot_addition.x;
	}
	return *this;
}

PolygonModifier& PolygonModifier::align(rynx::vec3<float> pos1, float rot1, rynx::vec3<float> pos2, float rot2, const rynx::Polygon& other) {
	// find a pair of vertices that are closest to a pair on the other polygon
	int firstIndex = 0;
	float minimumEvaluatedValue = 10000;
	auto& vertices = m_polygon.vertices;
	std::pair<rynx::vec3<float>, rynx::vec3<float>> matchingVertices;
	for (int index = 0; index < vertices.size(); ++index) {
		std::pair<float, rynx::vec3<float>> val1 = minimumDistance(rynx::math::rotatedXY(vertices[index], rot1) + pos1, other, pos2, rot2);
		std::pair<float, rynx::vec3<float>> val2 = minimumDistance(rynx::math::rotatedXY(getNext(index), rot1), other, pos2, rot2);

		if (val1.first + val2.first < minimumEvaluatedValue) {
			minimumEvaluatedValue = val1.first + val2.first;
			firstIndex = index;
			matchingVertices.first = val1.second;
			matchingVertices.second = val2.second;
		}
	}

	// move to match
	const auto& v1 = m_polygon.vertices[firstIndex];
	const auto& v2 = getNext(firstIndex);

	for (int i = 0; i < 4; ++i) {
		rynx::vec3<float> mod = rynx::math::rotatedXY(v1, rot1) + rynx::math::rotatedXY(v2, rot1) - rynx::math::rotatedXY(matchingVertices.first, rot2) - rynx::math::rotatedXY(matchingVertices.second, rot2) + (pos1 - pos2) * 2;
		mod *= 0.5f;
		for (auto& v : vertices) {
			pos1 += mod;
		}
	}

	return *this;
}

PolygonModifier& PolygonModifier::merge(const rynx::Polygon& other) {
	// find a pair of vertices that are closest to a pair on the other polygon.
	// enumerate all vertices in proper order to create the merged polygon.
}

std::pair<float, rynx::vec3<float>> PolygonModifier::minimumDistance(rynx::vec3<float> point, const rynx::Polygon& polygon, rynx::vec3<float> pos, float rot) {
	float minDist(10000);
	rynx::vec3<float> t;
	for (const auto& v : polygon.vertices) {
		float dist = (rynx::math::rotatedXY(v, rot) + pos - point).length_squared();
		if (dist < minDist) {
			minDist = dist;
			t = v;
		}
	}
	
	return std::make_pair(minDist, t);
}

