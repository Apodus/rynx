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

		/*
		Sphere::Sphere(const Point& O, const Point& A, const Point& B, const Point& C)
		{
			Vector a = A - O;
			Vector b = B - O;
			Vector c = C - O;

			float Denominator = 2.0f * Matrix::det(a.x, a.y, a.z,
				b.x, b.y, b.z,
				c.x, c.y, c.z);

			Vector o = (c.length_squared()) * (a_cross_b) +
				(b.length_squared()) * (c_cross_a) +
				(a.length_squared()) * (b_cross_c)) / Denominator;

			radius = length(o);
			center = O + o;
		}
		*/

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

				auto bounding_sphere_for_points = [](auto a, auto b, auto c) {
					const auto a_ = b - a;
					const auto b_ = c - a;

					const auto a_cross_b = a_.cross(b_);
					float Denominator = 2.0f * a_cross_b.length_squared();

					auto o = (a_cross_b.cross(a_) * b_.length_squared() + b_.cross(a_cross_b) * a_.length_squared()) / Denominator;
					return a + o;
				};

				const auto proposed_center = bounding_sphere_for_points(a, b, c);

				const auto a_push = (proposed_center - a).normalize();
				const auto b_push = (proposed_center - b).normalize();
				const auto c_push = (proposed_center - c).normalize();

				const auto a_fixed = a - a_push * radius1;
				const auto b_fixed = b - b_push * radius2;
				const auto c_fixed = c - c_push * radius3;

				const auto proposed_center_fixed = bounding_sphere_for_points(a_fixed, b_fixed, c_fixed);
				const auto weighted_radius = farPoint(proposed_center_fixed, points).second;
				return { proposed_center_fixed, weighted_radius };
			}
			else {
				return { center, (center - a).length() + radius1 };
			}
		}
	}
}