﻿
#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/math/math.hpp>

#include <cmath>
#include <utility>
#include <cstdint>

#ifdef max
#undef max
#endif

namespace rynx {
	namespace math {
		template<class T>
		inline T& rotateXY(T& v, decltype(T().x) radians) {
			decltype(T().x) cosVal = math::cos(radians);
			decltype(T().x) sinVal = math::sin(radians);
			decltype(T().x) xNew = (v.x * cosVal - v.y * sinVal);
			decltype(T().x) yNew = (v.x * sinVal + v.y * cosVal);
			v.x = xNew;
			v.y = yNew;
			return v;
		}


		inline vec3<float> rotatedXY(vec3<float> v, float sin_value, float cos_value) {
			return vec3<float>{
				v.x * cos_value - v.y * sin_value,
				v.x * sin_value + v.y * cos_value,
				0
			};
		}

		template<class T>
		inline T rotatedXY(const T& v, decltype(T().x) radians) {
			return rotatedXY(v, math::sin(radians), math::cos(radians));
		}

		inline vec3<float> velocity_at_point_2d(vec3<float> pos, float angular_velocity) {
			return pos.length() * angular_velocity * pos.normal2d().normalize();
		}

		template<class P>
		inline auto distanceSquared(const P& p1, const P& p2) {
			decltype(p1.x) x = (p1.x - p2.x);
			decltype(p1.x) y = (p1.y - p2.y);
			return x * x + y * y;
		}

		inline std::pair<float, vec3<float>> pointDistanceLineSegmentSquared(vec3<float> v, vec3<float> w, vec3<float> p) {
			const float l2 = (v - w).length_squared();
			if (l2 == 0.0) return { (p - v).length_squared(), v };
			float bound = (p - v).dot(w - v) / l2;
			bound = bound < 1 ? bound : 1;
			bound = bound > 0 ? bound : 0;
			const vec3<float> projection = v + (w - v) * bound;
			return { (p - projection).length_squared(), projection };
		}

		inline std::pair<float, vec3<float>> pointDistanceLineSegment(vec3<float> v, vec3<float> w, vec3<float> p) {
			auto res = pointDistanceLineSegmentSquared(v, w, p);
			res.first = math::sqrt_approx(res.first); // TODO: this might not be ok :D
			return res;
		}

		template<class P>
		inline bool pointInTriangle(const P& point, const P& t1, const P& t2, const P& t3) {
			typedef decltype(point.x) Scalar;
			P v0, v1, v2;
			v0.x = t3.x - t1.x;
			v0.y = t3.y - t1.y;

			v1.x = t2.x - t1.x;
			v1.y = t2.y - t1.y;

			v2.x = point.x - t1.x;
			v2.y = point.y - t1.y;

			Scalar dot00 = v0.x * v0.x + v0.y * v0.y;
			Scalar dot01 = v0.x * v1.x + v0.y * v1.y;
			Scalar dot02 = v0.x * v2.x + v0.y * v2.y;
			Scalar dot11 = v1.x * v1.x + v1.y * v1.y;
			Scalar dot12 = v1.x * v2.x + v1.y * v2.y;

			// Compute barycentric coordinates
			Scalar invDenom = Scalar(1) / (dot00 * dot11 - dot01 * dot01);
			Scalar u = (dot11 * dot02 - dot01 * dot12) * invDenom;
			Scalar v = (dot00 * dot12 - dot01 * dot02) * invDenom;

			// Check if point is in triangle
			return (u > Scalar(0)) & (v > Scalar(0)) & (u + v < Scalar(1));
		}

		template<class P>
		inline bool pointLeftOfLine(const P& point, const P& t1, const P& t2) {
			return (t2.x - t1.x) * (point.y - t1.y) - (t2.y - t1.y) * (point.x - t1.x) > (decltype(point.x))(0);
		}

		template<typename T>
		inline auto lineSegmentIntersectionPoint(rynx::vec3<T> p1, rynx::vec3<T> p2, rynx::vec3<T> q1, rynx::vec3<T> q2) {
			struct Result {
				operator bool() const { return m_intersected; }
				rynx::vec3<T> point() const { return m_point; }

				rynx::vec3<T> m_point;
				bool m_intersected = false;
			};

			rynx::vec3<T> s = q2 - q1;
			rynx::vec3<T> r = p2 - p1;
			rynx::vec3<T> qp = q1 - p1;
			
			auto rs = r.cross2d(s);
			if (rs == 0)
				return Result({ T(), false });

			auto u = qp.cross2d(r) / rs;
			auto t = qp.cross2d(s) / rs;
			return Result({ p1 + r * t, bool((t >= 0) & (t <= 1) & (u >= 0) & (u <= 1)) });
		}
	}
}