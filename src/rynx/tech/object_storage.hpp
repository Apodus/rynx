#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/tech/type_index.hpp>
#include <rynx/tech/memory.hpp>

#include <vector>
#include <type_traits>

namespace rynx {

	// object storage does not own the data it manages. user is responsible for life time management.
	class object_storage {
	public:
		~object_storage() {}

		// set object for type T. if one exists already, replace the pointer value and return the old value
		template<typename T>
		rynx::observer_ptr<T> set_and_discard(rynx::observer_ptr<T> t) {
			auto id = rynx::type_index::id<std::remove_reference_t<T>>();
			ensure_size(id);
			rynx::observer_ptr<T> old_value = m_stateObjects[id].static_pointer_cast<T>();
			m_stateObjects[id] = std::move(t);
			
			delete m_owned_objects[id];
			m_owned_objects[id] = nullptr;
			
			return old_value;
		}

		template<typename T>
		std::unique_ptr<T> set_and_discard(std::unique_ptr<T> t) {
			auto id = rynx::type_index::id<std::remove_reference_t<T>>();
			ensure_size(id);

			
			std::unique_ptr<T> original(static_cast<T*>(m_owned_objects[id]));
			
			m_owned_objects[id] = t.get();
			t.release();

			m_stateObjects[id] = m_owned_objects[id];
			return original;
		}

		template<typename T>
		rynx::observer_ptr<T> try_get() {
			auto id = rynx::type_index::id<std::remove_reference_t<T>>();
			ensure_size(id);
			return m_stateObjects[id].static_pointer_cast<T>();
		}

		template<typename T>
		rynx::observer_ptr<T> get() {
			auto id = rynx::type_index::id<std::remove_reference_t<T>>();
			rynx_assert(id < m_stateObjects.size()  && m_stateObjects[id] != nullptr, "requested type does not exist in object storage.");
			return m_stateObjects[id].static_pointer_cast<T>();
		}

		template<typename T> const rynx::observer_ptr<T> get() const { return const_cast<object_storage*>(this)->get<T>(); }
		template<typename T> const rynx::observer_ptr<T> try_get() const { return const_cast<object_storage*>(this)->try_get<T>(); }

	private:
		void ensure_size(size_t s) {
			if (s >= m_stateObjects.size()) {
				size_t newSize = 3 * s / 2 + 1;
				m_stateObjects.resize(newSize, nullptr);
				m_owned_objects.resize(newSize);
			}
		}

		std::vector<rynx::observer_ptr<void>> m_stateObjects;
		std::vector<void*> m_owned_objects;
	};
}
