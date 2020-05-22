
#pragma once

#include <rynx/thread/this_thread.hpp>
#include <vector>
#include <tuple>
#include <memory>

namespace rynx {
	template<typename... Ts>
	class parallel_accumulator {
		std::vector<std::tuple<std::vector<Ts>...>> m_storage;

		template <class T, class Tuple> struct find_index_of_type_in_tuple;
		template <class T, class... Types> struct find_index_of_type_in_tuple<T, std::tuple<T, Types...>> { enum { value = 0 }; };
		template <class T, class U, class... Types> struct find_index_of_type_in_tuple<T, std::tuple<U, Types...>> {
			enum {
				value = 1 + find_index_of_type_in_tuple<T, std::tuple<Types...>>::value
			};
		};

	public:
		parallel_accumulator(size_t max_threads = 64) {
			m_storage.resize(max_threads);
		}

		~parallel_accumulator() {}

		template<typename T>
		parallel_accumulator& emplace_back(T&& t) {
			uint64_t tid = rynx::this_thread::id();
			rynx_assert(tid < m_storage.size(), "parallel accumulator is not allowed to realloc worker thread count.");
			auto& vec = std::get<find_index_of_type_in_tuple<std::remove_cvref_t<T>, std::tuple<Ts...>>::value>(
				m_storage[tid]
			);
			vec.emplace_back(std::forward<T>(t));
			return *this;
		}

		template<typename T>
		std::vector<T>& get_local_storage() {
			uint64_t tid = rynx::this_thread::id();
			rynx_assert(tid < m_storage.size(), "parallel accumulator is not allowed to realloc worker thread count.");
			return std::get<find_index_of_type_in_tuple<T, std::tuple<Ts...>>::value>(
				m_storage[tid]
			);
		}

		template<typename Func>
		void for_each(Func&& op) {
			for (auto&& entry_line : m_storage) {
				if (std::apply([](const auto&... ts) { return true & (ts.empty() & ...); }, entry_line))
					continue;
				std::apply(op, entry_line);
			}
		}

		bool empty() {
			bool any_content = false;
			for_each([&](const auto&... v) mutable { any_content |= (!v.empty() | ...); });
			return !any_content;
		}

		void clear() {
			for (auto&& entry_line : m_storage) {
				std::apply([](auto&... ts) { (ts.clear(), ...); }, entry_line);
			}
		}
	};

	template<typename... Ts>
	std::shared_ptr<rynx::parallel_accumulator<Ts...>> make_accumulator_shared_ptr() {
		return std::make_shared<rynx::parallel_accumulator<Ts...>>();
	}
}
