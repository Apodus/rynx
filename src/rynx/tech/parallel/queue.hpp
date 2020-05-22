
#pragma once

#include <rynx/thread/this_thread.hpp>

#include <atomic>
#include <array>
#include <vector>

namespace rynx {
	namespace parallel {
		
		template<typename T, size_t MaxSize = 1024, size_t MaxThreads = 8>
		class queue {
			static_assert((MaxSize & (MaxSize - 1)) == 0, "must be power of two");

			class per_thread_queue {
			public:
				per_thread_queue() {
					m_data.resize(MaxSize);
				}

				void enque(T&& t) {
					auto index = m_top_actual.load();
					m_data[index & (MaxSize - 1)] = std::move(t);
					++m_top_actual;
				}

				bool deque(T& ans) {
					if (m_bot_reserving >= m_top_actual) [[unlikely]] {
						// que is empty.
						return false;
					}

					{
						auto index = m_bot_reserving.fetch_add(1);
						if (index >= m_top_actual) {
							// we didn't get it.
							--m_bot_reserving;
							return false;
						}
					}

					{
						auto index = m_bot_actual.fetch_add(1);
						ans = std::move(m_data[index & (MaxSize - 1)]);
						return true;
					}
				}

				uint32_t size() const {
					return m_top_actual - m_bot_actual;
				}

			private:
				std::vector<T> m_data;
				alignas(std::hardware_destructive_interference_size) std::atomic<uint32_t> m_top_actual = 0;
				alignas(std::hardware_destructive_interference_size) std::atomic<uint32_t> m_bot_reserving = 0;
				alignas(std::hardware_destructive_interference_size) std::atomic<uint32_t> m_bot_actual = 0;
			};

		public:
			void enque(T&& t) {
				m_subques[rynx::this_thread::id()].enque(std::move(t));
			}

			void enque(std::vector<T>&& ts) {
				// TODO: append with single atomic increment.
				for(auto&& t : ts)
					m_subques[rynx::this_thread::id()].enque(std::move(t));
			}
			
			bool deque(T& t) {
				auto current = rynx::this_thread::id();
				for (int i = 0; i < MaxThreads; ++i) {
					if (m_subques[(current + i) % MaxThreads].deque(t)) {
						return true;
					}
				}
				return false;
			}

			bool empty() const {
				uint32_t count = 0;
				for (auto&& que : m_subques) {
					count += que.size();
				}
				return count == 0;
			}

		private:
			std::array<per_thread_queue, MaxThreads> m_subques;
		};
	}
}
