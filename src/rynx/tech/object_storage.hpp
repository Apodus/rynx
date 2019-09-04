#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/tech/type_index.hpp>

#include <vector>
#include <type_traits>

namespace rynx {

	// object storage does not own the data it manages. user is responsible for life time management.
	class object_storage {
	public:
		~object_storage() {}

		// set object for type T. if an object for that type already exists, delete the previous.
		template<typename T>
		void set_and_release(T* t) {
			auto id = m_typeIndex.id<std::remove_reference_t<T>>();
			if (id >= m_stateObjects.size()) {
				m_stateObjects.resize(3 * m_stateObjects.size() / 2 + 1, nullptr);
			}
			if (m_stateObjects[id])
				delete m_stateObjects[id];
			m_stateObjects[id] = t;
		}

		// set object for type T. if one exists already, replace the pointer value.
		template<typename T>
		void set_and_discard(T* t) {
			auto id = m_typeIndex.id<std::remove_reference_t<T>>();
			if (id >= m_stateObjects.size()) {
				m_stateObjects.resize(3 * m_stateObjects.size() / 2 + 1, nullptr);
			}
			m_stateObjects[id] = t;
		}

		template<typename T>
		T& get() {
			auto id = m_typeIndex.id<std::remove_reference_t<T>>();
			if (id >= m_stateObjects.size()) {
				m_stateObjects.resize(2 * m_stateObjects.size() / 3 + 1, nullptr);
			}
			auto* value = static_cast<T*>(m_stateObjects[id]);
			rynx_assert(value, "requested type does not exist in object storage.");
			return *value;
		}

		template<typename T> const T& get() const { return const_cast<object_storage*>(this)->get<T>(); }

	private:
		type_index m_typeIndex;
		std::vector<void*> m_stateObjects;
	};
}