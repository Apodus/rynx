
#pragma once

#include <rynx/graphics/mesh/polygon.hpp>
#include <utility> // for std::pair

#ifdef min
#undef min
#endif


	template<typename T>
	class PolygonModifier {
		typedef decltype(T().x) Scalar;
	
	public:
		PolygonModifier(Polygon<T>& polygon);
		PolygonModifier& roundEdges(const Scalar& amount, int iterationCount = 1);
		PolygonModifier& bend_S(const Scalar& amount);
		PolygonModifier& bend_U(const Scalar& amount);

		PolygonModifier& align(T& pos1, const Scalar& rot1, const T& pos2, const Scalar& rot2, const Polygon<T>& other);
		PolygonModifier& merge(const Polygon<T>& other);

	private:
		Polygon<T>& m_polygon;
		std::vector<T> m_modifiers;

		static std::pair<Scalar, T> minimumDistance(const T& point, const Polygon<T>& polygon, const T& pos, const Scalar& rot);
		T& getPrev(int index) { if (index == 0) return m_polygon.vertices.back(); return m_polygon.vertices[index - 1]; }
		T& getNext(int index) { if (index + 1 == m_polygon.vertices.size()) return m_polygon.vertices.front(); return m_polygon.vertices[index + 1];}
	};

template<typename T> PolygonModifier<T>::PolygonModifier(Polygon<T>& polygon) : m_polygon(polygon) { m_modifiers.resize(polygon.vertices.size()); }

template<typename T> PolygonModifier<T>& PolygonModifier<T>::roundEdges(const Scalar& amount, int iterationCount) {
	typedef decltype(T().x) Scalar;
	auto& verts = m_polygon.vertices;

	for (int count = 0; count < iterationCount; ++count) {
		for (int i = 0; i < static_cast<int>(verts.size()); ++i) {
			auto& currentVertice = m_polygon.vertices[i];
			auto midPoint = (getPrev(i) + getNext(i)) / Scalar(2);
			auto direction = midPoint - currentVertice;
			m_modifiers[i] = direction * amount;
		}

		for (size_t i = 0; i < verts.size(); ++i) {
			m_polygon.vertices[i] += m_modifiers[i];
		}
	}

	return *this;
}

template<typename T> PolygonModifier<T>& PolygonModifier<T>::bend_U(const Scalar& amount) {
	Scalar top_x = -Scalar(1000);
	for (auto& v : m_polygon.vertices) {
		if (v.x > top_x)
			top_x = v.x;
	}

	// For now just assume an AA rectangle, with width as the long side.
	Scalar c = amount * math::PI<Scalar>();
	auto& verts = m_polygon.vertices;
	for (int i = 0; i < static_cast<int>(verts.size()); ++i) {
		auto& v = verts[i];
		Scalar x_trig = c * v.x / top_x;
		
		vec3<Scalar> rot_x(v.x, Scalar(0), Scalar(0));
		math::rotateXY(rot_x, -x_trig);

		vec3<Scalar> rot_addition(Scalar(0), v.y, Scalar(0));
		math::rotateXY(rot_addition, -x_trig * Scalar(2));

		v.y = rot_x.y + rot_addition.y;
		v.x = rot_x.x + rot_addition.x;
	}
	return *this;
}

template<typename T> PolygonModifier<T>& PolygonModifier<T>::bend_S(const Scalar& amount) {
	Scalar top_x = -Scalar(1000);
	for (auto& v : m_polygon.vertices) {
		if (v.x > top_x)
			top_x = v.x;
	}

	// For now just assume an AA rectangle, with width as the long side.
	Scalar c = amount * math::PI<Scalar>();
	auto& verts = m_polygon.vertices;
	for (int i = 0; i < static_cast<int>(verts.size()); ++i) {
		auto& v = verts[i];
		Scalar x_trig = c * v.x / top_x;

		if (v.x > Scalar(0))
			x_trig *= Scalar(-1);

		vec3<Scalar> rot_x(v.x, Scalar(0), Scalar(0));
		math::rotateXY(rot_x, -x_trig);

		vec3<Scalar> rot_addition(Scalar(0), v.y, Scalar(0));
		math::rotateXY(rot_addition, -x_trig * Scalar(2));

		v.y = rot_x.y + rot_addition.y;
		v.x = rot_x.x + rot_addition.x;
	}
	return *this;
}

template<typename T> PolygonModifier<T>& PolygonModifier<T>::align(T& pos1, const Scalar& rot1, const T& pos2, const Scalar& rot2, const Polygon<T>& other) {
	// find a pair of vertices that are closest to a pair on the other polygon
	int firstIndex = 0;
	Scalar minimumEvaluatedValue = Scalar(10000);
	auto& vertices = m_polygon.vertices;
	std::pair<T, T> matchingVertices;
	for (int index = 0; index < vertices.size(); ++index) {
		std::pair<Scalar, T> val1 = minimumDistance(math::rotatedXY(vertices[index], rot1) + pos1, other, pos2, rot2);
		std::pair<Scalar, T> val2 = minimumDistance(math::rotatedXY(getNext(index), rot1), other, pos2, rot2);

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
		T mod = math::rotatedXY(v1, rot1) + math::rotatedXY(v2, rot1) - math::rotatedXY(matchingVertices.first, rot2) - math::rotatedXY(matchingVertices.second, rot2) + pos1 * Scalar(2) - pos2 * Scalar(2);
		mod /= Scalar(2);
		for (auto& v : vertices) {
			pos1 += mod;
		}
	}

	return *this;
}

template<typename T> PolygonModifier<T>& PolygonModifier<T>::merge(const Polygon<T>& other) {
	// find a pair of vertices that are closest to a pair on the other polygon.

	// enumerate all vertices in proper order to create the merged polygon.
}

template<typename T> std::pair<decltype(T().x), T> PolygonModifier<T>::minimumDistance(const T& point, const Polygon<T>& polygon, const T& pos, const Scalar& rot) {
	Scalar minDist(10000);
	T t;
	for (const auto& v : polygon.vertices) {
		Scalar dist = (math::rotatedXY(v, rot) + pos).distanceSquared(point);
		if (dist < minDist) {
			minDist = dist;
			t = v;
		}
	}
	
	return std::make_pair(minDist, t);
}