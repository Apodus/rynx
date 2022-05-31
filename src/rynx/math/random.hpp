
#pragma once

#include <cstdint>

namespace rynx {
	namespace serialization {
		template<typename T> struct Serialize;
	}

	namespace math {
		// xorshift32 - feed the result as the next parameter to get next random number.
		//              must start with nonzero value.
		uint32_t rand(uint32_t x);
		uint32_t rand(int32_t x);

		// xorshift64 - feed the result as the next parameter to get next random number.
		//              must start with nonzero value.
		uint64_t rand(uint64_t& state);

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

			template<typename T> T generate() {
				m_state = math::rand(m_state);
				return T(m_state);
			}
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

	namespace serialization {
		template<> struct Serialize<rynx::math::rand64> {
			template<typename IOStream>
			void serialize(const rynx::math::rand64& s, IOStream& writer) {
				writer(s.m_state);
			}

			template<typename IOStream>
			void deserialize(rynx::math::rand64& s, IOStream& reader) {
				reader(s.m_state);
			}
		};
		
		template<typename T> struct Serialize<rynx::math::value_range<T>> {
			template<typename IOStream>
			void serialize(const rynx::math::value_range<T>& s, IOStream& writer) {
				writer(s.begin);
				writer(s.end);
			}

			template<typename IOStream>
			void deserialize(rynx::math::value_range<T>& s, IOStream& reader) {
				reader(s.begin);
				reader(s.end);
			}
		};
	}
}