#pragma once

#include <rynx/system/intrinsics.hpp>
#include <rynx/std/dynamic_buffer.hpp>
#include <cstdint>

namespace rynx {
	// todo: allow sizes of not-64-multiples to work correctly on nextOne / nextZero
	class RynxStdDLL dynamic_bitset {
	public:
		static const uint64_t npos = ~uint64_t(0);

		enum class data_view {
			PassThrough,
			Invert
		};

		struct iterator {
			uint64_t index;
			const dynamic_bitset* host;

			iterator& operator++() {
				index = host->nextOne(index + 1);
				return *this;
			}

			bool operator == (const iterator& other) const {
				rynx_assert(host == other.host, "comparing iterators from different bitsets");
				return index == other.index;
			}

			bool operator != (const iterator& other) const {
				return !operator==(other);
			}

			uint64_t operator*() const {
				return index;
			}
		};

		iterator begin() const { return iterator{ nextOne(0), this }; }
		iterator end() const { return iterator{ npos, this }; }

		dynamic_buffer<uint64_t>& data();
		const dynamic_buffer<uint64_t>& data() const;

		dynamic_bitset& set(uint64_t id);
		dynamic_bitset& reset(uint64_t id);

		bool test(uint64_t id) const;
		dynamic_bitset& clear();

		bool includes(const dynamic_bitset& other) const;
		bool excludes(const dynamic_bitset& other) const;

		// merge with operator AND
		template<data_view view>
		dynamic_bitset& merge(const dynamic_bitset& other) {
			rynx_assert(this != &other, "merging with self makes no sense");
			size_t mySize = m_index_data.size();
			size_t otherSize = other.m_index_data.size();
			if (mySize > otherSize) {
				for (size_t i = 0; i < otherSize; ++i)
					if constexpr (view == data_view::PassThrough)
						m_index_data[i] &= other.m_index_data[i];
					else if constexpr (view == data_view::Invert)
						m_index_data[i] &= ~other.m_index_data[i];
				for (size_t i = otherSize; i < mySize; ++i)
					if constexpr (view == data_view::PassThrough)
						m_index_data[i] = 0;
					else if constexpr (view == data_view::Invert)
					{
						// do nothing. the logical idea here is: this &= 0xfffffffffffff
					}
			}
			else {
				for (size_t i = 0; i < mySize; ++i)
					if constexpr (view == data_view::PassThrough)
						m_index_data[i] &= other.m_index_data[i];
					else if constexpr (view == data_view::Invert)
						m_index_data[i] &= ~other.m_index_data[i];
			}
			return *this;
		}

		// NOTE: this requires creating a new bitset => reserves memory => slow.
		dynamic_bitset operator & (const dynamic_bitset& other) const;

		bool operator ==(const dynamic_bitset& other) const;
		dynamic_bitset& resize_bits(size_t bits);

		uint64_t nextOne(uint64_t firstAllowed = 0) const;
		uint64_t nextZero(uint64_t firstAllowed = 0) const;

		template<typename F>
		const dynamic_bitset& forEachOne(F&& op) const {
			uint64_t next = nextOne();
			while (next != npos) {
				op(next);
				next = nextOne(next + 1);
			}
			return *this;
		}

		size_t size() const noexcept {
			return m_index_data.size() << 6;
		}

	private:
		dynamic_buffer<uint64_t> m_index_data;
	};
}