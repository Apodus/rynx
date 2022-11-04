#pragma once

#ifdef _WIN32
#include <intrin.h> // for bitscanforward
#endif
#include <cstdint>

#ifdef _WIN32
#pragma intrinsic(_BitScanForward64)
inline uint64_t findFirstSetBit(uint64_t source) noexcept {
	unsigned long out = 0;
	_BitScanForward64(&out, source);
	return out;
}
#else
inline uint64_t findFirstSetBit(uint64_t source) noexcept {
	static_assert(sizeof(unsigned long) == sizeof(uint64_t), "unsigned long is not 64bit on this system. rip. you probably need to use __builtin_ctzll instead.");
	return __builtin_ctzl(source);
}
#endif


#ifdef _WIN32
#define rynx_restrict __restrict
#else
#define rynx_restrict __restrict__
#endif
