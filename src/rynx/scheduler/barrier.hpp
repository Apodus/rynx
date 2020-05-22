#pragma once

#include <string>
#include <atomic>
#include <memory>
#include <vector>

#include <rynx/system/assert.hpp>

namespace rynx {
	namespace scheduler {
		class context;

		struct barrier {
			barrier() : barrier("anon") {}
			barrier(std::string name) : name(std::move(name)), counter(std::make_shared<std::atomic<int32_t>>(0)) {}
			barrier(const barrier& other) = default;
			barrier(barrier&& other) = default;
			barrier& operator = (const barrier& other) = default;
			barrier& operator = (barrier&& other) = default;

			void busy_wait() const {
				while(!*this) {}
				return;
			}

			operator bool() const { return counter->load(std::memory_order_acquire) == 0; }

			std::string name;
			std::shared_ptr<std::atomic<int32_t>> counter; // counter object is shared by all copies of the same barrier.
		};

		class operation_barriers {
		public:
			operation_barriers() = default;
			operation_barriers(operation_barriers&&) = default;
			operation_barriers(const operation_barriers& other) {
				m_requires = other.m_requires;
				m_blocks = other.m_blocks;

				for (auto&& bar : m_blocks)
					++*bar.counter;
			}

			operation_barriers& operator =(operation_barriers&&) = default;

			operation_barriers& depends_on(barrier bar) {
				m_requires.emplace_back(std::move(bar));
				return *this;
			}

			operation_barriers& required_for(barrier bar) {
				for (auto&& extension : m_extensions)
				{
					auto shared_extension = extension.lock();
					if (shared_extension)
						shared_extension->required_for(bar);
				}
				++*bar.counter;
				m_blocks.emplace_back(std::move(bar));
				return *this;
			}

			void dump() const;

			bool can_start() {
				while (!m_requires.empty()) {
					if (m_requires.front()) {
						m_requires[0] = std::move(m_requires.back()); // requirement met. erase it so we don't check it again.
						m_requires.pop_back();
					}
					else {
						return false;
					}
				}
				return true;
			}

			bool blocks_other_tasks() const {
				return !m_blocks.empty();
			}

			void on_complete() {
				for (auto&& bar : m_blocks) {
					bar.counter->fetch_sub(1, std::memory_order_release);
				}
			}

			// in cases where we are running a task, and notice that there is some additional work with different resources
			// that must be done before we can say the first task is complete, we can say completion_blocked_by(other)
			void completion_blocked_by(std::shared_ptr<operation_barriers>& other) {
				m_extensions.emplace_back(other);
				for (auto& bar : m_blocks) {
					other->required_for(bar);
				}
			}

		private:
			std::vector<barrier> m_requires; // barriers that must be completed before starting this task.
			std::vector<barrier> m_blocks; // barriers that are not complete without this task.
			std::vector<std::weak_ptr<operation_barriers>> m_extensions; // operations that extend this operation instance.
		};

		class operation_resources {
			std::vector<uint64_t> m_writeAccess;
			std::vector<uint64_t> m_readAccess;

		public:
			operation_resources& require_write(uint64_t resourceId) {
				m_writeAccess.emplace_back(resourceId);
				return *this;
			}

			operation_resources& require_read(uint64_t resourceId) {
				m_readAccess.emplace_back(resourceId);
				return *this;
			}

			const std::vector<uint64_t>& read_requirements() const { return m_readAccess; }
			const std::vector<uint64_t>& write_requirements() const { return m_writeAccess; }

			bool empty() const { return m_readAccess.empty() & m_writeAccess.empty(); }
		};

		// this barrier is attached to end of any new task added to scheduler while in scope.
		// this barrier depends on tasks created.
		class scoped_barrier_after {
		public:
			scoped_barrier_after(scheduler::context& context, rynx::scheduler::barrier bar);
			~scoped_barrier_after();
		private:
			scheduler::context* m_context;
		};

		// this barrier is attached to front of any new task added to scheduler while in scope.
		// created tasks depend on this barrier.
		class scoped_barrier_before {
		public:
			scoped_barrier_before(scheduler::context& context) : m_context(&context) {}

			~scoped_barrier_before();
			scoped_barrier_before& addDependency(rynx::scheduler::barrier bar);

		private:
			scheduler::context* m_context;
			int m_dependenciesAdded = 0;
		};
	}

	// TODO: Delete these. We should never spin.
	template<typename T>
	void wait_spin(std::vector<T>& availabilities) {
		while (!availabilities.empty()) {
			while (!availabilities.back()) {}
			availabilities.pop_back();
		}
	}

	class spin_waiter {
	public:
		template <typename T>
		spin_waiter& operator | (T& availability) {
			while (!availability) {}
			return *this;
		}

		template <typename T>
		spin_waiter& operator | (std::vector<T>& availabilities) {
			while (!availabilities.empty()) {
				operator | (availabilities.back());
				availabilities.pop_back();
			}
			return *this;
		}
	};
}
