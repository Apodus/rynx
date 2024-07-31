
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

void rynx::compression::rangeencoding::model::node::update(int symbol, int learnRate) {
	m_weights[symbol] += learnRate;
	m_sum += learnRate;
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

std::pair<std::span<const int>, int> rynx::compression::rangeencoding::model::predict(int previous_symbol) {
	if (m_layer1[previous_symbol].has_sufficient_data()) {
		return { m_layer1[previous_symbol].predict(), m_layer1[previous_symbol].m_sum };
	}
	return { m_layer0.predict(), m_layer0.m_sum };
}

void rynx::compression::rangeencoding::model::update(int symbol, int learnRate) {
	m_layer0.update(symbol, learnRate);
	m_layer1[prev_symbol].update(symbol, learnRate);
	prev_symbol = symbol;
}

void rynx::compression::rangeencoding::model::update_custom(int node, int symbol, int rate) {
	m_layer1[node].update(symbol, rate);
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



namespace {
	void init_image_model(rynx::compression::rangeencoding::model& m) {
		constexpr int n0 = 256;
		constexpr int n1 = 32;
		constexpr int n2 = 16;

		for (int i = 0; i < int(m.m_layer1.size()); ++i) {
			m.m_layer1[i].m_weights[i] += n0;
			m.m_layer1[i].m_sum += n0;

			if (i > 0) {
				m.m_layer1[i].m_weights[i - 1] += n1;
				m.m_layer1[i].m_sum += n1;
			}

			if (i > 1) {
				m.m_layer1[i].m_weights[i - 2] += n2;
				m.m_layer1[i].m_sum += n2;
			}

			if (i < int(m.m_layer1[i].m_weights.size()) - 2) {
				m.m_layer1[i].m_weights[i + 1] += n1;
				m.m_layer1[i].m_sum += n1;
			}

			if (i < int(m.m_layer1[i].m_weights.size()) - 3) {
				m.m_layer1[i].m_weights[i + 2] += n2;
				m.m_layer1[i].m_sum += n2;
			}
		}
	}
}

rynx::compression::image_coder::WeightDistribution rynx::compression::image_coder::combine(rynx::compression::image_coder::WeightDistribution p1, rynx::compression::image_coder::WeightDistribution p2) {
	int sum = 0;
	for (int i = 0; i < int(combineStorage.size()); ++i) {
		int v = p1.first[i] + p2.first[i];
		combineStorage[i] = v;
		sum += v;
	}
	return WeightDistribution{ combineStorage, sum };
};

std::vector<char> rynx::compression::image_coder::encode(int width, int height, unsigned char* data) {
	auto get_pixel = [data, width](int x, int y) { return data + (y * width + x) * 4; };

	rynx::compression::rangeencoding::range compressor_m;

	{
		rynx::compression::rangeencoding::model m_r(256 / lossRatio);
		rynx::compression::rangeencoding::model m_g(256 / lossRatio);
		rynx::compression::rangeencoding::model m_b(256 / lossRatio);
		rynx::compression::rangeencoding::model m_a(256 / lossRatio);

		init_image_model(m_r);
		init_image_model(m_g);
		init_image_model(m_b);
		init_image_model(m_a);

		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				int r = get_pixel(x, y)[0] / lossRatio;
				int g = get_pixel(x, y)[1] / lossRatio;
				int b = get_pixel(x, y)[2] / lossRatio;
				int a = get_pixel(x, y)[3] / lossRatio;

				if (x > 0 && y > 0) {
					compressor_m.encode(combine(m_a.predict(), m_a.predict(get_pixel(x, y - 1)[3] / lossRatio)), a);
					if (a >= 2) {
						compressor_m.encode(combine(m_r.predict(), m_r.predict(get_pixel(x, y - 1)[0] / lossRatio)), r);
						compressor_m.encode(combine(m_g.predict(), m_g.predict(get_pixel(x, y - 1)[1] / lossRatio)), g);
						compressor_m.encode(combine(m_b.predict(), m_b.predict(get_pixel(x, y - 1)[2] / lossRatio)), b);
					}
				}
				else {
					if (y > 0) {
						compressor_m.encode(m_a.predict(get_pixel(x, y - 1)[3] / lossRatio), a);
						if (a >= 2) {
							compressor_m.encode(m_r.predict(get_pixel(x, y - 1)[0] / lossRatio), r);
							compressor_m.encode(m_g.predict(get_pixel(x, y - 1)[1] / lossRatio), g);
							compressor_m.encode(m_b.predict(get_pixel(x, y - 1)[2] / lossRatio), b);
						}
					}
					else {
						compressor_m.encode(m_a.predict(), a);
						if (a >= 2) {
							compressor_m.encode(m_r.predict(), r);
							compressor_m.encode(m_g.predict(), g);
							compressor_m.encode(m_b.predict(), b);
						}
					}
				}

				if (y != 0) {
					m_a.update_custom(get_pixel(x, y - 1)[3] / lossRatio, a, learnRate);
					if (a >= 2) {
						m_r.update_custom(get_pixel(x, y - 1)[0] / lossRatio, r, learnRate);
						m_g.update_custom(get_pixel(x, y - 1)[1] / lossRatio, g, learnRate);
						m_b.update_custom(get_pixel(x, y - 1)[2] / lossRatio, b, learnRate);
					}
				}

				if (x != 0) {
					m_a.update(a, learnRate);
					if (a >= 2) {
						m_r.update(r, learnRate);
						m_g.update(g, learnRate);
						m_b.update(b, learnRate);
					}
				}

				m_a.prev_symbol = a;
				m_r.prev_symbol = r;
				m_g.prev_symbol = g;
				m_b.prev_symbol = b;
			}

			m_r.normalize();
			m_g.normalize();
			m_b.normalize();
			m_a.normalize();
		}

		compressor_m.encode(m_r.predict(), int(m_r.m_layer0.m_weights.size()) - 1);
		compressor_m.encode(m_r.predict(), int(m_r.m_layer0.m_weights.size()) - 1);
		compressor_m.flush();
	}

	return compressor_m.extract_result();
}

unsigned char* rynx::compression::image_coder::decode(int width, int height, std::vector<char>&& encoded) {
	rynx::compression::rangeencoding::range decode(std::move(encoded));

	rynx::compression::rangeencoding::model m_r(256 / lossRatio);
	rynx::compression::rangeencoding::model m_g(256 / lossRatio);
	rynx::compression::rangeencoding::model m_b(256 / lossRatio);
	rynx::compression::rangeencoding::model m_a(256 / lossRatio);

	init_image_model(m_r);
	init_image_model(m_g);
	init_image_model(m_b);
	init_image_model(m_a);

	unsigned char* data = new unsigned char[width * height * 4];
	auto get_pixel = [data, width](int x, int y) { return data + (y * width + x) * 4; };

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			int r = 0, g = 0, b = 0, a = 0;

			if (x > 0 && y > 0) {
				a = decode.decode(combine(m_a.predict(), m_a.predict(get_pixel(x, y - 1)[3] / lossRatio)));
				if (a >= 2) {
					r = decode.decode(combine(m_r.predict(), m_r.predict(get_pixel(x, y - 1)[0] / lossRatio)));
					g = decode.decode(combine(m_g.predict(), m_g.predict(get_pixel(x, y - 1)[1] / lossRatio)));
					b = decode.decode(combine(m_b.predict(), m_b.predict(get_pixel(x, y - 1)[2] / lossRatio)));
				}
			}
			else {
				if (y > 0) {
					a = decode.decode(m_a.predict(get_pixel(x, y - 1)[3] / lossRatio));
					if (a >= 2) {
						r = decode.decode(m_r.predict(get_pixel(x, y - 1)[0] / lossRatio));
						g = decode.decode(m_g.predict(get_pixel(x, y - 1)[1] / lossRatio));
						b = decode.decode(m_b.predict(get_pixel(x, y - 1)[2] / lossRatio));
					}
				}
				else {
					a = decode.decode(m_a.predict());
					if (a >= 2) {
						r = decode.decode(m_r.predict());
						g = decode.decode(m_g.predict());
						b = decode.decode(m_b.predict());
					}
				}
			}

			auto* pixel = get_pixel(x, y);
			pixel[0] = r * lossRatio;
			pixel[1] = g * lossRatio;
			pixel[2] = b * lossRatio;
			pixel[3] = a * lossRatio;

			if (y > 0) {
				m_a.update_custom(get_pixel(x, y - 1)[3] / lossRatio, a, learnRate);
				if (a >= 2) {
					m_r.update_custom(get_pixel(x, y - 1)[0] / lossRatio, r, learnRate);
					m_g.update_custom(get_pixel(x, y - 1)[1] / lossRatio, g, learnRate);
					m_b.update_custom(get_pixel(x, y - 1)[2] / lossRatio, b, learnRate);
				}
			}

			if (x > 0) {
				m_a.update(a, learnRate);
				if (a >= 2) {
					m_r.update(r, learnRate);
					m_g.update(g, learnRate);
					m_b.update(b, learnRate);
				}
			}

			m_a.prev_symbol = a;
			m_r.prev_symbol = r;
			m_g.prev_symbol = g;
			m_b.prev_symbol = b;
		}

		m_r.normalize();
		m_g.normalize();
		m_b.normalize();
		m_a.normalize();
	}

	// apply gradient
	int fillPoint = lossRatio;
	while (fillPoint > 1) {
		for (int y = 1; y < height - 1; ++y) {
			for (int x = 1; x < width - 1; ++x) {
				auto* pixel = get_pixel(x, y);

				auto* up = get_pixel(x, y);
				auto* down = get_pixel(x, y);
				auto* left = get_pixel(x, y);
				auto* right = get_pixel(x, y);

				auto correct = [fillPoint, pixel, up, down, left, right](int channel) {
					unsigned char v = pixel[channel] & ~(fillPoint - 1);
					unsigned char u = up[channel] & ~(fillPoint - 1);
					unsigned char d = down[channel] & ~(fillPoint - 1);
					unsigned char l = left[channel] & ~(fillPoint - 1);
					unsigned char r = right[channel] & ~(fillPoint - 1);
					bool has_higher_neighbour = (u > v) | (d > v) | (l > v) | (r > v);
					pixel[channel] = v + (fillPoint >> 1);
					};

				correct(0);
				correct(1);
				correct(2);
				correct(3);
			}
		}

		fillPoint >>= 1;
	}

	// apply half noise
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			auto* pixel = get_pixel(x, y);
			pixel[0] = std::min(255, pixel[0] + rand() % (lossRatio / 2));
			pixel[1] = std::min(255, pixel[1] + rand() % (lossRatio / 2));
			pixel[2] = std::min(255, pixel[2] + rand() % (lossRatio / 2));
			pixel[3] = std::min(255, pixel[3] + rand() % (lossRatio / 2));
		}
	}

	return data;
}
