
#pragma once

#include <rynx/thread/this_thread.hpp>

#include <atomic>
#include <array>
#include <vector>

namespace rynx {
	namespace parallel {
		
		template<typename T, size_t MaxSize = 1024>
		class queue {
			static_assert((MaxSize & (MaxSize - 1)) == 0, "must be power of two");
			
			class per_thread_queue {
				static constexpr int32_t counter_max_value = (1 << 30);
			public:
				per_thread_queue() {
					m_data.resize(MaxSize);
				}

				void enque(T&& t) {
					auto index = m_top_actual.load();
					rynx_assert( ((index + 1) & (MaxSize - 1)) != (m_bot_actual & (MaxSize - 1)), "que overflow!");
					m_data[index & (MaxSize - 1)] = std::move(t);
					m_top_actual.fetch_add(1);

					if (m_bot_actual > counter_max_value) [[unlikely]] {
						m_top_actual -= counter_max_value;
						m_bot_actual -= counter_max_value;
						m_bot_reserving -= counter_max_value;
					}
				}

				void enque(std::vector<T>&& ts) {
					auto index = m_top_actual.load();
					
					for (auto&& data : ts) {
						rynx_assert(((index + 1) & (MaxSize - 1)) != (m_bot_actual & (MaxSize - 1)), "que overflow!");
						m_data[index & (MaxSize - 1)] = std::move(data);
						++index;
					}

					m_top_actual.fetch_add(ts.size());

					if (m_bot_actual > counter_max_value) [[unlikely]] {
						m_top_actual -= counter_max_value;
						m_bot_actual -= counter_max_value;
						m_bot_reserving -= counter_max_value;
					}
				}

				[[nodiscard]] bool deque(T& ans) {
					if (m_bot_reserving >= m_top_actual) [[unlikely]] {
						// que is empty.
						return false;
					}

					{
						auto index = m_bot_reserving.fetch_add(1);
						if (index >= m_top_actual) [[unlikely]] {
							// there was stuff in que but someone else took it already.
							m_bot_reserving.fetch_add(-1);
							return false;
						}
					}

					{
						auto index = m_bot_actual.fetch_add(1);
						ans = std::move(m_data[index & (MaxSize - 1)]);
						return true;
					}
				}

				bool empty() const {
					return m_top_actual == m_bot_actual;
				}

			private:
				std::vector<T> m_data;
				alignas(std::hardware_destructive_interference_size) std::atomic<int32_t> m_top_actual = 0;
				alignas(std::hardware_destructive_interference_size) std::atomic<int32_t> m_bot_reserving = 0;
				alignas(std::hardware_destructive_interference_size) std::atomic<int32_t> m_bot_actual = 0;
			};

		public:
			queue(int32_t numThreads = 8) {
				m_subques.reset(new per_thread_queue[numThreads]);
				m_size = numThreads;
			}

			void enque(T&& t) {
				auto tid = rynx::this_thread::id();
				rynx_assert(tid < int64_t(m_size), "overflow - you need to increase MaxThreads value");
				m_subques[tid].enque(std::move(t));
			}

			void enque(std::vector<T>&& ts) {
				auto tid = rynx::this_thread::id();
				rynx_assert(tid < int64_t(m_size), "overflow - you need to increase MaxThreads value");
				m_subques[tid].enque(std::move(ts));
			}
			
			bool deque(T& t) {
				auto current = rynx::this_thread::id();
				uint32_t MaxThreads = m_size;
				for (uint32_t i = 1; i <= MaxThreads; ++i) {
					if (m_subques[(current + i) % MaxThreads].deque(t)) {
						return true;
					}
				}
				return false;
			}

			bool empty() const {
				bool empty = true;
				const auto* end = m_subques.get() + m_size;
				const auto* begin = m_subques.get();
				for (; begin != end; ++begin) {
					empty &= begin->empty();
				}
				return empty;
			}

		private:
			uint32_t m_size = 0;
			std::unique_ptr<per_thread_queue[]> m_subques;
		};
	}
}
