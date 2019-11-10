#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/tech/math/math.hpp>

#include <cstring>
#include <immintrin.h>
#include <intrin.h>

class matrix4 {
public:
	union {
		float m[16];
		vec4f row[4];
	};

	float length(float x, float y, float z) { return math::sqrt_approx(x * x + y * y + z * z) + 0.00000001f; }

public:

	matrix4() {}
	// matrix4() { identity(); }
	matrix4(const matrix4& other) { std::memcpy(row, other.row, sizeof(row)); }

	matrix4& identity() {
		row[0] = vec4f(1, 0, 0, 0);
		row[1] = vec4f(0, 1, 0, 0);
		row[2] = vec4f(0, 0, 1, 0);
		row[3] = vec4f(0, 0, 0, 1);
		return *this;
	}
	
	static inline __m256 twolincomb_AVX_8(__m256 A01, const matrix4& B)
	{
		__m256 result;
		result = _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x00), _mm256_broadcast_ps(&B.row[0].xmm));
		result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x55), _mm256_broadcast_ps(&B.row[1].xmm)));
		result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xaa), _mm256_broadcast_ps(&B.row[2].xmm)));
		result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xff), _mm256_broadcast_ps(&B.row[3].xmm)));
		return result;
	}

	static void matmult_AVX_8(matrix4& out, const matrix4& A, const matrix4& B)
	{
		_mm256_zeroupper();
		__m256 A01 = _mm256_load_ps(&A.m[0 * 4 + 0]);
		__m256 A23 = _mm256_load_ps(&A.m[2 * 4 + 0]);

		__m256 out01x = twolincomb_AVX_8(A01, B);
		__m256 out23x = twolincomb_AVX_8(A23, B);

		_mm256_store_ps(&out.m[0 * 4 + 0], out01x);
		_mm256_store_ps(&out.m[2 * 4 + 0], out23x);
	}

	matrix4& discardSetLookAt(float eyeX, float eyeY, float eyeZ,
		float centerX, float centerY, float centerZ,
		float upX, float upY, float upZ) {
		float fx = centerX - eyeX;
		float fy = centerY - eyeY;
		float fz = centerZ - eyeZ;

		// Normalize f
		float rlf = 1.0f / length(fx, fy, fz);
		fx *= rlf;
		fy *= rlf;
		fz *= rlf;

		// compute s = f x up (x means "cross product")
		float sx = fy * upZ - fz * upY;
		float sy = fz * upX - fx * upZ;
		float sz = fx * upY - fy * upX;

		// and normalize s
		float rls = 1.0f / length(sx, sy, sz);
		sx *= rls;
		sy *= rls;
		sz *= rls;

		// compute u = s x f
		float ux = sy * fz - sz * fy;
		float uy = sz * fx - sx * fz;
		float uz = sx * fy - sy * fx;

		row[0] = vec4f(sx, ux, -fx, 0.0f);
		row[1] = vec4f(sy, uy, -fy, 0.0f);
		row[2] = vec4f(sz, uz, -fz, 0.0f);
		row[3] = vec4f(0.0f, 0.0f, 0.0f, 1.0f);
		return translate(-eyeX, -eyeY, -eyeZ);
	}

	matrix4& discardSetFrustum(
		float left, float right,
		float bottom, float top,
		float near, float far)
	{
		float r_width = 1.0f / (right - left);
		float r_height = 1.0f / (top - bottom);
		float r_depth = 1.0f / (near - far);
		float x = 2.0f * (near * r_width);
		float y = 2.0f * (near * r_height);
		// float A = 2.0f * ((right + left) * r_width);
		float A = (right + left) * r_width;
		float B = (top + bottom) * r_height;
		float C = (far + near) * r_depth;
		float D = 2.0f * (far * near * r_depth);
		
		float tmp[16] = {
			x, 0.0f, 0.0f, 0.0f,
			0.0f, y, 0.0f, 0.0f,
			A, B, C, -1.0f,
			0.0f, 0.0f, D, 0.0f
		};
		
		std::memcpy(m, tmp, sizeof(m));
		return *this;
	}

	matrix4& discardSetOrtho(float left, float right, float bottom, float top, float near, float far) {
		float r_width = 1.0f / (right - left);
		float r_height = 1.0f / (top - bottom);
		float r_depth = 1.0f / (far - near);

		row[0] = vec4f(2.0f * (r_width), 0.0f, 0.0f, 0.0f);
		row[1] = vec4f(0.0f, 2.0f * (r_height), 0.0f, 0.0f);
		row[2] = vec4f(0.0f, 0.0f, -2.0f * (r_depth), 0.0f);
		row[3] = vec4f(-(right + left) * r_width, -(top + bottom) * r_height, -(far + near) * r_depth, 1.0f);
		return *this;
	}

	matrix4& discardSetRotation(float angle, float x, float y, float z) {
		float s = std::sin(angle);
		float c = std::cos(angle);
		float len = length(x, y, z);
		float recipLen = 1.0f / len;
		x *= recipLen;
		y *= recipLen;
		z *= recipLen;
		float nc = 1.0f - c;
		float xy = x * y;
		float yz = y * z;
		float zx = z * x;
		float xs = x * s;
		float ys = y * s;
		float zs = z * s;
		
		row[0] = vec4f(x * x * nc + c, xy * nc + zs, zx * nc - ys, 0);
		row[1] = vec4f(xy * nc - zs, y * y * nc + c, yz * nc + xs, 0);
		row[2] = vec4f(zx * nc + ys, yz * nc - xs, z * z * nc + c, 0);
		row[3] = vec4f(0, 0, 0, 1);
		return *this;
	}

	matrix4& discardSetRotation_z(float angle) {
		float s = std::sin(angle);
		float c = std::cos(angle);
		row[0] = vec4f(c, s, 0, 0);
		row[1] = vec4f(-s, c, 0, 0);
		row[2] = vec4f(0, 0, 1, 0);
		row[3] = vec4f(0, 0, 0, 1);
		return *this;
	}

	matrix4& operator = (const matrix4& other) {
		row[0] = other.row[0];
		row[1] = other.row[1];
		row[2] = other.row[2];
		row[3] = other.row[3];
		return *this;
	}

	matrix4& storeMultiply(const matrix4& a, const matrix4& b) {
		matrix4 result;
		matmult_AVX_8(result, b, a);
		*this = result;
		return *this;
	}

	matrix4& operator *=(const matrix4& other) {
		return storeMultiply(*this, other);
	}

	matrix4& translate(vec3<float>& out) {
		matrix4 translation_matrix;
		translation_matrix.identity();
		translation_matrix.row[3] = vec4f(out, 1.0f);
		*this *= translation_matrix;
		return *this;
	}

	matrix4& translate(float dx, float dy, float dz) {
		vec3<float> v(dx, dy, dz);
		return translate(v);
	}

	matrix4& scale(float scale) { return this->scale(scale, scale, scale); }
	matrix4& scale(float dx, float dy, float dz) {
		matrix4 scaleMatrix;
		scaleMatrix.identity();
		scaleMatrix[0] = dx;
		scaleMatrix[5] = dy;
		scaleMatrix[10] = dz;
		return *this *= scaleMatrix;
	}

	// TODO:
	matrix4& scale(const vec3<float>& v) {
		return scale(v.x, v.y, v.z);
	}

	matrix4& discardSetScale(float v) { return discardSetScale(v, v, v); }
	matrix4& discardSetScale(float x, float y, float z) {
		row[0] = vec4f(x, 0, 0, 0);
		row[1] = vec4f(0, y, 0, 0);
		row[2] = vec4f(0, 0, z, 0);
		row[3] = vec4f(0, 0, 0, 1);
		return *this;
	}

	matrix4& discardSetTranslate(float x, float y, float z) {
		row[0] = vec4f(1, 0, 0, 0);
		row[1] = vec4f(0, 1, 0, 0);
		row[2] = vec4f(0, 0, 1, 0);
		row[3] = vec4f(x, y, z, 1);
		return *this;
	}

	matrix4& discardSetTranslate(const vec3<float>& v) {
		return discardSetTranslate(v.x, v.y, v.z);
	}

	matrix4& rotate(float radians, float x, float y, float z) {
		matrix4 rotMat;
		rotMat.discardSetRotation(radians, x, y, z);
		return *this *= rotMat;
	}

	matrix4& rotate_2d(float radians) {
		matrix4 rotMat;
		rotMat.discardSetRotation_z(radians);
		return *this *= rotMat;
	}
	
	float operator [] (size_t index) const {
		return m[index];
	}

	float& operator [] (size_t index) {
		return m[index];
	}

	vec3<float> operator *(const vec3<float>& v) const {
#if 0
		float tmp[3];
		for (int i = 0; i < 3; ++i) {
			float sum = m[i * 4 + 0] * v.x;
			sum += m[i * 4 + 1] * v.y;
			sum += m[i * 4 + 2] * v.z;
			sum += m[i * 4 + 3];
			tmp[i] = sum;
		}
		return vec3<float>(tmp[0], tmp[1], tmp[2]);
#else
		// TODO: does this work or not?
		vec4f as_vec(v, 1.0f);
		vec4f vec_res = as_vec * row[0] + as_vec * row[1] + as_vec * row[2] + as_vec * row[3];
		floats4 res = vec_res.to_floats();
		return vec3<float>(res.x, res.y, res.z);
#endif
	}
};