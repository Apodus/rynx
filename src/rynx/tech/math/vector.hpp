#pragma once


#include <intrin.h>
#include <rynx/system/assert.hpp>
#include <rynx/tech/math/math.hpp>

#include <cinttypes>
#include <cmath>
#include <limits>

template <class T>
struct vec3 {
	vec3(const vec3& other) : x(other.x), y(other.y), z(other.z) {}
	vec3(T x = 0, T y = 0, T z = 0) : x(x), y(y), z(z) {}
	
	template<typename U> explicit operator vec3<U>() const { return vec3<U>(static_cast<U>(x), static_cast<U>(y), static_cast<U>(z)); }

	vec3 normal() { T l = length() + std::numeric_limits<float>::epsilon(); return *this * (1.0f / l); }
	vec3& normalize() { T l = length() + std::numeric_limits<float>::epsilon(); *this *=  1.0f / l; return *this; }
	
	vec3& operator*=(T scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }
	vec3& operator/=(T scalar) { x /= scalar; y /= scalar; z /= scalar; return *this; }
	
	vec3& operator+=(vec3<T> other) { x += other.x; y += other.y; z += other.z; return *this; }
	vec3& operator-=(vec3<T> other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
	vec3& operator*=(vec3<T> other) { x *= other.x; y *= other.y; z *= other.z; return *this; }
	vec3& operator/=(vec3<T> other) { x /= other.x; y /= other.y; z /= other.z; return *this; }

	vec3 operator+(const vec3<T>& other) const { return vec3(x + other.x, y + other.y, z + other.z); }
	vec3 operator-(const vec3<T>& other) const { return vec3(x - other.x, y - other.y, z - other.z); }
	vec3 operator*(const vec3<T>& other) const { return vec3(x * other.x, y * other.y, z * other.z); }
	
	vec3 operator*(const T& scalar) const { return vec3(x * scalar, y * scalar, z * scalar); }
	vec3 operator/(const T& scalar) const { return vec3(x / scalar, y / scalar, z / scalar); }
	vec3 operator-() const { return vec3(-x, -y, -z); }
	
	bool operator != (const vec3& other) { return (x != other.x) | (y != other.y) | (z != other.z); }
	bool operator == (const vec3& other) { return !((*this)!=other); }

	vec3& set(T xx, T yy, T zz) { x = xx; y = yy; z = zz; return *this; }
	vec3 cross(const vec3& other) const { return vec3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x); }

	vec3 normal2d() const { return vec3(-y, +x, 0); }
	float cross2d(const vec3& other) const { return x * other.y - y * other.x; }
	T dot(const vec3& other) const { return x * other.x + y * other.y + z * other.z; }
	
	T length() const { return math::sqrt_approx(lengthSquared()); }
	T lengthSquared() const { return dot(*this); }

	T x;
	T y;
	T z;
};

template <class T>
struct vec4 {
	vec4(const vec4& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
	constexpr vec4(T x = 0, T y = 0, T z = 0, T w = 0) : x(x), y(y), z(z), w(w) {}

	template<typename U> explicit operator vec4<U>() const { return vec4<U>(static_cast<U>(x), static_cast<U>(y), static_cast<U>(z), static_cast<U>(w)); }

	vec4 normal() { T l = 1.0f / length(); return *this * l; }
	vec4& normalize() { T l = 1.0f / length(); *this *= l; return *this; }
	
	vec4& operator*=(T scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
	vec4& operator/=(T scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }
	vec4& operator+=(const vec4& other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
	vec4& operator-=(const vec4& other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }
	
	vec4 operator+(const vec4& other) const { return vec4(x + other.x, y + other.y, z + other.z, w + other.w); }
	vec4 operator-(const vec4& other) const { return vec4(x - other.x, y - other.y, z - other.z, w - other.w); }
	vec4 operator*(const vec4& other) const { return vec4(x * other.x, y * other.y, z * other.z, w * other.w); }
	
	vec4 operator*(T scalar) const { return vec4(x * scalar, y * scalar, z * scalar, w * scalar); }
	vec4 operator/(T scalar) const { return vec4(x / scalar, y / scalar, z / scalar, w / scalar); }
	vec4 operator-() const { return vec4(-x, -y, -z, -w); }

	bool operator != (const vec4& other) { return (x != other.x) | (y != other.y) | (z != other.z) | (w != other.w); }
	bool operator == (const vec4& other) { return !((*this) != other); }

	vec4& set(T xx, T yy, T zz, T ww) { x = xx; y = yy; z = zz; w = ww; return *this; }
	T dot(const vec4<T>& other) const { return x * other.x + y * other.y + z * other.z + w * other.w; }
	T length() const { return math::sqrt_approx(lengthSquared()); }
	T lengthSquared() const { return dot(*this); }

	const T& operator [](int index) const {
		rynx_assert(index >= 0 && index < 4, "index out of bounds");
		return data[index];
	}

	T& operator [](int index) {
		rynx_assert(index >= 0 && index < 4, "index out of bounds");
		return data[index];
	}

#pragma warning (disable : 4201) // language extension used, anonymous structs

	union {
		struct {
			T x, y, z, w;
		};
		struct {
			T r, g, b, a;
		};
		struct {
			T left, right, top, bottom;
		};
		T data[4];
	};
};

struct floats4 {

#pragma warning (disable : 4201)
	union {
		struct {
			float x, y, z, w;
		};
		struct {
			float r, g, b, a;
		};
		struct {
			float left, right, top, bottom;
		};
		float data[4];
	};

	floats4(float x, const vec3<float>& other) : x(x), y(other.x), z(other.y), w(other.z) {}
	floats4(const vec3<float>& other, float w) : x(other.x), y(other.y), z(other.z), w(w) {}
	floats4(const floats4& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
	constexpr floats4(float x = 0, float y = 0, float z = 0, float w = 0) : x(x), y(y), z(z), w(w) {}

	floats4 operator*(float scalar) const { return floats4(x * scalar, y * scalar, z * scalar, w * scalar); }
	floats4 operator/(float scalar) const { return operator*(1.0f / scalar); }

	floats4 operator/(const floats4& other) const { return floats4(x / other.x, y / other.y, z / other.z, w / other.w); }
	floats4 operator*(const floats4& other) const { return floats4(x * other.x, y * other.y, z * other.z, w * other.w); }
	floats4 operator+(const floats4& other) const { return floats4(x + other.x, y + other.y, z + other.z, w + other.w); }
	floats4 operator-(const floats4& other) const { return floats4(x - other.x, y - other.y, z - other.z, w - other.w); }
	floats4 operator-() { return floats4(-x, -y, -z, -w); }

	floats4& operator+=(const floats4& other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
	floats4& operator-=(const floats4& other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }


	const float& operator [](int index) const {
		rynx_assert(index >= 0 && index < 4, "index out of bounds");
		return data[index];
	}

	float& operator [](int index) {
		rynx_assert(index >= 0 && index < 4, "index out of bounds");
		return data[index];
	}
};

template<typename T> struct vec4;

template<>
struct vec4<float> {
	__m128 xmm;

	vec4() : vec4(0.0f) {}
	vec4(__m128 v) : xmm(v) {}
	vec4(float v) : xmm(_mm_set_ps(0, v, v, v)) {}
	vec4(float x, float y, float z, float w = 0.0f) : xmm(_mm_set_ps(w, z, y, x)) {}
	vec4(vec3<float> const& other, float w = 0.0f) : xmm(_mm_set_ps(w, other.z, other.y, other.x)) {}

	operator __m128() const { return xmm; }

	vec4 operator * (const vec4& v) const { return vec4(_mm_mul_ps(xmm, v.xmm)); }
	vec4 operator + (const vec4& v) const { return vec4(_mm_add_ps(xmm, v.xmm)); }
	vec4 operator - (const vec4& v) const { return vec4(_mm_sub_ps(xmm, v.xmm)); }
	vec4 operator / (const vec4& v) const { return vec4(_mm_div_ps(xmm, v.xmm)); }

	vec4& operator *= (const vec4& v) { xmm = _mm_mul_ps(xmm, v.xmm); return *this; }
	vec4& operator += (const vec4& v) { xmm = _mm_add_ps(xmm, v.xmm); return *this; }
	vec4& operator -= (const vec4& v) { xmm = _mm_sub_ps(xmm, v.xmm); return *this; }
	vec4& operator /= (const vec4& v) { xmm = _mm_div_ps(xmm, v.xmm); return *this; }

	vec4 operator -() const { return operator *(-1.0f); }

	floats4 to_floats() const {
		floats4 result;
		_mm_storeu_ps(result.data, xmm);
		return result;
	}

	operator floats4() const {
		return to_floats();
	}

	vec4 cross(const vec4& v2) const {
		__m128 t1 = _mm_shuffle_ps(xmm, xmm, 0xc9);
		__m128 t2 = _mm_shuffle_ps(xmm, xmm, 0xd2);
		__m128 t3 = _mm_shuffle_ps(v2.xmm, v2.xmm, 0xd2);
		__m128 t4 = _mm_shuffle_ps(v2.xmm, v2.xmm, 0xc9);
		__m128 t5 = _mm_mul_ps(t1, t3);
		__m128 t6 = _mm_mul_ps(t2, t4);
		return _mm_sub_ps(t5, t6);
	}

	vec4 operator * (float s) const {
		const __m128 scalar = _mm_set1_ps(s);
		return _mm_mul_ps(xmm, scalar);
	}
	
	vec4& operator *= (float s) {
		const __m128 scalar = _mm_set1_ps(s);
		return operator *= (scalar);
	}
	
	vec4 operator / (float scalar) const { return operator*(1.0f / scalar); }
	vec4& operator /= (float scalar) { return operator *= (1.0f / scalar); }

	float length() const {
		return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(xmm, xmm, 0xff)));
	}

	float length_squared() const {
		return _mm_cvtss_f32(_mm_dp_ps(xmm, xmm, 0xff));
	}

	vec4 normal() const {
		return _mm_div_ps(xmm, _mm_sqrt_ps(_mm_dp_ps(xmm, xmm, 0xff)));
	}

	vec4& normalize() { xmm = normal().xmm; return *this; }
	
	bool operator != (const vec4& other) {
		__m128i vcmp = _mm_castps_si128(_mm_cmpneq_ps(xmm, other.xmm));
		int32_t test = _mm_movemask_epi8(vcmp);
		return test != 0;
	}
	bool operator == (const vec4& other) { return !((*this) != other); }

	vec4& set(float xx, float yy, float zz, float ww = 0.0f) { xmm = _mm_set_ps(ww, zz, yy, xx); return *this; }
	float dot(const vec4& other) const {
		__m128 r1 = _mm_mul_ps(xmm, other.xmm);
		__m128 shuf = _mm_shuffle_ps(r1, r1, _MM_SHUFFLE(2, 3, 0, 1));
		__m128 sums = _mm_add_ps(r1, shuf);
		shuf = _mm_movehl_ps(shuf, sums);
		sums = _mm_add_ss(sums, shuf);
		return _mm_cvtss_f32(sums);
	}

	float horizontal_add() const {
		__m128 t1 = _mm_movehl_ps(xmm, xmm);
		__m128 t2 = _mm_add_ps(xmm, t1);
		__m128 t3 = _mm_shuffle_ps(t2, t2, 1);
		__m128 t4 = _mm_add_ss(t2, t3);
		return _mm_cvtss_f32(t4);
	}

	vec4 normal2d() const { return vec4(_mm_shuffle_ps(xmm, xmm, _MM_SHUFFLE(3, 3, 0, 1))) * vec4(-1, +1, 0, 0); }
	float cross2d(const vec4& other) const {
		// TODO: this is madness.
		floats4 kek1 = *this;
		floats4 kek2 = other;
		return kek1.x * kek2.y - kek1.y * kek2.x;
	}
};

/*
template <>
struct vec3<float> : public vec4<float> {
	vec3(const vec3& other) = default;
	vec3(float x = 0, float y = 0, float z = 0) { vec4<float>::set(x, y, z, 0.0f); }
	vec3(vec4 v) { xmm = v.xmm; }

	bool operator != (const vec3& other) { return vec4<float>::operator != (other); }
	bool operator == (const vec3& other) { return vec4<float>::operator == (other); }
};
*/

inline vec3<float> operator * (float x, const vec3<float>& other) {
	return other * x;
}

using vec4f = vec4<float>;
using vec3f = vec3<float>;