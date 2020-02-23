#pragma once

#include <rynx/system/intrinsics.hpp>
#include <rynx/tech/dynamic_buffer.hpp>
#include <cstdint>

namespace rynx {
	// todo: allow sizes of not-64-multiples to work correctly on nextOne / nextZero
	class dynamic_bitset {
	public:
		static const uint64_t npos = ~uint64_t(0);

		enum class data_view {
			PassThrough,
			Invert
		};

		dynamic_buffer<uint64_t>& data() { return m_index_data; }
		const dynamic_buffer<uint64_t>& data() const { return m_index_data; }

		dynamic_bitset& set(uint64_t id) {
			uint64_t block = id >> 6;
			uint8_t bit = id & 63;
			if (block >= m_index_data.size())
				m_index_data.resize(((3 * block) >> 1) + 1, 0);
			rynx_assert(block < m_index_data.size(), "out of bounds");
			m_index_data[block] |= uint64_t(1) << bit;
			return *this;
		}

		dynamic_bitset& reset(uint64_t id) {
			uint64_t block = id >> 6;
			uint8_t bit = id & 63;
			rynx_assert(block < m_index_data.size(), "out of bounds");
			m_index_data[block] &= ~(uint64_t(1) << bit);
			return *this;
		}

		bool test(uint64_t id) const {
			uint64_t block = id >> 6;
			if (block >= m_index_data.size())
				return false;
			uint8_t bit = id & 63;
			return (m_index_data[block] & (uint64_t(1) << bit)) != 0;
		}

		dynamic_bitset& clear() { m_index_data.fill_memset(uint8_t(0)); return *this; }

		bool includes(const dynamic_bitset& other) const {
			size_t mySize = m_index_data.size();
			size_t otherSize = other.m_index_data.size();

			if (otherSize <= mySize) {
				bool ok = true;
				for (size_t i = 0, end = other.m_index_data.size(); i < end; ++i) {
					ok &= (other.m_index_data[i] & m_index_data[i]) == other.m_index_data[i];
				}
				return ok;
			}
			else {
				bool ok = true;
				size_t end = end = m_index_data.size();
				for (size_t i = 0; i < end; ++i) {
					ok &= (other.m_index_data[i] & m_index_data[i]) == other.m_index_data[i];
				}
				for (size_t i = end; i < other.m_index_data.size(); ++i) {
					ok &= other.m_index_data[i] == 0;
				}
				return ok;
			}
		}

		bool excludes(const dynamic_bitset& other) const {
			size_t mySize = m_index_data.size();
			size_t otherSize = other.m_index_data.size();

			if (otherSize <= mySize) {
				bool ok = true;
				for (size_t i = 0, end = other.m_index_data.size(); i < end; ++i) {
					ok &= (other.m_index_data[i] & m_index_data[i]) == 0;
				}
				return ok;
			}
			else {
				bool ok = true;
				size_t end = end = m_index_data.size();
				for (size_t i = 0; i < end; ++i) {
					ok &= (other.m_index_data[i] & m_index_data[i]) == 0;
				}
				return ok;
			}
		}

		// merge with operator AND
		template<data_view view>
		dynamic_bitset& merge(const dynamic_bitset& other) {
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
		dynamic_bitset operator & (const dynamic_bitset& other) const {
			dynamic_bitset result = *this;
			result.merge<data_view::PassThrough>(other);
			return result;
		}

		bool operator ==(const dynamic_bitset& other) const {
			auto compare = [](const dynamic_bitset& smaller, const dynamic_bitset& larger) {
				bool ok = true;
				for (size_t i = 0; i < smaller.m_index_data.size(); ++i) {
					ok &= smaller.m_index_data[i] == larger.m_index_data[i];
				}
				for (size_t k = smaller.m_index_data.size(); k < larger.m_index_data.size(); ++k) {
					ok &= larger.m_index_data[k] == uint64_t(0);
				}
				return ok;
			};
			if (m_index_data.size() < other.m_index_data.size())
				return compare(*this, other);
			return compare(other, *this);
		}

		dynamic_bitset& resize_bits(size_t bits) {
			m_index_data.resize(((bits + 63) >> 6), uint8_t(0));
			return *this;
		}

		uint64_t nextOne(uint64_t firstAllowed = 0) const {
			uint64_t block = firstAllowed >> 6;
			uint8_t bit = firstAllowed & 63;

			auto size = m_index_data.size();
			if (block >= size)
				return npos;

			uint64_t blockData = m_index_data[block];
			blockData &= ~((uint64_t(1) << uint64_t(bit)) - uint64_t(1));

			if (blockData) {
				uint64_t firstSetBit = findFirstSetBit(blockData);
				return (block << 6) + firstSetBit;
			}

			while (++block < size) {
				blockData = m_index_data[block];
				if (blockData) {
					uint64_t firstSetBit = findFirstSetBit(blockData);
					return (block << 6) + firstSetBit;
				}
			}

			return npos;
		}

		uint64_t nextZero(uint64_t firstAllowed = 0) const {
			uint64_t block = firstAllowed >> 6;
			uint8_t bit = firstAllowed & 63;

			auto size = m_index_data.size();
			if (block >= size)
				return npos;

			uint64_t blockData = ~m_index_data[block];
			blockData &= ~((uint64_t(1) << uint64_t(bit)) - uint64_t(1));

			if (blockData) {
				uint64_t firstSetBit = findFirstSetBit(blockData);
				return (block << 6) + firstSetBit;
			}

			while (++block < size) {
				blockData = ~m_index_data[block];
				if (blockData) {
					uint64_t firstSetBit = findFirstSetBit(blockData);
					return (block << 6) + firstSetBit;
				}
			}

			return npos;
		}

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