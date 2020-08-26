
#pragma once

#include <cstdint>

namespace rynx {
	namespace math {
		// xorshift32 - feed the result as the next parameter to get next random number.
		//              must start with nonzero value.
		uint32_t rand(uint32_t x);
		uint32_t rand(int32_t x);

		// xorshift64 - feed the result as the next parameter to get next random number.
		//              must start with nonzero value.
		inline uint64_t rand(uint64_t& state);

		struct rand64 {
			rand64(uint64_t seed = 0x75892735A374E381);
			uint64_t m_state;

			float operator()();
			float operator()(float from, float to);
			float operator()(float to);
			int64_t operator()(int64_t from, int64_t to);
			int64_t operator()(int64_t to);
			uint64_t operator()(uint64_t from, uint64_t to);
			uint64_t operator()(uint64_t to);
		};

		// TODO: where does this belong.
		template<typename T>
		struct value_range {
			value_range(T b, T e) : begin(b), end(e) {}
			value_range() = default;

			T begin;
			T end;

			T operator()(float v) const {
				return static_cast<T>(begin * (1.0f - v) + end * v);
			}
		};
	}
}