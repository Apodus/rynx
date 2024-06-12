
#include <rynx/std/compression.hpp>
#include <rynx/system/assert.hpp>
#include <zstd.h>

// size_t ZSTD_compress(void* dst, size_t dstCapacity, const void* src, size_t srcSize, int compressionLevel);
// size_t ZSTD_decompress(void* dst, size_t dstCapacity, const void* src, size_t compressedSize);
// ZSTD_getFrameContentSize(compressedBuffer, bufferSize);
// size_t ZSTD_compressBound(size_t srcSize); /*!< maximum compressed size in worst case single-pass scenario */

std::vector<char> rynx::compression::compress(std::span<char> memory, int compressionLevel) {
	size_t target_size = ZSTD_compressBound(memory.size_bytes());
	std::vector<char> result;
	result.resize(target_size);
	size_t compressedSize = ZSTD_compress(result.data(), result.size(), memory.data(), memory.size(), compressionLevel);
	result.resize(compressedSize);
	return result;
}

std::vector<char> rynx::compression::compress(std::span<char> memory) {
	size_t target_size = ZSTD_compressBound(memory.size_bytes());
	std::vector<char> result;
	result.resize(target_size);
	size_t compressedSize = ZSTD_compress(result.data(), result.size(), memory.data(), memory.size(), ZSTD_maxCLevel());
	result.resize(compressedSize);
	return result;
}

std::vector<char> rynx::compression::decompress(std::span<char> memory) {
	if (memory.empty())
		return {};
	size_t expected_decompressed_size = ZSTD_getFrameContentSize(memory.data(), memory.size());
	std::vector<char> result(expected_decompressed_size);
	size_t actual_decompressed_size = ZSTD_decompress(result.data(), result.size(), memory.data(), memory.size());
	rynx_assert(actual_decompressed_size == expected_decompressed_size, "??");
	return result;
}

rynx::compression::rangeencoding::model::model(int numSymbols) {
	m_layer0.init(numSymbols);
	m_layer1.resize(numSymbols, model::node(numSymbols));
}

rynx::compression::rangeencoding::model::node::node(int numSymbols) {
	init(numSymbols);
}

void rynx::compression::rangeencoding::model::node::init(int numSymbols) {
	m_weights.resize(numSymbols + 1, 1);
	m_sum = numSymbols + 1;
}

const std::vector<int>& rynx::compression::rangeencoding::model::node::predict() {
	return m_weights;
}

void rynx::compression::rangeencoding::model::node::update(int symbol) {
	m_weights[symbol] += 2;
	m_sum += 2;
}

bool rynx::compression::rangeencoding::model::node::has_sufficient_data() {
	return m_sum > m_weights.size();
}

std::pair<std::span<const int>, int> rynx::compression::rangeencoding::model::predict() {
	if (m_layer1[prev_symbol].has_sufficient_data()) {
		return { m_layer1[prev_symbol].predict(), m_layer1[prev_symbol].m_sum };
	}
	return { m_layer0.predict(), m_layer0.m_sum };
}

void rynx::compression::rangeencoding::model::update(int symbol) {
	m_layer0.update(symbol);
	m_layer1[prev_symbol].update(symbol);
	prev_symbol = symbol;
}

void rynx::compression::rangeencoding::range::bit_stream::write_bit(int a) {
	current |= (a << current_bits);
	if (++current_bits == 8) {
		m_memory.emplace_back(current);
		current = 0;
		current_bits = 0;
	}
}

int rynx::compression::rangeencoding::range::bit_stream::read_bit() {
	if (current_bits / 8 < m_memory.size()) {
		int bit = (m_memory[current_bits / 8] >> (current_bits & 7)) & 1;
		++current_bits;
		return bit;
	}
	return 0;
}

void rynx::compression::rangeencoding::range::bit_stream::flush() {
	m_memory.emplace_back(current);
}

rynx::compression::rangeencoding::range::range(std::vector<char> memory) {
	stream.m_memory = std::move(memory);
	stream.current_bits = 0;
	stream.current = 0;
	for (int i = 0; i < 64; ++i)
		code_value = (code_value << 1) | stream.read_bit();
}

void rynx::compression::rangeencoding::range::encode(std::pair<std::span<const int>, int> weights_and_sum, int selection) {
	std::span<const int> weights = weights_and_sum.first;
	uint64_t weights_sum = weights_and_sum.second;
	uint64_t value_range = max - min;
	uint64_t unit_range = value_range / weights_sum;

	while (unit_range < 100) {
		stream.write_bit(min >> 63);
		min <<= 1;		
		max = ~uint64_t(0);
		while ((min >> 63) == (max >> 63)) {
			stream.write_bit(min >> 63);
			min <<= 1;
			max = (max << 1) | 1;
		}

		rynx_assert(max > min, "max must be greater than min after correction");
		value_range = max - min;
		unit_range = value_range / weights_sum;
	}

	int weight_sum_until_selection = 0;
	for (int i = 0; i < selection; ++i) {
		weight_sum_until_selection += weights[i];
	}

	max = min + (weight_sum_until_selection + weights[selection]) * unit_range;
	min = min + weight_sum_until_selection * unit_range;

	while ((min >> 63) == (max >> 63)) {
		stream.write_bit(min >> 63);
		min <<= 1;
		max = (max << 1) | 1;
	}
}

int rynx::compression::rangeencoding::range::decode(std::pair<std::span<const int>, int> weights_and_sum) {
	std::span<const int> weights = weights_and_sum.first;
	uint64_t weights_sum = weights_and_sum.second;
	uint64_t value_range = max - min;
	uint64_t unit_range = value_range / weights_sum;

	// renormalize if there's not enough range
	while (unit_range < 100) {
		min <<= 1;
		max = ~uint64_t(0);
		code_value = (code_value << 1) | stream.read_bit();
		
		while ((min >> 63) == (max >> 63)) {
			min <<= 1;
			max = (max << 1) | 1;
			code_value = (code_value << 1) | stream.read_bit();
		}

		value_range = max - min;
		unit_range = value_range / weights_sum;
	}

	uint64_t nextRangeMin = min;
	uint64_t nextRangeMax = min;

	int output_symbol = -1;
	int i = 0;
	while (true) {
		nextRangeMin = nextRangeMax;
		nextRangeMax += weights[i++] * unit_range;

		if (code_value >= nextRangeMin && code_value < nextRangeMax) {
			min = nextRangeMin;
			max = nextRangeMax;
			output_symbol = i - 1;
			break;
		}
	}

	while ((min >> 63) == (max >> 63)) {
		min <<= 1;
		max = (max << 1) | 1;
		code_value = (code_value << 1) | stream.read_bit();
	}

	return output_symbol;
}

void rynx::compression::rangeencoding::range::flush() {
	uint64_t mid = min + ((max - min) >> 1);
	for (int i = 0; i < 64; ++i) {
		stream.write_bit(mid >> 63);
		mid <<= 1;
	}
	stream.flush();
}