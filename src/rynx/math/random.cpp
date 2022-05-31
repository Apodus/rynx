#include <rynx/math/random.hpp>

uint32_t rynx::math::rand(uint32_t x) {
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}

uint32_t rynx::math::rand(int32_t x) {
	return rynx::math::rand(static_cast<uint32_t>(x));
}

// xorshift64 - feed the result as the next parameter to get next random number.
//              must start with nonzero value.
uint64_t rynx::math::rand(uint64_t& state) {
	uint64_t x = state;
	x ^= x >> 12; // a
	x ^= x << 25; // b
	x ^= x >> 27; // c
	state = x;
	return x * 0x2545F4914F6CDD1D;
}

rynx::math::rand64::rand64(uint64_t seed) : m_state(seed) {}
	
float rynx::math::rand64::operator()() {
	return operator()(0.0f, 1.0f);
}

float rynx::math::rand64::operator()(float from, float to) {
	m_state = math::rand(m_state);
	float range = to - from;
	return from + range * float(m_state & (uint32_t(~0u) >> 4)) / float(uint32_t(~0u) >> 4);
}

float rynx::math::rand64::operator()(float to) {
	return operator()(0.0f, to);
}

int64_t rynx::math::rand64::operator()(int64_t from, int64_t to) {
	m_state = math::rand(m_state);
	int64_t range = to - from;
	return from + (m_state % range);
}

int64_t rynx::math::rand64::operator()(int64_t to) {
	return operator()(int64_t(0), to);
}

uint64_t rynx::math::rand64::operator()(uint64_t from, uint64_t to) {
	m_state = math::rand(m_state);
	uint64_t range = to - from;
	return from + (m_state % range);
}

uint64_t rynx::math::rand64::operator()(uint64_t to) {
	return operator()(uint64_t(0), to);
}