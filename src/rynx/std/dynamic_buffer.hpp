#pragma once

#include <rynx/std/serialization_declares.hpp>
#include <rynx/system/assert.hpp>
#include <type_traits>
#include <cstring>

namespace rynx {

	template<typename T>
	class dynamic_buffer {
		static_assert(std::is_trivially_copyable<T>::value && std::is_standard_layout<T>::value, "dynamic buffer can only be used with pods");

		class buffer_ptr {
		public:
			buffer_ptr() noexcept = default;
			buffer_ptr(T* t) noexcept : t(t) {}
			buffer_ptr(buffer_ptr&& other) noexcept : t(other.t) { other.t = nullptr; }
			buffer_ptr& operator = (buffer_ptr&& other) noexcept {
				t = other.t;
				other.t = nullptr;
				return *this;
			}

			buffer_ptr(const buffer_ptr& other) = delete;
			buffer_ptr& operator = (const buffer_ptr& other) = delete;

			~buffer_ptr() {
				if (t) {
					delete[] t;
				}
			}

			buffer_ptr& reset(T* nt) {
				if (t) {
					delete[] t;
				}
				t = nt;
				return *this;
			}

			T* get() const {
				return t;
			}

			operator bool() const {
				return t != nullptr;
			}

		private:
			T* t = nullptr;
		};

	public:
		dynamic_buffer() {}
		dynamic_buffer(size_t s) {
			resize_discard(s);
		}
		dynamic_buffer(dynamic_buffer&& other) noexcept {
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

		T& front() { return *m_data.get(); }
		T& back() { return *(m_data.get() + (m_size - 1)); }

		T front() const { return *m_data.get(); }
		T back() const { return *(m_data.get() + (m_size - 1)); }


		T* begin() { return m_data.get(); }
		T* end() { return m_data.get() + m_size; }
		
		const T* begin() const { return m_data.get(); }
		const T* end() const { return m_data.get() + m_size; }

		T* data() { return m_data.get(); }
		const T* data() const { return m_data.get(); }

		dynamic_buffer& operator=(dynamic_buffer&& other) noexcept {
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

		T operator [](size_t index) const {
			rynx_assert(index < m_size, "index out of bounds");
			return *(m_data.get() + index);
		}
		T& operator [](size_t index) {
			rynx_assert(index < m_size, "index out of bounds");
			return *(m_data.get() + index);
		}

		size_t size() const { return m_size; }

	private:
		buffer_ptr m_data;
		size_t m_size = 0;

		friend struct rynx::serialization::Serialize<rynx::dynamic_buffer<T>>;
	};

	template<typename T>
	class pod_vector
	{
	public:
		pod_vector() = default;
		pod_vector(const pod_vector&) = default;
		pod_vector(pod_vector&&) = default;

		pod_vector& operator = (const pod_vector&) = default;
		pod_vector& operator = (pod_vector&& other) noexcept {
			m_data = std::move(other.m_data);
			m_size = other.m_size;
			other.m_size = 0;
			return *this;
		}

		T& front() noexcept { rynx_assert(m_data.size() > 0, "asking for front() of size 0 vector"); return m_data.front(); }
		T& back() noexcept { rynx_assert(m_data.size() > 0, "asking for back() of size 0 vector"); return m_data[m_size  - 1]; }

		T front() const noexcept { rynx_assert(m_data.size() > 0, "asking for front() of size 0 vector"); return m_data.front(); }
		T back() const noexcept { rynx_assert(m_data.size() > 0, "asking for back() of size 0 vector"); return m_data[m_size - 1]; }

		T* begin() noexcept { return m_data.data(); }
		T* end() noexcept { return m_data.data() + m_size; }

		const T* begin() const noexcept { return m_data.data(); }
		const T* end() const noexcept { return m_data.data() + m_size; }

		T operator [](size_t index) const noexcept { return m_data[index]; }
		T& operator [](size_t index) noexcept { return m_data[index]; }

		void resize(size_t newSize) { m_size = newSize; m_data.resize(newSize); }
		size_t size() const noexcept { return m_size; }

		T pop_back() noexcept { return m_data[--m_size]; }
		bool empty() const noexcept { return m_size == 0; }
		
		void emplace_back(T t) {
			if (m_size == m_data.size()) {
				m_data.resize((m_size == 0) ? 8 : (2 * m_size));
			}
			m_data[m_size] = t;
			++m_size;
		}

		void erase(T* iter) noexcept {
			int32_t n = int32_t(iter - m_data.data()); // / sizeof(T);
			while (n + 1 < m_size) {
				m_data[n] = m_data[n + 1];
				++n;
			}
			--m_size;
		}

		void insert(T* where, T v) {
			emplace_back(T());
			T* it = end();
			--it;
			
			while (it != where) {
				*it = *(it - 1);
				--it;
			}

			*(where + 1) = v;
		}

		size_t capacity() const noexcept {
			return m_data.size();
		}

	private:
		rynx::dynamic_buffer<T> m_data;
		size_t m_size = 0;

		friend struct rynx::serialization::Serialize<rynx::pod_vector<T>>;
	};

	namespace serialization {
		template<typename T> struct Serialize<rynx::dynamic_buffer<T>> {
			template<typename IOStream>
			void serialize(const rynx::dynamic_buffer<T>& s, IOStream& writer) {
				uint64_t size = s.size();
				writer(size);
				writer(s.data(), sizeof(T) * s.size());
			}

			template<typename IOStream>
			void deserialize(rynx::dynamic_buffer<T>& s, IOStream& reader) {
				uint64_t size = rynx::deserialize<uint64_t>(reader);
				s.resize_discard(size);
				reader(s.data(), sizeof(T) * size);
			}
		};

		template<typename T> struct Serialize<rynx::pod_vector<T>> {
			template<typename IOStream>
			void serialize(const rynx::pod_vector<T>& s, IOStream& writer) {
				rynx::serialize(s.m_data, writer);
				rynx::serialize(s.m_size, writer);
			}

			template<typename IOStream>
			void deserialize(rynx::pod_vector<T>& s, IOStream& reader) {
				rynx::deserialize(s.m_data, reader);
				rynx::deserialize(s.m_size, reader);
			}
		};
	}
}