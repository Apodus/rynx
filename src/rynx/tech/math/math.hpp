#pragma once

#include <cmath>
#include <cinttypes>
#include <cstring>

namespace math {
	constexpr float PI_float = 3.14159f;
	constexpr float PI_inv_float = 1.0f / PI_float;

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

		float operator()() {
			return operator()(0.0f, 1.0f);
		}

		float operator()(float from, float to) {
			m_state = math::rand(m_state);
			float range = to - from;
			return from + range * float(m_state & (uint32_t(~0u) >> 4)) / float(uint32_t(~0u) >> 4);
		}

		float operator()(float to) {
			return operator()(0.0f, to);
		}

		int64_t operator()(int64_t from, int64_t to) {
			m_state = math::rand(m_state);
			int64_t range = to - from;
			return from + (m_state % range);
		}

		int64_t operator()(int64_t to) {
			return operator()(int64_t(0), to);
		}

		uint64_t operator()(uint64_t from, uint64_t to) {
			m_state = math::rand(m_state);
			uint64_t range = to - from;
			return from + (m_state % range);
		}

		uint64_t operator()(uint64_t to) {
			return operator()(uint64_t(0), to);
		}
	};

	inline float sin(const float& f) {
		return ::sin(f);
	}

	inline float sin_approx(float in) {
		return std::sin(in);
	}

	inline float cos_approx(float x) {
		return std::cos(x);
		// return sin_approx(x + PI_float * 0.5f);
	}

	inline float atan_approx(float x) {
		// arctan(-x) = -arctan(x)
		// arctan(1 / x) = 0.5 * pi - arctan(x)[x > 0]
		// idea from paper: "Efficient Approximations for the Arctangent Function".
		// Beware of parameters inf & 0.
		int sign_bit = ((x > 0) * 2 - 1);
		float absx = fabsf(x); // we only compute in the positive domain. using identity arctan(-x) above.
		auto func = [](float x) { return PI_float * 0.25f * x + 0.285f * x * (1.0f - x); };
		return sign_bit * ((absx < 1.0f) ? func(absx) : (PI_float * 0.5f - func(1.0f / absx)));
	}

	inline float cos(float f) {
		return ::cos(f);
	}

	inline float abs(float f) {
		return std::abs(f);
	}

	inline float sqrt_inverse_approx(float val) {
		float x2 = val * 0.5f;
		int32_t i;

		std::memcpy(&i, &val, sizeof(val));
		i = 0x5f3759df - (i >> 1);

		float y;
		std::memcpy(&y, &i, sizeof(val));
		y *= 1.5f - x2 * y * y;
		y *= 1.5f - x2 * y * y;
		return y;
	}

		// approximation, good enough: never over estimates. max error ~3%.
	inline float sqrt_approx(float val) {
		/*
		int32_t x;
		std::memcpy(&x, &val, sizeof(val));
		x = (1 << 29) + (x >> 1) - (1 << 22) - 0x4C000;
		float result;
		std::memcpy(&result, &x, sizeof(x));
		return result;
		*/

		return 1.0f / sqrt_inverse_approx(val);
	}

	inline constexpr double exp_approx(double v) {
		double x = 1.0 + v / 1024.0;
		x *= x; x *= x; x *= x; x *= x;
		x *= x; x *= x; x *= x; x *= x;
		x *= x; x *= x;
		return x;
	}

	inline float exp_approx(float x) {
		x = 1.0f + x / 1024.0f;
		x *= x; x *= x; x *= x; x *= x;
		x *= x; x *= x; x *= x; x *= x;
		x *= x; x *= x;
		return x;
	}
}