#pragma once

#include <rynx/system/assert.hpp>

#include <cmath>
#include <limits>

template <class T>
struct vec3 {
	vec3(const vec3& other) : x(other.x), y(other.y), z(other.z) {}
	vec3(T x = 0, T y = 0, T z = 0) : x(x), y(y), z(z) {}
	
	template<typename U> explicit operator vec3<U>() const { return vec3<U>(static_cast<U>(x), static_cast<U>(y), static_cast<U>(z)); }

	vec3& normalizeAccurate() { T l = lengthAccurate() + std::numeric_limits<float>::epsilon(); *this /= l; return *this; }
	vec3& normalizeApprox() { T l = lengthApprox() + std::numeric_limits<float>::epsilon(); *this /= l; return *this; }
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

	vec3 cross(const vec3<T>& other) const { return vec3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x); }
	T dot(const vec3<T>& other) const { return x * other.x + y * other.y + z * other.z; }
	
	T lengthAccurate() const { return std::sqrtf(lengthSquared()); }
	T lengthApprox() const { return lengthAccurate(); } // todo: implement.
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

	vec4& normalizeAccurate() { T l = lengthAccurate(); *this /= l; return *this; }
	vec4& normalizeApprox() { T l = lengthApprox(); *this /= l; return *this; }
	vec4& operator*=(T scalar) { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
	vec4& operator/=(T scalar) { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }
	vec4& operator+=(const vec4& other) { x += other.x; y += other.y; z += other.z; w += other.w; return *this; }
	vec4& operator-=(const vec4& other) { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }
	vec4 operator+(const vec4& other) const { return vec4(x + other.x, y + other.y, z + other.z, w + other.w); }
	vec4 operator-(const vec4& other) const { return vec4(x - other.x, y - other.y, z - other.z, w - other.w); }
	vec4 operator*(const vec4& other) const { return vec4(x * other.x, y * other.y, z * other.z, w * other.w); }
	vec4 operator*(T scalar) const { return vec4(x * scalar, y * scalar, z * scalar, w * scalar); }
	vec4 operator/(T scalar) const { return vec4(x / scalar, y / scalar, z / scalar, w / scalar); }
	// vec4 operator-() const { return vec4(-x, -y, -z, -w); }

	bool operator != (const vec4& other) { return (x != other.x) | (y != other.y) | (z != other.z) | (w != other.w); }
	bool operator == (const vec4& other) { return !((*this) != other); }

	vec4& set(T xx, T yy, T zz, T ww) { x = xx; y = yy; z = zz; w = ww; return *this; }
	T dot(const vec4<T>& other) const { return x * other.x + y * other.y + z * other.z + w * other.w; }

	T lengthAccurate() const { return std::sqrtf(lengthSquared()); }
	T lengthApprox() const { return lengthAccurate(); } // todo: implement.
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