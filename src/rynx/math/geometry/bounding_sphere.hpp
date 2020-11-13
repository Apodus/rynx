#pragma once

#include <rynx/math/vector.hpp>
#include <utility>
#include <vector>
#include <memory>

namespace rynx {
	namespace math {
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

		template<typename T> std::pair<size_t, float> farPoint(rynx::vec3<float> p, const std::vector<T>& points) {
			float maxDist = -1;
			size_t outIndex = 0;
			for (size_t i = 0; i < points.size(); ++i) {
				const auto& point = access(points[i]);
				float pointDist = (point.pos - p).length() + point.radius;
				if (pointDist > maxDist) {
					maxDist = pointDist;
					outIndex = i;
				}
			}
			return { outIndex, maxDist };
		}

		// Sphere from 3 or 4 points implementation details obtained from
		// https://www.flipcode.com/archives/Smallest_Enclosing_Spheres.shtml

		inline rynx::vec3f bounding_sphere(rynx::vec3f a, rynx::vec3f b, rynx::vec3f c) {
			const auto a_ = b - a;
			const auto b_ = c - a;

			const auto a_cross_b = a_.cross(b_);
			float Denominator = 2.0f * a_cross_b.length_squared();

			auto o = (a_cross_b.cross(a_) * b_.length_squared() + b_.cross(a_cross_b) * a_.length_squared()) / Denominator;
			return a + o;
		};

		template<typename T> std::pair<rynx::vec3<float>, float> bounding_sphere(std::vector<T>& points) {
			std::pair<size_t, float> indexFar1 = farPoint(access(points[0]).pos, points);
			std::pair<size_t, float> indexFar2 = farPoint(access(points[indexFar1.first]).pos, points);
			indexFar1 = farPoint(access(points[indexFar2.first]).pos, points);

			const auto a = access(points[indexFar1.first]).pos;
			const auto b = access(points[indexFar2.first]).pos;

			const float radius1 = access(points[indexFar1.first]).radius;
			const float radius2 = access(points[indexFar2.first]).radius;
			float maxRadius = radius1 > radius2 ? radius1 : radius2;

			auto atob = b - a;
			atob.normalize();

			auto center = (a + b + atob * (radius2 - radius1)) * 0.5f;
			std::pair<size_t, float> indexFar3 = farPoint(center, points);

			if (indexFar3.first != indexFar1.first && indexFar3.first != indexFar2.first) {
				const auto c = access(points[indexFar3.first]).pos;
				const float radius3 = access(points[indexFar3.first]).radius;
				maxRadius = maxRadius > radius3 ? maxRadius : radius3;

				const auto proposed_center = bounding_sphere(a, b, c);

				const auto a_push = (proposed_center - a).normalize();
				const auto b_push = (proposed_center - b).normalize();
				const auto c_push = (proposed_center - c).normalize();

				const auto a_fixed = a - a_push * radius1;
				const auto b_fixed = b - b_push * radius2;
				const auto c_fixed = c - c_push * radius3;

				const auto proposed_center_fixed = bounding_sphere(a_fixed, b_fixed, c_fixed);
				const auto weighted_radius = farPoint(proposed_center_fixed, points).second;
				return { proposed_center_fixed, weighted_radius };
			}
			else {
				return { center, (center - a).length() + radius1 };
			}
		}

		inline std::pair<rynx::vec3f, float> bounding_sphere(const std::vector<rynx::vec3f>& points) {
			auto furthest_point = [&](rynx::vec3f p) {
				float furthest_dist_sqr = 0;
				rynx::vec3f result;
				int32_t index = -1;
				int32_t counter = 0;
				for (auto point : points) {
					float dist_sqr = (p - point).length_squared();
					if (dist_sqr > furthest_dist_sqr) {
						furthest_dist_sqr = dist_sqr;
						result = point;
						index = counter;
					}
					++counter;
				}

				return std::make_pair(result, index);
			};

			auto [p_random, i_random] = furthest_point(points[0]);
			auto [p1, i1] = furthest_point(p_random);
			auto [p2, i2] = furthest_point(p1);
			auto [p3, i3] = furthest_point((p1+p2) * 0.5f);

			if ((i3 != i2) & (i3 != i1)) {
				auto sphere1 = bounding_sphere(p1, p2, p3);
				auto [radius_point, i_radius_point] = furthest_point(sphere1);
				return { sphere1, (sphere1 - radius_point).length() };
			}

			return { (p1 + p2) * 0.5f, (p1 - p2).length() * 0.5f };
		}
	}
}