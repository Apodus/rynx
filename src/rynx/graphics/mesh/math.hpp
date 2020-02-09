
#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/tech/math/math.hpp>

#include <cmath>
#include <utility>
#include <cstdint>

#ifdef max
#undef max
#endif

namespace math {
	template<int N>
	struct Factorial {
		enum { value = N * Factorial<N - 1>::value };
	};

	template<>
	struct Factorial<0> {
		enum { value = 1 };
	};

	// xorshift32 - feed the result as the next parameter to get next random number.
	//              must start with nonzero value.
	inline uint32_t rand(uint32_t x) {
		x ^= x << 13;
		x ^= x >> 17;
		x ^= x << 5;
		return x;
	}

	inline uint32_t rand(int32_t x) {
		return math::rand(static_cast<uint32_t>(x));
	}

	// xorshift64 - feed the result as the next parameter to get next random number.
	//              must start with nonzero value.
	inline uint64_t rand(uint64_t& state) {
		uint64_t x = state;
		x ^= x >> 12; // a
		x ^= x << 25; // b
		x ^= x >> 27; // c
		state = x;
		return x * 0x2545F4914F6CDD1D;
	}

	struct rand64 {
		rand64(uint64_t seed = 0x75892735A374E381) : m_state(seed) {}
		uint64_t m_state;

		uint64_t operator()() {
			m_state = math::rand(m_state);
			return m_state;
		}

		float operator()(float from, float to) {
			m_state = math::rand(m_state);
			float range = to - from;
			return from + range * float(m_state & (uint32_t(~0u) >> 4)) / float(uint32_t(~0u) >> 4);
		}

		float operator()(float to) {
			return operator()(0.0f, to);
		}

		uint64_t operator()(int64_t from, int64_t to) {
			m_state = math::rand(m_state);
			int64_t range = to - from;
			return from + (m_state % range);
		}

		uint64_t operator()(int64_t to) {
			return operator()(0, to);
		}
	};

	template<int iterations, typename T>
	inline T rand(T x) {
		for (int i = 0; i < iterations; ++i)
			x = math::rand(x);
		return x;
	}

	template<class T>
	inline T& rotateXY(T& v, decltype(T().x) radians) {
		decltype(T().x) cosVal = math::cos_approx(radians);
		decltype(T().x) sinVal = math::sin_approx(radians);
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
		return rotatedXY(v, math::sin_approx(radians), math::cos_approx(radians));
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

	inline std::pair<float, vec3<float>> pointDistanceLineSegment(vec3<float> v, vec3<float> w, vec3<float> p) {
		const float l2 = (v - w).length_squared();
		if (l2 == 0.0) return { (p - v).length(), v };
		float bound = (p - v).dot(w - v) / l2;
		bound = bound < 1 ? bound : 1;
		bound = bound > 0 ? bound : 0;
		const vec3<float> projection = v + (w - v) * bound;
		return { (p - projection).length(), projection };
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
		return (u > Scalar(0)) && (v > Scalar(0)) && (u + v < Scalar(1));
	}

	template<class P>
	inline bool pointLeftOfLine(const P& point, const P& t1, const P& t2) {
		return (t2.x - t1.x) * (point.y - t1.y) - (t2.y - t1.y) * (point.x - t1.x) > (decltype(point.x))(0);
	}

	template<class T>
	inline auto crossProduct(const T& t1, const T& t2) {
		return (t1.x * t2.y - t1.y * t2.x);
	}

	template<typename T>
	inline auto lineSegmentIntersectionPoint(const T& p1, const T& p2, const T& q1, const T& q2) {
		struct Result {
			operator bool() const { return m_intersected; }
			T point() const { return m_point; }

			T m_point;
			bool m_intersected = false;
		};

		T s = q2 - q1;
		T r = p2 - p1;

		T qp = q1 - p1;
		auto rs = crossProduct(r, s);
		if (rs == 0)
			return Result({T(), false});

		auto u = crossProduct(qp, r) / rs;
		auto t = crossProduct(qp, s) / rs;
		return Result({ p1 + r * t, bool((t >= 0) & (t <= 1) & (u >= 0) & (u <= 1)) });
	}
}