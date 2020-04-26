#pragma once

#include <rynx/math/vector.hpp>

namespace rynx {
	class matrix4 {
	public:
		union {
			float m[16];
			vec4f row[4];
		};

	public:

		matrix4() {}
		matrix4(const matrix4& other);

		matrix4& identity();
		matrix4& invert();
		matrix4 compute_inverse() const;
		
		matrix4& discardSetLookAt(
			vec3f eye_position,
			vec3f eye_direction,
			vec3f up);

		matrix4& discardSetFrustum(
			float left, float right,
			float bottom, float top,
			float near, float far);

		matrix4& discardSetOrtho(float left, float right, float bottom, float top, float near, float far);
		matrix4& discardSetRotation(float angle, float x, float y, float z);
		matrix4& discardSetRotation_z(float angle);
		matrix4& operator = (const matrix4& other);

		matrix4& storeMultiply(const matrix4& a, const matrix4& b);
		matrix4& operator *=(const matrix4& other);
		matrix4 operator * (const matrix4& other) const;
		rynx::vec4f operator * (vec4f other) const;

		matrix4& translate(vec3<float> out);
		matrix4& translate(float dx, float dy, float dz);

		matrix4& scale(float scale);
		matrix4& scale(float dx, float dy, float dz);
		matrix4& scale(vec3<float> v);

		matrix4& discardSetScale(float v);
		matrix4& discardSetScale(float x, float y, float z);
		matrix4& discardSetTranslate(float x, float y, float z);
		matrix4& discardSetTranslate(const vec3<float>& v);

		matrix4& rotate(float radians, float x, float y, float z);
		matrix4& rotate_2d(float radians);

		float operator [] (size_t index) const;
		float& operator [] (size_t index);

		vec3<float> operator *(const vec3<float>& v) const;
	};
}