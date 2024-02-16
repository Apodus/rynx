

#pragma once

#include <rynx/system/assert.hpp>
#include <cstdint>
#include <cstring>

// todo: maybe get rid of
#include <type_traits>
#include <iosfwd>

namespace rynx {
	namespace std_replacements {
		template<typename T>
		T min(T a, T b) {
			return a < b ? a : b;
		}
		
		class string;
		class RynxStdDLL string_view {
			char const* m_data = nullptr;
			size_t m_length = 0;

		public:
			string_view();
			string_view(char const* str);
			string_view(const rynx::std_replacements::string& s);
			string_view(const rynx::std_replacements::string& s, size_t len);
			string_view(string_view&&) = default;
			string_view(const string_view&) = default;

			string_view& operator =(string_view&&) = default;
			string_view& operator =(const string_view&) = default;

			char const* data() const;
			size_t length() const noexcept;
			char operator[](size_t i) const noexcept;
		};

		class RynxStdDLL string {
		public:
			static constexpr uint64_t npos = ~uint64_t(0);

		private:
			static constexpr size_t SmallBufferSize = 32;

			union {
				char m_storage[SmallBufferSize] = { 0 };
				
				struct {
					char* m_data;
					uint64_t m_size;
					uint64_t m_capacity;
					uint64_t m_padding;
				} m_longform;
			};

			void reallocate_copy(size_t new_size);
			void reallocate_ignore(size_t new_size);

			void ensure_capacity_copy(size_t required);
			void ensure_capacity(size_t required);

			bool is_small_space() const noexcept;
			void set_dynamic() noexcept;
			void factory_reset() noexcept; // when moved out of, we can reuse the small space.
			uint64_t capacity() const noexcept;
			void set_size_internal(int64_t new_size);
			void add_size_internal(int64_t add_size);

		public:
			string() = default;

			string(char const* const str, size_t length);
			string(char const* const str, char const* const end);
			string(size_t count, char value = ' ');
			string(const string& other);

			string(string&& other) noexcept;
			string(char const* const str);

			string& operator = (const string& other);
			string& operator = (string&& other) noexcept;
			string& operator = (char const* const str);

			bool operator == (const string& other) const noexcept;
			bool operator == (char const* const other) const noexcept;

			string& resize(size_t required);
			size_t find_last_of(char t, size_t offset = ~uint64_t(0)) const noexcept;
			size_t find_last_of(const string& ts) const noexcept;
			size_t find_first_of(char c, size_t offset = 0) const noexcept;

			string& clear() noexcept;
			
			void insert(char* where, char what);
			void erase(char* where) noexcept;

			void replace(size_t offset, size_t count, rynx::std_replacements::string_view what);
			
			char& back() noexcept;
			char back() const noexcept;

			char& front() noexcept;
			char front() const noexcept;

			char pop_back() noexcept;
			char& operator[](size_t i) noexcept;
			char operator[](size_t i) const noexcept;

			string& push_back(char t);
			string substr(size_t offset, size_t length = ~uint64_t(0)) const;

			string& operator += (const string& other);
			string& operator += (char c);
			string operator + (const string& other) const;
			string operator + (char c) const;

			bool operator < (const string& other) const noexcept;
			
			bool starts_with(const string& other) const noexcept;
			bool ends_with(const string& other) const noexcept;

			// naive & slow, if there's a use for it, improve to an actual string find algo.
			size_t find(const string& other, size_t offset = 0) const noexcept;

			char* begin() noexcept;
			char* end() noexcept;

			char const* begin() const noexcept;
			char const* end() const noexcept;

			char const* c_str() const noexcept;
			char* data() noexcept;
			char const* data() const noexcept;

			uint64_t size() const noexcept;
			uint64_t length() const noexcept;
			bool empty() const noexcept;
		};


	}

	template<class T>
	char* unsigned_value_to_str_buffer(char* buffer_next, T unsigned_value) {
		static_assert(std::is_unsigned_v<T>, "T must be unsigned");
		auto unsigned_value_remaining = unsigned_value;
		do {
			*--buffer_next = static_cast<char>('0' + unsigned_value_remaining % 10);
			unsigned_value_remaining /= 10;
		} while (unsigned_value_remaining != 0);
		return buffer_next;
	}

	template <class T>
	[[nodiscard]] rynx::std_replacements::string signed_value_to_string(const T signed_t) {
		static_assert(std::is_integral_v<T>, "T must be integral");
		using UT = std::make_unsigned_t<T>;
		char buffer[21]; // can hold -2^63 and 2^64 - 1, plus terminator
		char* const buffer_end = buffer+21;
		char* buffer_next = buffer_end;
		const auto unsigned_t = static_cast<UT>(signed_t);
		if (signed_t < 0) {
			buffer_next = unsigned_value_to_str_buffer(buffer_next, 0 - unsigned_t);
			*--buffer_next = '-';
		}
		else {
			buffer_next = unsigned_value_to_str_buffer(buffer_next, unsigned_t);
		}

		return rynx::std_replacements::string(buffer_next, buffer_end);
	}

	template <class T>
	[[nodiscard]] rynx::std_replacements::string unsigned_value_to_string(const T t) {
		static_assert(std::is_integral_v<T>, "T must be integral");
		static_assert(std::is_unsigned_v<T>, "T must be unsigned");
		char buffer[21]; // can hold 2^64 - 1, plus terminator
		char* const buffer_end = buffer + 21;
		char* const buffer_next = unsigned_value_to_str_buffer(buffer_end, t);
		return rynx::std_replacements::string(buffer_next, buffer_end);
	}

	[[nodiscard]] inline rynx::std_replacements::string to_string(int v) {
		return signed_value_to_string(v);
	}

	[[nodiscard]] inline rynx::std_replacements::string to_string(unsigned int v) {
		return unsigned_value_to_string(v);
	}

	[[nodiscard]] inline rynx::std_replacements::string to_string(long v) {
		return signed_value_to_string(v);
	}

	[[nodiscard]] inline rynx::std_replacements::string to_string(unsigned long v) {
		return unsigned_value_to_string(v);
	}

	[[nodiscard]] inline rynx::std_replacements::string to_string(long long v) {
		return signed_value_to_string(v);
	}

	[[nodiscard]] inline rynx::std_replacements::string to_string(unsigned long long v) {
		return unsigned_value_to_string(v);
	}

	[[nodiscard]] inline rynx::std_replacements::string to_string(double v) {
		const auto len = static_cast<size_t>(_scprintf("%f", v));
		rynx::std_replacements::string str(len, '\0');
		sprintf_s(&str[0], len + 1, "%f", v);
		return str;
	}

	[[nodiscard]] inline rynx::std_replacements::string to_string(float v) {
		return rynx::to_string(static_cast<double>(v));
	}

	[[nodiscard]] inline rynx::std_replacements::string to_string(long double v) {
		return rynx::to_string(static_cast<double>(v));
	}

	int64_t RynxStdDLL str_to_int64(rynx::std_replacements::string_view str);
	float RynxStdDLL stof(rynx::std_replacements::string_view str);
	bool RynxStdDLL is_small_space(const rynx::std_replacements::string& str);

	using string = rynx::std_replacements::string;
	using string_view = rynx::std_replacements::string_view;
}

RynxStdDLL rynx::string operator + (char const* const other, const rynx::string& str);
RynxStdDLL rynx::string operator + (char const* const other, rynx::string&& str);

// todo: use some actual string hash
namespace std {
	template<typename T> struct hash;
	template<>
	struct hash<rynx::string> {
		size_t operator()(const rynx::string& str) const noexcept {
			size_t result = 579287893572;
			for (char c : str) {
				result ^= (7549875389 * c) << 13;
				result ^= (7239857289 * c) >> 4;
			}
			return result;
		}
	};
}

RynxStdDLL bool operator == (rynx::std_replacements::string_view a, const rynx::std_replacements::string& b);
RynxStdDLL bool operator == (const rynx::std_replacements::string& b, rynx::std_replacements::string_view a);

RynxStdDLL std::ostream& operator << (std::ostream& out, const rynx::string& str);
RynxStdDLL std::istream& operator >> (std::istream& in, rynx::string& str);