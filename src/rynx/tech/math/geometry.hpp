#pragma once

#include <rynx/tech/math/vector.hpp>
#include <utility>
#include <vector>
#include <memory>

template<typename T> struct access_object {
	T& operator()(T& t) { return t; }
	const T& operator()(const T& t) { return t; }
};

template<typename T> struct access_object<std::unique_ptr<T>> {
	T& operator()(std::unique_ptr<T>& t) { return *t; }
	const T& operator()(const std::unique_ptr<T>& t) { return *t; }
};

template<typename T> struct access_object<std::shared_ptr<T>> {
	T& operator()(std::shared_ptr<T>& t) { return *t; }
	const T& operator()(const std::shared_ptr<T>& t) { return *t; }
};

template<typename T> struct access_object<T*> {
	T& operator()(T* t) { return *t; }
	const T& operator()(const T* t) { return *t; }
};

template<typename T> auto& access(T& t) { return access_object<T>()(t); }
template<typename T> const auto& access(const T& t) { return access_object<T>()(t); }

template<typename T> std::pair<size_t, float> farPoint(vec3<float> p, const std::vector<T>& points) {
	float maxDist = -1;
	size_t outIndex = 0;
	for (size_t i = 0; i < points.size(); ++i) {
		const auto& point = access(points[i]);
		float pointDist = (point.pos - p).lengthApprox() + point.radius;
		if (pointDist > maxDist) {
			maxDist = pointDist;
			outIndex = i;
		}
	}
	return { outIndex, maxDist };
}

template<typename T> std::pair<vec3<float>, float> boundingSphere(std::vector<T>& points) {
	std::pair<size_t, float> indexFar1 = farPoint(access(points[0]).pos, points);
	std::pair<size_t, float> indexFar2 = farPoint(access(points[indexFar1.first]).pos, points);
	indexFar1 = farPoint(access(points[indexFar2.first]).pos, points);

	auto a = access(points[indexFar1.first]).pos;
	auto b = access(points[indexFar2.first]).pos;

	float radius1 = access(points[indexFar1.first]).radius;
	float radius2 = access(points[indexFar2.first]).radius;
	float maxRadius = radius1 > radius2 ? radius1 : radius2;

	auto atob = b - a;
	atob.normalizeApprox();

	auto center = (a - atob * radius1 + b + atob * radius2) * 0.5f;
	std::pair<size_t, float> indexFar3 = farPoint(center, points);

	if (indexFar3.first != indexFar1.first && indexFar3.first != indexFar2.first) {
		// points[indexFar3.first].selected = 1;
		center = (access(points[indexFar1.first]).pos + access(points[indexFar2.first]).pos + access(points[indexFar3.first]).pos) * 0.333333333f;
		float sqr1 = (center - access(points[indexFar1.first]).pos).lengthSquared();
		float sqr2 = (center - access(points[indexFar2.first]).pos).lengthSquared();
		float sqr3 = (center - access(points[indexFar3.first]).pos).lengthSquared();
		float sqr = sqr1 > sqr2 ? sqr1 : sqr2;
		sqr = sqr > sqr3 ? sqr : sqr3;

		float radius3 = access(points[indexFar3.first]).radius;
		maxRadius = maxRadius > radius3 ? maxRadius : radius3;
		return { center, std::sqrtf(sqr) + maxRadius };
	}
	else {
		return { center, (center - access(points[indexFar1.first]).pos).lengthApprox() + maxRadius };
	}
}