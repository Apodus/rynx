#pragma once

#include <rynx/tech/math/vector.hpp>
#include <cstring>

class matrix4 {
private:
	static constexpr float s_identity[16] = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 0, 1
	};

	float length(float x, float y, float z) { return std::sqrtf(x * x + y * y + z * z) + 0.00000001f; }

public:
	float data[16];

	matrix4() { identity(); }
	matrix4(const matrix4& other) { std::memcpy(data, other.data, sizeof(data)); }

	matrix4& identity() { std::memcpy(data, s_identity, sizeof(s_identity)); return *this; }

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

		float m[16] = {
			sx, ux, -fx, 0.0f,
			sy, uy, -fy, 0.0f,
			sz, uz, -fz, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};
		std::memcpy(data, m, sizeof(m));

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
		
		float m[16] = {
			x, 0.0f, 0.0f, 0.0f,
			0.0f, y, 0.0f, 0.0f,
			A, B, C, -1.0f,
			0.0f, 0.0f, D, 0.0f
		};
		std::memcpy(data, m, sizeof(m));

		return *this;
	}

	matrix4& discardSetOrtho(float left, float right, float bottom, float top, float near, float far) {
		float r_width = 1.0f / (right - left);
		float r_height = 1.0f / (top - bottom);
		float r_depth = 1.0f / (far - near);

		float m[16] = {
			2.0f * (r_width), 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f * (r_height), 0.0f, 0.0f,
			0.0f, 0.0f, -2.0f * (r_depth), 0.0f,
			-(right + left) * r_width, -(top + bottom) * r_height, -(far + near) * r_depth, 1.0f
		};
		std::memcpy(data, m, sizeof(m));

		return *this;
	}

	matrix4& discardSetRotation(float angle, float x, float y, float z) {
		data[3] = 0;
		data[7] = 0;
		data[11] = 0;
		data[12] = 0;
		data[13] = 0;
		data[14] = 0;
		data[15] = 1;
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
		data[0] = x * x * nc + c;
		data[1] = xy * nc + zs;
		data[2] = zx * nc - ys;
		data[4] = xy * nc - zs;
		data[5] = y * y * nc + c;
		data[6] = yz * nc + xs;
		data[8] = zx * nc + ys;
		data[9] = yz * nc - xs;
		data[10] = z * z * nc + c;
		return *this;
	}

	matrix4& operator = (const matrix4& other) {
		memcpy(data, other.data, sizeof(data));
		return *this;
	}

	matrix4& storeMultiply(const matrix4& a, const matrix4& b) {
		float m[16];
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				float sum = 0.0f;
				for (int k = 0; k < 4; ++k) {
					sum += a.data[4 * k + i] * b.data[4 * j + k];
				}
				m[j * 4 + i] = sum;
			}
		}
		std::memcpy(data, m, sizeof(m));
		return *this;
	}

	matrix4& operator *=(const matrix4& other) {
		float m[16];
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				float sum = 0.0f;
				for (int k = 0; k < 4; ++k) {
					sum += data[4 * k + i] * other[4 * j + k];
				}
				m[j * 4 + i] = sum;
			}
		}
		std::memcpy(data, m, sizeof(m));
		return *this;
	}

	matrix4& translate(vec3<float>& out) { return translate(out.x, out.y, out.z); }

	matrix4& translate(float dx, float dy, float dz) {
		for (int i = 0; i < 4; ++i) {
			data[i + 12] += data[i] * dx + data[i + 4] * dy + data[i + 8] * dz;
		}
		return *this;
	}

	matrix4& scale(float scale) { return this->scale(scale, scale, scale); }

	matrix4& scale(float dx, float dy, float dz) {
		matrix4 scaleMatrix;
		scaleMatrix[0] = dx;
		scaleMatrix[5] = dy;
		scaleMatrix[10] = dz;
		return *this *= scaleMatrix;
	}

	matrix4& scale(const vec3<float>& v) {
		return scale(v.x, v.y, v.z);
	}

	matrix4& discardSetScale(float v) { return discardSetScale(v, v, v); }
	matrix4& discardSetScale(float x, float y, float z) {
		identity();
		data[0] = x;
		data[5] = y;
		data[10] = z;
		return *this;
	}

	matrix4& discardSetTranslate(float x, float y, float z) {
		identity();
		data[12] = x;
		data[13] = y;
		data[14] = z;
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

	float operator [] (size_t index) const {
		return data[index];
	}

	float& operator [] (size_t index) {
		return data[index];
	}

	vec3<float> operator *(const vec3<float>& v) const {
		float tmp[3];
		for (int i = 0; i < 3; ++i) {
			float sum = data[i * 4 + 0] * v.x;
			sum += data[i * 4 + 1] * v.y;
			sum += data[i * 4 + 2] * v.z;
			sum += data[i * 4 + 3];
			tmp[i] = sum;
		}
		return vec3<float>(tmp[0], tmp[1], tmp[2]);
	}
};