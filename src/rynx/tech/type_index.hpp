
#pragma once

#include <rynx/tech/unordered_map.hpp>

namespace rynx {
	class type_index {
		mutable uint64_t runningTypeIndex = 0;
		static constexpr uint64_t no_type = ~uint64_t(0);
		mutable unordered_map<uintptr_t, uint64_t> m_typeMap;

		// NOTE: As long as we are not using DLLs, this should return the same char* value.
		//       This means we can use the pointer value instead of the actual string representation for map key.
#ifdef _WIN32
		template<typename T> static constexpr const char* unique_str_for_type() { return __FUNCDNAME__; }
#else
		template<typename T> static constexpr const char* unique_str_for_type() { return __PRETTY_FUNCTION__; }
#endif
		template<typename T> uint64_t id_private() const {
			constexpr const char* kek = unique_str_for_type<T>();
			auto it = m_typeMap.find(uintptr_t(kek));
			if (it != m_typeMap.end()) {
				return it->second;
			}
			return m_typeMap.insert({ uintptr_t(kek), runningTypeIndex++ }).first->second;
		}

	public:
		template<typename T> uint64_t id() const {
			return id_private<std::remove_cv_t<std::remove_reference_t<T>>>();
		}
	};
}
