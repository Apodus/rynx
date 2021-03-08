
#pragma once

#include <rynx/scheduler/barrier.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/object_storage.hpp>
#include <rynx/tech/ecs.hpp>
#include <rynx/math/random.hpp>

#include <rynx/tech/parallel/queue.hpp>
#include <rynx/tech/binary_config.hpp>

#include <mutex>
#include <atomic>
#include <shared_mutex>
#include <iostream>

namespace rynx {

	namespace scheduler {
		class task_token;
		class task;
		class task_scheduler;
		class scoped_barrier_after;
		class scoped_barrier_before;

		class context {
			friend class rynx::scheduler::task_token;
			friend class rynx::scheduler::task_scheduler;
			friend class rynx::scheduler::task;
			
			friend class scoped_barrier_after;
			friend class scoped_barrier_before;

			using context_id = uint64_t;
			
			context_id m_id;
			rynx::type_index m_typeIndex;
			
			struct resource_state {
				alignas(std::hardware_destructive_interference_size) std::atomic<int> readers = 0;
				alignas(std::hardware_destructive_interference_size) std::atomic<int> writers = 0;

				resource_state() = default;
				resource_state(resource_state&& other) noexcept :
					readers(other.readers.load()),
					writers(other.writers.load())
				{}

				resource_state& operator = (resource_state&& other) noexcept {
					readers.store(other.readers.load());
					writers.store(other.writers.load());
					return *this;
				}
			};

			template<typename T>
			struct resource_getter {
				T& operator()(context* context_) {
					return context_->get_resource<std::remove_reference_t<T>>();
				}
			};

			template<typename...Args>
			struct resource_getter<rynx::ecs::view<Args...>> {
				rynx::ecs& operator()(context* context_) {
					return context_->get_resource<rynx::ecs>();
				}
			};

			template<typename...Args>
			struct resource_getter<rynx::ecs::edit_view<Args...>> {
				rynx::ecs& operator()(context* context_) {
					return context_->get_resource<rynx::ecs>();
				}
			};

			template<typename T>
			auto& getTaskArgParam() {
				return resource_getter<T>()(this);
			}

			void task_finished() {
				--m_task_counter;
			}

			math::rand64 m_random;
			std::atomic<int32_t> m_task_counter = 0;
			std::atomic<int32_t> m_tasks_per_frame = 0;

			std::vector<task> m_tasks;
			rynx::parallel::queue<task, 1024> m_tasks_parallel_for;

			std::vector<barrier> m_activeTaskBarriers; // barriers that depend on any task that is created while they are here.
			std::vector<barrier> m_activeTaskBarriers_Dependencies; // new tasks are not allowed to run until these barriers are complete.
			
			std::vector<resource_state> m_resource_counters;

			rynx::object_storage m_resources;
			
			std::mutex m_taskMutex;
			task_scheduler* m_scheduler = nullptr;

			rynx::binary_config m_execution_state;

			// TODO: hide these as private.
		public:
			void release_resources(const operation_resources& resources) {
				for (uint64_t readResource : resources.read_requirements()) {
					rynx_assert(readResource < m_resource_counters.size(), "out of bounds");
					m_resource_counters[readResource].readers.fetch_sub(1);
				}
				for (uint64_t writeResource : resources.write_requirements()) {
					rynx_assert(writeResource < m_resource_counters.size(), "out of bounds");
					m_resource_counters[writeResource].writers.fetch_sub(1);
				}
			}

			void reserve_resources(const operation_resources& resources) {
				for (uint64_t readResource : resources.read_requirements()) {
					rynx_assert(readResource < m_resource_counters.size(), "out of bounds");
					m_resource_counters[readResource].readers.fetch_add(1);
				}
				for (uint64_t writeResource : resources.write_requirements()) {
					rynx_assert(writeResource < m_resource_counters.size(), "out of bounds");
					m_resource_counters[writeResource].writers.fetch_add(1);
				}
			}

		private:
			[[nodiscard]] task findWork();

		public:

			rynx::binary_config& access_state() { return m_execution_state; }

			[[nodiscard]] bool resourcesAvailableFor(const task& t) const;

			context(context_id id, task_scheduler* scheduler);

			[[nodiscard]] bool isFinished() const {
				// return m_tasks.empty() & m_tasks_parallel_for.empty();
				return m_task_counter.load() == 0;
			}

			template<typename T> context& set_resource(T* t) {
				m_resources.set_and_discard(t);
				uint64_t type_id = m_typeIndex.id<std::remove_cvref_t<T>>();
				rynx_assert(
					m_resource_counters[type_id].readers == 0 &&
					m_resource_counters[type_id].writers == 0,
					"setting a resource for scheduler context, while the previous instance of the resource is in use! not ok."
				);
				return *this;
			}

			template<typename T> T& get_resource() {
				return m_resources.get<T>();
			}

			task_token add_task(task task);

			template<typename F> task_token add_task(std::string taskName, F&& taskOp);

			void schedule_task(task task);
			void dump();
		};
	}
}

#include <rynx/scheduler/task.hpp>

template<typename F> rynx::scheduler::task_token rynx::scheduler::context::add_task(std::string taskName, F&& taskOp) {
	return add_task(task(*this, std::move(taskName), std::forward<F>(taskOp)));
}