
#pragma once

#include <rynx/tech/ecs/id.hpp>
#include <rynx/system/assert.hpp>

#include <type_traits>
#include <vector>
#include <string>

namespace rynx {
	template<typename T, typename IOStream> void serialize(const T& t, IOStream& io);
	template<typename T, typename IOStream> void deserialize(T& t, IOStream& io);
	template<typename T, typename IOStream> T deserialize(IOStream& io);

	namespace serialization {
		template<typename T> struct Serialize {
			template<typename IOStream>
			void serialize(const T& t, IOStream& writer) {
				static_assert(std::is_trivially_constructible_v<T> && std::is_trivially_copyable_v<T>, "no serialization defined for type");
				if constexpr (std::is_trivially_constructible_v<T> && std::is_trivially_copyable_v<T>) {
					writer(&t, sizeof(t));
				}
			}

			template<typename IOStream>
			void deserialize(T& t, IOStream& reader) {
				if constexpr (std::is_trivially_constructible_v<T> && std::is_trivially_copyable_v<T>) {
					reader(&t, sizeof(t));
				}
			}
		};

		template<> struct Serialize<rynx::ecs_internal::id> {
			template<typename IOStream>
			void serialize(const rynx::ecs_internal::id& id, IOStream& writer) {
				rynx::serialize(id.value, writer);
			}

			template<typename IOStream>
			void deserialize(rynx::ecs_internal::id& id, IOStream& reader) {
				rynx::deserialize(id.value, reader);
			}
		};

		template<> struct Serialize<std::string> {
			template<typename IOStream>
			void serialize(const std::string& s, IOStream& writer) {
				writer(s.size());
				writer(s.data(), s.size());
			}

			template<typename IOStream>
			void deserialize(std::string& s, IOStream& reader) {
				size_t numElements = 0;
				reader(numElements);
				s.resize(numElements);
				reader(s.data(), numElements);
			}
		};

		template<typename T> struct Serialize<std::vector<T>> {
			template<typename IOStream>
			void serialize(const std::vector<T>& vec_t, IOStream& writer) {
				writer(vec_t.size());
				for(auto&& t : vec_t)
					rynx::serialize(t, writer);
			}

			template<typename IOStream>
			void deserialize(std::vector<T>& vec_t, IOStream& reader) {
				size_t numElements = 0;
				reader(numElements);
				size_t originalSize = vec_t.size();
				vec_t.reserve(originalSize + numElements);
				for (size_t i = 0; i < numElements; ++i) {
					T t;
					rynx::deserialize(t, reader);
					vec_t.emplace_back(std::move(t));
				}
			}
		};

		class vector_reader {
		public:
			vector_reader(std::vector<char> data) : m_data(std::move(data)) {}
			vector_reader(std::vector<char>&& data) : m_data(std::move(data)) {}

			void operator()(void* dst, size_t size) {
				rynx_assert(m_head + size <= m_data.size(), "reading out of bounds");
				std::memcpy(dst, m_data.data() + m_head, size);
				m_head += size;
			}

			template<typename T> void operator()(T& t) { operator()(&t, sizeof(t)); }
			template<typename T> T read() { T t; operator()(&t, sizeof(t)); return t; }

		private:
			std::vector<char> m_data;
			size_t m_head = 0;
		};

		class vector_writer {
		public:
			vector_writer() {}
			vector_writer(std::vector<char> data) : m_data(std::move(data)) { m_head = m_data.size(); }
			vector_writer(std::vector<char>&& data) : m_data(std::move(data)) { m_head = m_data.size(); }

			void operator()(const void* src, size_t size) {
				if (m_head + size > m_data.size()) {
					m_data.resize((m_head + size) * 2);
				}
				
				std::memcpy(m_data.data() + m_head, src, size);
				m_head += size;
			}

			std::vector<char>& data() { return m_data; }
			const std::vector<char>& data() const { return m_data; }

			template<typename T> void operator()(const T& t) { operator()(&t, sizeof(t)); }

		private:
			std::vector<char> m_data;
			size_t m_head = 0;
		};


		template<typename T> struct for_each_id_field_t {
			template<typename Op> void execute(T&, Op&&) {}
		};

		template<> struct for_each_id_field_t<rynx::id> {
			template<typename Op> void execute(rynx::id& id_value, Op&& op) { op(id_value); }
		};
	}



	template<typename T, typename IOStream> void serialize(const T& t, IOStream& io) {
		rynx::serialization::Serialize<T>().serialize(t, io);
	}

	template<typename T, typename IOStream> void deserialize(T& t, IOStream& io) {
		rynx::serialization::Serialize<T>().deserialize(t, io);
	}

	template<typename T, typename IOStream> T deserialize(IOStream& io) {
		T t;
		rynx::deserialize(t, io);
		return t;
	}

	template<typename T> rynx::serialization::vector_writer serialize_to_memory(const T& t) {
		rynx::serialization::vector_writer writer;
		rynx::serialization::Serialize<T>().serialize(t, writer);
		return writer;
	}

	template<typename T, typename Op> void for_each_id_field(T& t, Op&& op) {
		rynx::serialization::for_each_id_field_t<T>().execute(t, std::forward<Op>(op));
	}
}
