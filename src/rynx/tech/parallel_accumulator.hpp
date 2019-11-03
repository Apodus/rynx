
#pragma once

#include <rynx/tech/unordered_map.hpp>
#include <mutex>
#include <vector>

namespace rynx {
	template<typename T>
	class parallel_accumulator {
		rynx::unordered_map<std::thread::id, std::vector<T>> m_storage;
		std::mutex m_storage_insert_mutex;

	public:
		using storage_type = std::vector<T>;

		parallel_accumulator(size_t max_threads = 64) {
			m_storage.reserve(max_threads);
		}

		~parallel_accumulator() {}

		std::vector<T>& get_local_storage() {
			auto initial_cap = m_storage.capacity();
			std::lock_guard lock(m_storage_insert_mutex);
			
			auto tid = std::this_thread::get_id();
			auto it = m_storage.find(tid);
			if (it == m_storage.end()) {
				it = m_storage.emplace(tid, std::vector<T>()).first;
			}
			std::vector<T>& result = it->second;
			rynx_assert(initial_cap == m_storage.capacity(), "parallel accumulator is not allowed to realloc worker thread count.");
			return result;
		}

		template<typename Func>
		void for_each(Func&& op) {
			for (auto&& entry : m_storage) {
				if (entry.second.empty())
					continue;
				op(entry.second);
			}
		}

		std::vector<T> combine_and_clear() {
			std::vector<T> result;
			for (auto&& entry : m_storage) {
				result.insert(result.end(), std::move_iterator(std::begin(entry.second)), std::move_iterator(std::end(entry.second)));
				entry.second.clear();
			}
			// m_storage = {};
			return result;
		}
	};
}
