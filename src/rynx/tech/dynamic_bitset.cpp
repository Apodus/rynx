
#include <rynx/tech/dynamic_bitset.hpp>

rynx::dynamic_buffer<uint64_t>& rynx::dynamic_bitset::data() { return m_index_data; }
const rynx::dynamic_buffer<uint64_t>& rynx::dynamic_bitset::data() const { return m_index_data; }

rynx::dynamic_bitset& rynx::dynamic_bitset::set(uint64_t id) {
	uint64_t block = id >> 6;
	uint8_t bit = id & 63;
	if (block >= m_index_data.size())
		m_index_data.resize(((3 * block) >> 1) + 1, 0);
	rynx_assert(block < m_index_data.size(), "out of bounds");
	m_index_data[block] |= uint64_t(1) << bit;
	return *this;
}

rynx::dynamic_bitset& rynx::dynamic_bitset::reset(uint64_t id) {
	uint64_t block = id >> 6;
	uint8_t bit = id & 63;
	if (block >= m_index_data.size())
		m_index_data.resize(((3 * block) >> 1) + 1, 0);
	rynx_assert(block < m_index_data.size(), "out of bounds");
	m_index_data[block] &= ~(uint64_t(1) << bit);
	return *this;
}

bool rynx::dynamic_bitset::test(uint64_t id) const {
	uint64_t block = id >> 6;
	if (block >= m_index_data.size())
		return false;
	uint8_t bit = id & 63;
	return (m_index_data[block] & (uint64_t(1) << bit)) != 0;
}

rynx::dynamic_bitset& rynx::dynamic_bitset::clear() { m_index_data.fill_memset(uint8_t(0)); return *this; }

bool rynx::dynamic_bitset::includes(const dynamic_bitset& other) const {
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

bool rynx::dynamic_bitset::excludes(const dynamic_bitset& other) const {
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

// NOTE: this requires creating a new bitset => reserves memory => slow.
rynx::dynamic_bitset rynx::dynamic_bitset::operator & (const dynamic_bitset& other) const {
	dynamic_bitset result = *this;
	result.merge<data_view::PassThrough>(other);
	return result;
}

bool rynx::dynamic_bitset::operator ==(const dynamic_bitset& other) const {
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

rynx::dynamic_bitset& rynx::dynamic_bitset::resize_bits(size_t bits) {
	m_index_data.resize(((bits + 63) >> 6), uint8_t(0));
	return *this;
}

uint64_t rynx::dynamic_bitset::nextOne(uint64_t firstAllowed) const {
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

uint64_t rynx::dynamic_bitset::nextZero(uint64_t firstAllowed) const {
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