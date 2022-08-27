
#include <rynx/std/string.hpp>
#include <ostream>
#include <istream>

rynx::std_replacements::string_view::string_view(const rynx::std_replacements::string& s) : m_data(s.data()), m_length(s.length()) {}
rynx::std_replacements::string_view::string_view(const rynx::std_replacements::string& s, size_t len) : m_data(s.data()), m_length(len) {}

rynx::std_replacements::string_view::string_view() {}
rynx::std_replacements::string_view::string_view(char const* str) { m_data = str; m_length = std::strlen(str); }

char const* rynx::std_replacements::string_view::data() const { return m_data; }
size_t rynx::std_replacements::string_view::length() const noexcept { return m_length; }
char rynx::std_replacements::string_view::operator[](size_t i) const noexcept { return *(m_data + i); }


std::ostream& operator << (std::ostream& out, const rynx::string& str) {
	out.write(str.data(), str.length());
	return out;
}

std::istream& operator >> (std::istream& in, rynx::string& str) {
	std::string output;
	in >> output;
	str = output.c_str(); // todo optimize
	return in;
}

bool operator == (rynx::std_replacements::string_view a, const rynx::std_replacements::string& b) { return std::strcmp(a.data(), b.data()) == 0; }
bool operator == (const rynx::std_replacements::string& b, rynx::std_replacements::string_view a) { return std::strcmp(a.data(), b.data()) == 0; }

rynx::string operator + (char const* const other, const rynx::string& str) {
	return rynx::string(other) + str;
}

rynx::string operator + (char const* const other, rynx::string&& str) {
	return rynx::string(other) + str;
}




// string member functions

void rynx::std_replacements::string::reallocate_copy(size_t new_size) {
	uint64_t current_size = size();
	char* new_data = new char[new_size];
	std::memcpy(new_data, data(), current_size + 1);
	if (!is_small_space())
		delete[] m_longform.m_data;
	
	m_longform.m_data = new_data;
	m_longform.m_capacity = new_size;
	m_longform.m_size = current_size;
	set_dynamic();
}

void rynx::std_replacements::string::reallocate_ignore(size_t new_size) {
	if (!is_small_space())
		delete[] m_longform.m_data;
	m_longform.m_data = new char[new_size];
	m_longform.m_capacity = new_size;
	m_longform.m_size = 0;
	set_dynamic();
}

void rynx::std_replacements::string::ensure_capacity_copy(size_t required) {
	if (capacity() < required)
		reallocate_copy((required * 3) >> 1);
}

void rynx::std_replacements::string::ensure_capacity(size_t required) {
	if (capacity() < required)
		reallocate_ignore((required * 3) >> 1);
}

rynx::std_replacements::string::string(char const* const str, size_t length) {
	ensure_capacity(length + 1);
	std::memcpy(data(), str, length);
	*(data() + length) = 0;
	set_size_internal(length);
}

rynx::std_replacements::string::string(char const* const str, char const* const end) {
	size_t len = end - str;
	ensure_capacity(len + 1);
	std::memcpy(data(), str, len);
	*(data() + len) = 0;
	set_size_internal(len);
}

rynx::std_replacements::string::string(size_t count, char value) {
	ensure_capacity(count + 1);
	std::memset(data(), value, count);
	*(data() + count) = 0;
	set_size_internal(count);
}

rynx::std_replacements::string::string(const string& other) {
	uint64_t other_size = other.size();
	ensure_capacity(other_size + 1);
	std::memcpy(data(), other.data(), other_size + 1);
	set_size_internal(other_size);
}

rynx::std_replacements::string::string(string&& other) noexcept {
	if (other.is_small_space()) {
		uint64_t other_size = other.size();
		std::memcpy(data(), other.data(), other_size + 1);
		set_size_internal(other_size);
	}
	else {
		m_longform = other.m_longform;
		other.factory_reset(); // doesn't have a dynamic buffer after this.
	}
}

rynx::std_replacements::string::string(char const* const str) {
	size_t len = std::strlen(str);
	ensure_capacity(len + 1);
	std::memcpy(data(), str, len + 1);
	set_size_internal(len);
}

bool rynx::std_replacements::string::is_small_space() const noexcept {
	return *(m_storage + 31) < 127;
}

void rynx::std_replacements::string::set_dynamic() noexcept {
	*(m_storage + 31) = 127;
}

// when moved out of, we can reuse the small space.
void rynx::std_replacements::string::factory_reset() noexcept {
	*(m_storage + 31) = 0;
	m_storage[0] = 0;
}

uint64_t rynx::std_replacements::string::capacity() const noexcept {
	if (is_small_space())
		return SmallBufferSize - 1;
	return m_longform.m_capacity;
}

void rynx::std_replacements::string::set_size_internal(int64_t new_size) {
	if (is_small_space())
		*(m_storage + 31) = static_cast<char>(new_size);
	else
		m_longform.m_size = new_size;
}

void rynx::std_replacements::string::add_size_internal(int64_t add_size) {
	if (is_small_space())
		*(m_storage + 31) += static_cast<char>(add_size);
	else
		m_longform.m_size += add_size;
}


rynx::std_replacements::string& rynx::std_replacements::string::operator = (const string& other) {
	uint64_t other_size = other.size();
	ensure_capacity(other_size + 1);
	std::memcpy(data(), other.data(), other_size + 1);
	set_size_internal(other_size);
	return *this;
}

rynx::std_replacements::string& rynx::std_replacements::string::operator = (string&& other) noexcept {
	if (other.is_small_space()) {
		// no need to check if we have enough space,
		// will fit in small space optimized. and dynamic memory will never allocate less.
		uint64_t other_size = other.size();
		std::memcpy(data(), other.data(), other_size + 1);
		set_size_internal(other_size);
	}
	else {
		m_longform = other.m_longform;
		other.factory_reset(); // doesn't have a dynamic buffer after this.
	}
	return *this;
}

rynx::std_replacements::string& rynx::std_replacements::string::operator = (char const* const str) {
	size_t len = std::strlen(str);
	ensure_capacity(len + 1);
	std::memcpy(data(), str, len);
	*(data() + len) = 0;
	set_size_internal(len);
	return *this;
}

bool rynx::std_replacements::string::operator == (const string& other) const noexcept {
	return std::strcmp(data(), other.data()) == 0;
}

bool rynx::std_replacements::string::operator == (char const* const other) const noexcept {
	return std::strcmp(data(), other) == 0;
}

rynx::std_replacements::string& rynx::std_replacements::string::resize(size_t required) {
	ensure_capacity_copy(required+1);
	*(data() + required) = 0;
	set_size_internal(required);
	return *this;
}

size_t rynx::std_replacements::string::find_last_of(char t, size_t offset) const noexcept {
	char const* const begin = data();
	for (char const* it = begin + min(size(), offset); it != begin; --it) {
		if (*it == t) {
			return size_t(it - begin);
		}
	}
	return npos;
}

size_t rynx::std_replacements::string::find_last_of(const string& ts) const noexcept {
	char const* const begin = data();
	for (char const* it = begin + size(); it != begin; --it) {
		const char v = *it;
		for (const char t : ts)
			if (v == t)
				return size_t(it - begin);
	}
	return npos;
}

size_t rynx::std_replacements::string::find_first_of(char c, size_t offset) const noexcept {
	char const* const begin = data();
	char const* const end = begin + size();
	for (char const* it = begin + offset; it != end; ++it) {
		const char v = *it;
		if (v == c)
			return size_t(it - begin);
	}
	return npos;
}

rynx::std_replacements::string& rynx::std_replacements::string::clear() noexcept {
	if (!is_small_space()) {
		m_longform.m_size = 0;
		*m_longform.m_data = 0;
	}
	else {
		m_storage[0] = 0;
		m_storage[31] = 0;
	}
	return *this;
}

void rynx::std_replacements::string::insert(char* where, char what) {
	ensure_capacity_copy(size() + 2);
	char* it = data() + size() + 1;
	while (it != where) {
		*it = *(it - 1);
		--it;
	}
	*it = what;
	
	if (!is_small_space())
		++m_longform.m_size;
	else
		++* (m_storage + 31);
}

void rynx::std_replacements::string::erase(char* where) noexcept {
	rynx_assert(where >= begin() && where < end() && size() > 0, "erase requirements not met");
	const char* end_it = end()-1;
	while (where != end_it) {
		*where = *(where + 1);
		++where;
	}

	if (!is_small_space())
		--m_longform.m_size;
	else
		--*(m_storage + 31);
}

void rynx::std_replacements::string::replace(size_t offset, size_t count, rynx::std_replacements::string_view what) {
	const uint64_t what_len = what.length();
	const uint64_t my_size = size();
	rynx_assert(offset + count < my_size, "replace out of bounds");
	ensure_capacity_copy(my_size - count + what_len + 1);
	size_t tail_len = my_size - (offset + count) + 1;
	char* my_data = data();

	// move tail backward
	if (what_len < count) {
		char* dst = my_data + offset + what_len;
		char const* src = my_data + offset + count;
		while (--tail_len)
			*dst++ = *src++;
	}

	// move tail forward
	if (what_len > count) {
		char* dst = my_data + offset + what_len + tail_len - 1;
		char const* src = my_data + offset + count + tail_len - 1;
		while (--tail_len)
			*dst-- = *src--;
	}

	// apply the replace
	char* dst = my_data + offset;
	char const* src = what.data();
	for (size_t i = 0; i < what_len; ++i)
		*dst++ = *src++;

	add_size_internal(int64_t(what_len) - int64_t(count));
	*(data() + my_size + what_len - count) = 0;
}

char& rynx::std_replacements::string::back() noexcept {
	rynx_assert(size() > 0, "checking back of empty string");
	return *(data() + size() - 1);
}

char rynx::std_replacements::string::back() const noexcept {
	rynx_assert(size() > 0, "checking back of empty string");
	return *(data() + size() - 1);
}

char& rynx::std_replacements::string::front() noexcept {
	rynx_assert(size() > 0, "checking front of empty string");
	return *data();
}

char rynx::std_replacements::string::front() const noexcept {
	rynx_assert(size() > 0, "checking front of empty string");
	return *data();
}

char rynx::std_replacements::string::pop_back() noexcept {
	rynx_assert(size() > 0, "pop_back on empty string");
	char* last_char = data() + (size() - 1);
	char res = *last_char;
	*last_char = 0;
	if (!is_small_space())
		--m_longform.m_size;
	else
		--*(m_storage + 31);
	return res;
}

char& rynx::std_replacements::string::operator[](size_t i) noexcept {
	rynx_assert(i < size(), "operator[] out of bounds");
	return *(data() + i);
}

char rynx::std_replacements::string::operator[](size_t i) const noexcept {
	rynx_assert(i < size(), "operator[] out of bounds");
	return *(data() + i);
}

rynx::std_replacements::string& rynx::std_replacements::string::push_back(char t) {
	uint64_t my_size = size();
	ensure_capacity_copy(my_size + 1);
	char* dst = begin() + my_size;
	*dst = t;
	++dst;
	*dst = 0;
	if (!is_small_space())
		++m_longform.m_size;
	else
		++*(m_storage + 31);
	return *this;
}

rynx::std_replacements::string rynx::std_replacements::string::substr(size_t offset, size_t length) const {
	string result;
	size_t actual_length = rynx::std_replacements::min(size() - offset, length);
	result.ensure_capacity(actual_length + 1);
	char* result_begin = result.data();
	std::memcpy(result_begin, data() + offset, actual_length);
	*(result_begin + actual_length) = 0;
	result.set_size_internal(actual_length);
	return result;
}

rynx::std_replacements::string& rynx::std_replacements::string::operator += (const string& other) {
	uint64_t my_size = size();
	uint64_t other_size = other.size();
	ensure_capacity_copy(my_size + other_size + 1);
	char* my_begin = begin();
	std::memcpy(my_begin + my_size, other.begin(), other_size);
	*(my_begin + my_size + other_size) = 0;
	add_size_internal(other_size);
	return *this;
}

rynx::std_replacements::string& rynx::std_replacements::string::operator += (char c) {
	ensure_capacity_copy(size() + 2);
	auto size_prior = size();
	*(begin() + size_prior) = c;
	if (!is_small_space()) {
		*(m_longform.m_data + (++m_longform.m_size)) = 0;
	}
	else {
		*(begin() + size_prior + 1) = 0;
		++*(m_storage + 31);
	}
	return *this;
}

rynx::std_replacements::string rynx::std_replacements::string::operator + (const string& other) const {
	string result(*this);
	result += other;
	return result;
}

rynx::std_replacements::string rynx::std_replacements::string::operator + (char c) const {
	string result(*this);
	result += c;
	return result;
}

bool rynx::std_replacements::string::operator < (const string& other) const noexcept {
	char const* data1 = begin();
	char const* data2 = other.begin();

	while ((*data1 == *data2) & (*data1 != 0)) {
		++data1;
		++data2;
	}

	return *data1 < *data2;
}

bool rynx::std_replacements::string::starts_with(const string& other) const noexcept {
	if (other.length() > length())
		return false;

	const char* ptr1 = begin();
	const char* ptr2 = other.begin();
	char const* const end2 = other.end();
	bool starts = true;
	while (ptr2 != end2)
		starts &= (*ptr1++ == *ptr2++);
	return starts;
}

bool rynx::std_replacements::string::ends_with(const string& other) const noexcept {
	if (other.length() > length())
		return false;

	const char* ptr1 = begin() + length() - other.length();
	const char* ptr2 = other.begin();
	char const* const end2 = other.end();
	bool ends = true;
	while(ptr2 != end2)
		ends &= (*ptr1++ == *ptr2++);
	return ends;
}

// naive & slow, if there's a use for it, improve to an actual string find algo.
size_t rynx::std_replacements::string::find(const rynx::std_replacements::string& other, size_t offset) const noexcept {
	const size_t my_len = length();
	const size_t other_len = other.length();
	if (other_len > my_len)
		return npos;

	for (size_t pos = offset; pos <= my_len - other_len; ++pos) {
		bool found = true;
		char const* self_it = begin() + pos;
		char const* other_it = other.begin();
		char const* const other_end = other.end();
		while (other_it != other_end)
			found &= (*self_it++ == *other_it++);
		if (found)
			return pos;
	}
	return npos;
}

char* rynx::std_replacements::string::begin() noexcept { return data(); }
char* rynx::std_replacements::string::end() noexcept { return data() + size(); }

char const* rynx::std_replacements::string::begin() const noexcept { return data(); }
char const* rynx::std_replacements::string::end() const noexcept { return data() + size(); }

char const* rynx::std_replacements::string::c_str() const noexcept {
	return data();
}

char* rynx::std_replacements::string::data() noexcept {
	if (is_small_space())
		return m_storage;
	return m_longform.m_data;
}

char const* rynx::std_replacements::string::data() const noexcept {
	if (is_small_space())
		return m_storage;
	return m_longform.m_data;
}

uint64_t rynx::std_replacements::string::size() const noexcept {
	if (is_small_space())
		return *(m_storage + 31);
	return m_longform.m_size;
}

uint64_t rynx::std_replacements::string::length() const noexcept { return size(); }

bool rynx::std_replacements::string::empty() const noexcept { return size() == 0; }

float rynx::stof(rynx::string_view str) {
	float v;
	sscanf_s(str.data(), "%f", &v);
	return v;
}

bool rynx::is_small_space(const rynx::string& str) {
	char const* str_obj_addr = reinterpret_cast<char const*>(&str);
	char const* str_val_addr = str.c_str();
	return (str_val_addr >= str_obj_addr) && (str_val_addr < str_obj_addr + sizeof(rynx::string));
}