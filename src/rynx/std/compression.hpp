
#pragma once

#include <span>
#include <vector>

namespace rynx::compression {
	std::vector<char> decompress(std::span<char> memory);
	std::vector<char> compress(std::span<char> memory);
	std::vector<char> compress(std::span<char> memory, int compressionLevel);



	namespace rangeencoding {
		
		// this model is 1-depth only, provided as an example of the api.
		// you should probably write your own data model that is specific to your data and computational resources available.
		// more complex model = better compression ratio, slower to execute.
		struct model {
			model(int numSymbols);
			struct node {
				node() {};
				node(int numSymbols);
				void init(int numSymbols);
				const std::vector<int>& predict();
				void update(int symbol, int rate);
				bool has_sufficient_data();

				void normalize() {
					if (m_sum > 1000) {
						m_sum = 0;
						for (int& w : m_weights) {
							w = std::max(1, w >> 1);
							m_sum += w;
						}
					}
				}

				int m_sum = 0;
				std::vector<int> m_weights;
			};

			std::pair<std::span<const int>, int> predict();
			std::pair<std::span<const int>, int> predict(int previous_symbol);
			void update(int symbol, int rate);
			void update_custom(int node, int symbol, int rate);

			void normalize() {
				m_layer0.normalize();
				for (auto& layer : m_layer1) {
					layer.normalize();
				}
			}

			int prev_symbol = 0;
			model::node m_layer0;
			std::vector<model::node> m_layer1;
		};

		struct range {
			struct bit_stream {
				void write_bit(int a);
				int read_bit();
				void flush();

				char current = 0;
				int current_bits = 0;
				std::vector<char> m_memory;
			};

			uint64_t min = uint64_t(0);
			uint64_t max = ~uint64_t(0);
			uint64_t code_value = 0;

			bit_stream stream;

			range() = default;
			range(std::vector<char> memory);

			void encode(std::pair<std::span<const int>, int> weights_and_sum, int selection);
			int decode(std::pair<std::span<const int>, int> weights_and_sum);
			void flush();

			std::vector<char>&& extract_result() { return std::move(stream.m_memory); }
		};
	}

	struct image_coder {
		static constexpr int lossRatio = 8;
		static constexpr int learnRate = 16;

		std::vector<int> combineStorage;
		image_coder()
			: combineStorage(256 / lossRatio + 1) { }

		using WeightDistribution = std::pair<std::span<const int>, int>;
		WeightDistribution combine(WeightDistribution p1, WeightDistribution p2);

		std::vector<char> encode(int width, int height, unsigned char* data);
		unsigned char* decode(int width, int height, std::vector<char>&& encoded);
	};
}