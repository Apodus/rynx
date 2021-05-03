
#pragma once

#include <rynx/system/assert.hpp>

#include <cstdint>

namespace rynx {
	struct ecs_value_segregated_component_tag {};
	struct ecs_no_serialize_tag {};

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

			void clear() {
				m_nextId = 0; // reset id space.
			}

			static constexpr entity_id_t InvalidId = 0;
		private:

			// does not need to be atomic. creating new entities is not thread-safe anyway, only one thread is allowed to do so at a time.
			entity_id_t m_nextId = 0;
		};

		struct id {
			id() : value(entity_index::InvalidId) {}
			id(entity_id_t v) : value(v) {}
			bool operator == (const id& other) const { return value == other.value; }
			explicit operator bool() const { return value > 0; }

			entity_id_t value;
		};
	}

	using id = rynx::ecs_internal::id;
}