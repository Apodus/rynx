
#pragma once

#include <span>
#include <vector>

namespace rynx::compression {
	std::vector<char> decompress(std::span<char> memory);
	std::vector<char> compress(std::span<char> memory);


	namespace rangeencoding {
		struct model {
			model(int numSymbols);
			struct node {
				node() {};
				node(int numSymbols);
				void init(int numSymbols);
				const std::vector<int>& predict();
				void update(int symbol);
				bool has_sufficient_data();

				int m_sum = 0;
				std::vector<int> m_weights;
			};

			std::pair<const std::vector<int>&, int> predict();
			void update(int symbol);

			int prev_symbol = 0;
			model::node m_layer0;
			std::vector<model::node> m_layer1;
		};

		struct bit_stream {
			void write_bit(int a);
			int read_bit();

			char current = 0;
			int current_bits = 0;
			std::vector<char> m_memory;
		};

		struct range {
			uint64_t min = uint64_t(0);
			uint64_t max = ~uint64_t(0);
			uint64_t code_value = 0;

			bit_stream stream;

			range() = default;
			range(std::vector<char> memory);

			void encode(std::pair<const std::vector<int>&, int> weights_and_sum, int selection);
			int decode(std::pair<const std::vector<int>&, int> weights_and_sum);
			void flush();
		};
	}
}