
#pragma once

#include <rynx/tech/unordered_map.hpp>
#include <mutex>
#include <vector>

#include <atomic>

namespace rynx {
	class type_index {
		enum class type_model {
			Global,
			Local
		};
		
		static constexpr type_model type_index_model = type_model::Global;

		mutable std::atomic<uint64_t> runningTypeIndex = 0;
		static std::atomic<uint64_t> runningTypeIndex_global;
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
		// all type indices will share values of types. requires static linkage of all use sites.
		template<typename T> uint64_t id_private__global_ids() const {
			static uint64_t type_id_value = runningTypeIndex_global++;
			return type_id_value;
		}

		// all type indices will have private values of types. still requires static linkage of all use sites.
		template<typename T> uint64_t id_private__local_ids() const {
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
					logmsg_once_per_milliseconds(1000, "warning - type index using slow path. if this spam does not end, did you forget to call type_index.sync()?");
					return entry.second;
				}
			}
			m_fallback.emplace_back(unique_ptr_value, runningTypeIndex++);
			return m_fallback.back().second;
		}

		template<typename T> uint64_t id_private() const {
			if constexpr (type_index_model == type_model::Global) {
				return id_private__global_ids<T>();
			}
			else {
				return id_private__local_ids<T>();
			}
		}

	public:
		struct virtual_type {
			uint64_t type_value;
		};

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

		virtual_type create_virtual_type() {
			if constexpr (type_index_model == type_model::Global) {
				return virtual_type{ runningTypeIndex_global++ };
			}
			else {
				return virtual_type{ runningTypeIndex++ };
			}
		}

		template<typename T> uint64_t id() const {
			return id_private<std::remove_cv_t<std::remove_reference_t<T>>>();
		}
	};
}
