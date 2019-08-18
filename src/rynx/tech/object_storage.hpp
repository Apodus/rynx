#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/tech/type_index.hpp>

#include <vector>
#include <type_traits>

namespace rynx {
	class object_storage {

	public:
		~object_storage() {
			/*
			for (iobject* obj : m_stateObjects) {
				if (obj)
					delete obj;
			}
			*/
		}

		struct iobject {
			virtual ~iobject() {}
		};

		template<typename T>
		void set_and_release(T* t) {
			auto id = m_typeIndex.id<std::remove_reference_t<T>>();
			if (id >= m_stateObjects.size()) {
				m_stateObjects.resize(3 * m_stateObjects.size() / 2 + 1);
			}
			if (m_stateObjects[id])
				delete m_stateObjects[id];
			m_stateObjects[id] = t;
		}

		template<typename T>
		T& get() {
			auto id = m_typeIndex.id<std::remove_reference_t<T>>();
			if (id >= m_stateObjects.size()) {
				m_stateObjects.resize(2 * m_stateObjects.size() / 3 + 1);
			}
			auto* value = static_cast<T*>(m_stateObjects[id]);
			rynx_assert(value, "requested game state object which doesnt exist");
			// TODO: should we try to default construct the object if it's missing?
			// or does that just cause more bugs?
			return *value;
		}

		template<typename T> const T& get() const { return const_cast<object_storage*>(this)->get<T>(); }

	private:
		type_index m_typeIndex;
		// std::vector<iobject*> m_stateObjects;
		std::vector<void*> m_stateObjects;
	};
}