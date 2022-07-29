
#pragma once

#include <rynx/system/assert.hpp>
#include <cstdint>

namespace rynx {
	struct ecs_value_segregated_component_tag {};
	struct ecs_no_serialize_tag {};
	struct ecs_no_component_tag {};

	using type_id_t = uint64_t;
	using entity_id_t = uint64_t;
	using index_t = uint32_t;

	static constexpr uint32_t ecs_id_bits = 42; // 2^42 - 1 entity ids (zero not included).
	static constexpr uint64_t ecs_max_id_mask = (uint64_t(1) << ecs_id_bits) - 1;

	namespace ecs_internal {
		// NOTE: Since we are using categories now, it really doesn't matter how far apart the entity ids are.
		//       Reusing ids is thus very much not important, unless we get to the point of overflow.
		//       But still it would be easier to "reset" entity_index between levels or something.
		class entity_index {
		public:
			entity_id_t generateOne() {
				rynx_assert(m_nextId + 1 <= ecs_max_id_mask, "id space out of bounds");
				return ++m_nextId;
			} // zero is never generated. we can use that as InvalidId.

			entity_id_t peek_next_id() const {
				return m_nextId + 1;
			}

			entity_index() = default;
			entity_index(const entity_index& other) = default;
			entity_index& operator =(const entity_index& other) = default;

			void clear() {
				m_nextId = 0; // reset id space.
			}

			static constexpr entity_id_t InvalidId = 0;
		private:

			// does not need to be atomic. creating new entities is not thread-safe anyway, only one thread is allowed to do so at a time.
			entity_id_t m_nextId = 0;
		};

		struct id {
			id() noexcept : value(entity_index::InvalidId) {}
			id(entity_id_t v) noexcept : value(v) {}
			bool operator == (const id& other) const noexcept { return value == other.value; }
			bool operator == (entity_id_t other) const noexcept { return value == other; }
			explicit operator bool() const noexcept { return value > 0; }
			operator entity_id_t() const noexcept { return value; }

			entity_id_t value;
		};
	}

	// a range of entities with consecutive id values.
	// can be guaranteed for example when deserializing.
	struct entity_range_t : ecs_no_component_tag {
		struct iterator {
			rynx::ecs_internal::id current;
			rynx::ecs_internal::id operator *() const noexcept { return current; }
			iterator operator++() noexcept { ++current.value; return *this; }
			iterator operator++(int) noexcept { iterator copy = *this; ++current.value; return copy; }
			bool operator == (const iterator& other) const noexcept { return current == other.current; }
			bool operator != (const iterator& other) const noexcept { return !(operator==(other)); }
		};

		entity_range_t() noexcept = default;
		entity_range_t(rynx::ecs_internal::id begin, rynx::ecs_internal::id end) noexcept : m_begin(begin), m_end(end) {}
		entity_range_t& operator =(const entity_range_t& other) noexcept = default;

		bool contains(rynx::ecs_internal::id id) const noexcept { return (id.value >= m_begin.value) & (id.value < m_end.value); }
		iterator begin() const noexcept { return iterator{ m_begin }; }
		iterator end() const noexcept { return iterator{ m_end }; }
		size_t size() const noexcept { return m_end.value - m_begin.value; }

		rynx::ecs_internal::id m_begin;
		rynx::ecs_internal::id m_end;
	};

	using id = rynx::ecs_internal::id;
}