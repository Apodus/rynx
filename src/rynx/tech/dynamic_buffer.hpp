#pragma once

#include <rynx/system/assert.hpp>
#include <type_traits>
#include <memory>

template<typename T>
class dynamic_buffer {
	static_assert(std::is_pod<T>::value, "dynamic buffer can only be used with pods");

public:
	dynamic_buffer() {}
	dynamic_buffer(size_t s) {
		resize_discard(s);
	}
	dynamic_buffer(dynamic_buffer&& other) {
		*this = std::move(other);
	}
	dynamic_buffer(const dynamic_buffer& other) {
		*this = other;
	}
	dynamic_buffer& operator=(const dynamic_buffer& other) {
		if (other.m_data) {
			resize_discard(other.size());
			memcpy(m_data.get(), other.m_data.get(), other.size() * sizeof(T));
		}
		else {
			m_data = nullptr;
			m_size = 0;
		}
		return *this;
	}

	dynamic_buffer& operator=(dynamic_buffer&& other) {
		m_data = std::move(other.m_data);
		m_size = other.m_size;
		other.m_size = 0;
		other.m_data.reset(nullptr);
		return *this;
	}

	dynamic_buffer(size_t s, T initialValue) {
		resize_discard(s);
		fill(initialValue);
	}

	dynamic_buffer& resize_discard(size_t s) {
		m_data.reset(new T[s]);
		m_size = s;
		return *this;
	}

	template<typename U>
	dynamic_buffer& resize_discard(size_t s, U u) {
		resize_discard(s);
		if constexpr (sizeof(u) == 1)
			fill_memset(u);
		else
			fill(u);
		return *this;
	}

	dynamic_buffer& resize(size_t s) {
		T* oldData = m_data.get();
		T* newData = new T[s];
		memcpy(newData, oldData, sizeof(T) * (m_size < s ? m_size : s));
		m_data.reset(newData);
		m_size = s;
		return *this;
	}

	template<typename U>
	dynamic_buffer& resize(size_t s, U t) {
		T* oldData = m_data.get();
		T* newData = new T[s];
		
		memcpy(newData, oldData, sizeof(T) * (m_size < s ? m_size : s));
		m_data.reset(newData);
		
		if (s > m_size) {
			if constexpr (sizeof(t) == 1) {
				memset(newData + m_size, t, sizeof(T) * (s - m_size));
			}
			else {
				size_t index = m_size;
				while (index < s) {
					newData[index] = t;
					++index;
				}
			}
		}

		m_size = s;
		return *this;
	}

	dynamic_buffer& fill_memset(uint8_t u) { memset(m_data.get(), u, m_size * sizeof(T)); return *this; }
	dynamic_buffer& fill_memset(int8_t u) { memset(m_data.get(), u, m_size * sizeof(T)); return *this; }

	dynamic_buffer& fill(T t) {
		auto* ptr = m_data.get();
		rynx_assert(ptr != nullptr, "trying to fill an invalid buffer");
		auto* end = ptr + m_size;
		while (ptr != end)
			*ptr++ = t;
		return *this;
	}

	const T& operator [](size_t index) const {
		rynx_assert(index < m_size, "index out of bounds");
		return *(m_data.get() + index);
	}
	T& operator [](size_t index) {
		rynx_assert(index < m_size, "index out of bounds");
		return *(m_data.get() + index);
	}

	size_t size() const { return m_size; }

private:
	std::unique_ptr<T[]> m_data;
	size_t m_size = 0;
};
