
#pragma once

#include <chrono>

namespace rynx {
	class timer {
		std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_last_access;

	public:
		timer() {
			reset();
		}

		void reset() {
			m_begin = std::chrono::high_resolution_clock::now();
			m_last_access = m_begin;
		}

		uint64_t time_since_last_access_ms() {
			auto point = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(point - m_last_access).count();
			m_last_access = point;
			return dt;
		}

		uint64_t time_since_last_access_us() {
			auto point = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration_cast<std::chrono::microseconds>(point - m_last_access).count();
			m_last_access = point;
			return dt;
		}

		uint64_t time_since_last_access_ns() {
			auto point = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(point - m_last_access).count();
			m_last_access = point;
			return dt;
		}


		uint64_t time_since_begin_ms() {
			auto point = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(point - m_begin).count();
			m_last_access = point;
			return dt;
		}

		uint64_t time_since_begin_us() {
			auto point = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration_cast<std::chrono::microseconds>(point - m_begin).count();
			m_last_access = point;
			return dt;
		}

		uint64_t time_since_begin_ns() {
			auto point = std::chrono::high_resolution_clock::now();
			auto dt = std::chrono::duration_cast<std::chrono::nanoseconds>(point - m_begin).count();
			m_last_access = point;
			return dt;
		}
	};
}

#include <vector>

namespace rynx {
	template<typename T>
	class numeric_property {
		std::vector<T> m_storage;
		size_t m_index = 0;
	
	public:
		numeric_property(size_t window_size = 64) {
			resize(window_size);
		}

		numeric_property& resize(size_t window_size) {
			rynx_assert((window_size & (window_size - 1)) == 0, "must be power of 2");
			m_storage.resize(window_size);
			return *this;
		}

		numeric_property& observe_value(T t) noexcept {
			m_storage[m_index] = std::move(t);
			m_index = (++m_index & (m_storage.size() - 1));
			return *this;
		}

		T min() const noexcept {
			T result = m_storage[0];
			for (auto&& v : m_storage) {
				result = v < result ? v : result;
			}
			return result;
		}

		T avg() const noexcept {
			T result = m_storage[0];
			for (auto&& v : m_storage) {
				result += v;
			}
			return result / m_storage.size();
		}

		T max() const noexcept {
			T result = m_storage[0];
			for (auto&& v : m_storage) {
				result = v > result ? v : result;
			}
			return result;
		}
	};
}