
#pragma once

#include <rynx/tech/unordered_map.hpp>
#include <mutex>
#include <vector>

namespace rynx {
	class type_index {
		mutable uint64_t runningTypeIndex = 0;
		static constexpr uint64_t no_type = ~uint64_t(0);
		mutable unordered_map<uintptr_t, uint64_t> m_typeMap;
		mutable std::mutex m_slow_path_mutex;
		mutable std::vector<std::pair<uintptr_t, uint64_t>> m_fallback;

		// NOTE: As long as we are not using DLLs, this should return the same char* value.
		//       This means we can use the pointer value instead of the actual string representation for map key.
#ifdef _WIN32
		template<typename T> static constexpr char const*const unique_str_for_type() { return __FUNCDNAME__; }
#else
		template<typename T> static constexpr char const*const unique_str_for_type() { return __PRETTY_FUNCTION__; }
#endif
		template<typename T> uint64_t id_private() const {
			constexpr char const * const unique_type_name_ptr = unique_str_for_type<T>();
			uintptr_t unique_ptr_value = uintptr_t(unique_type_name_ptr);
			
			// this is the fast path, and it is thread safe.
			auto it = m_typeMap.find(unique_ptr_value);
			if (it != m_typeMap.end()) {
				return it->second;
			}
			
			// slow path can only be hit on one frame per unique type. so we don't really care how slow it is.
			std::unique_lock lock(m_slow_path_mutex);
			for (auto&& entry : m_fallback) {
				if (entry.first == unique_ptr_value) {
					logmsg("warning - type index using slow path. if this spam does not end, did you forget to call type_index.sync()?");
					return entry.second;
				}
			}
			m_fallback.emplace_back(unique_ptr_value, runningTypeIndex++);
			return m_fallback.back().second;
		}

	public:
		type_index() {
			m_typeMap.reserve(1024);
		}

		// should be called once per frame, while no other threads are using the type index.
		// most often this function does nothing, but it handles the special case of us seeing
		// types we have not seen before.
		void sync() {
			for (auto&& entry : m_fallback) {
				m_typeMap.insert(std::move(entry));
			}
			m_fallback.clear();
		}

		template<typename T> uint64_t id() const {
			return id_private<std::remove_cv_t<std::remove_reference_t<T>>>();
		}
	};
}
