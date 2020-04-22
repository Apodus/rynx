#pragma once

#include <cmath>
#include <cinttypes>
#include <cstring>

namespace rynx {
	namespace math {
		constexpr float pi = 3.14159f;
		constexpr float pinv = 1.0f / pi;

		inline float atan_approx(float x) {
			// arctan(-x) = -arctan(x)
			// arctan(1 / x) = 0.5 * pi - arctan(x)[x > 0]
			// idea from paper: "Efficient Approximations for the Arctangent Function".
			// Beware of parameters inf & 0.
			int sign_bit = ((x > 0) * 2 - 1);
			float absx = fabsf(x); // we only compute in the positive domain. using identity arctan(-x) above.
			auto func = [](float x) { return pi * 0.25f * x + 0.285f * x * (1.0f - x); };
			return sign_bit * ((absx < 1.0f) ? func(absx) : (pi * 0.5f - func(1.0f / absx)));
		}

		inline float sin(const float& f) {
			return ::sin(f);
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

		// 10-20x faster than std::exp on my cpu (intel 6600k)
		// the absolute error will grow exponentially, obviously.
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

		inline float exp(float x) {
			return std::exp(x);
		}
	}
}