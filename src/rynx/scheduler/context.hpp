
#pragma once

#include <rynx/scheduler/task.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/object_storage.hpp>
#include <rynx/tech/ecs.hpp>

#include <mutex>
#include <atomic>
#include <shared_mutex>

namespace rynx {
	namespace scheduler {
		class task_token;
		class task;
		class task_scheduler;
		class scoped_barrier_after;
		class scoped_barrier_before;

		// NOTE: adding new tasks is allowed while scheduler is running.
		class context {
			friend class rynx::scheduler::task_token;
			friend class rynx::scheduler::task_scheduler;
			friend class rynx::scheduler::task;
			
			friend class scoped_barrier_after;
			friend class scoped_barrier_before;

			using context_id = uint64_t;

			context_id m_id;
			rynx::type_index m_typeIndex;
			std::atomic<uint64_t> m_nextTaskId = 0;

			struct resource_state {
				std::atomic<int> readers = 0;
				std::atomic<int> writers = 0;

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

			rynx::unordered_map<uint64_t, task> m_tasks;
			std::vector<barrier> m_activeTaskBarriers; // barriers that depend on any task that is created while they are here.
			std::vector<barrier> m_activeTaskBarriers_Dependencies; // new tasks are not allowed to run until these barriers are complete.
			// rynx::unordered_map<uint64_t, resource_state> m_resource_counters;
			std::vector<resource_state> m_resource_counters;

			rynx::object_storage m_resources;
			std::mutex m_taskMutex;
			task_scheduler* m_scheduler = nullptr;

			// TODO: hide these as private.
		public:
			void release_resources(const operation_resources& resources) {
				for (uint64_t readResource : resources.read_requirements()) {
					m_resource_counters[readResource].readers.fetch_sub(1);
				}
				for (uint64_t writeResource : resources.write_requirements()) {
					m_resource_counters[writeResource].writers.fetch_sub(1);
				}
			}

			void reserve_resources(const operation_resources& resources) {
				for (uint64_t readResource : resources.read_requirements()) {
					m_resource_counters[readResource].readers.fetch_add(1);
				}
				for (uint64_t writeResource : resources.write_requirements()) {
					m_resource_counters[writeResource].writers.fetch_add(1);
				}
			}

		private:
			void erase_completed_parallel_for_tasks();
			[[nodiscard]] task findWork();
			[[nodiscard]] uint64_t nextTaskId() { return m_nextTaskId.fetch_add(1); }

		public:

			[[nodiscard]] bool resourcesAvailableFor(const task& t) const {
				int resources_in_use = 0;
				for (uint64_t readResource : t.resources().read_requirements()) {
					resources_in_use += m_resource_counters[readResource].writers.load();
				}
				for (uint64_t writeResource : t.resources().write_requirements()) {
					resources_in_use += m_resource_counters[writeResource].writers.load();
					resources_in_use += m_resource_counters[writeResource].readers.load();
				}
				return resources_in_use == 0;
			}

			context(context_id id, task_scheduler* scheduler) : m_id(id), m_scheduler(scheduler) {
				m_resource_counters.resize(1024);
				set_resource<context>(this);
			}

			[[nodiscard]] bool isFinished() const { return m_tasks.empty(); }

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

			template<typename F> [[nodiscard]] task make_task(std::string taskName, F&& op) { return task(*this, nextTaskId(), std::move(taskName), std::forward<F>(op)); }
			[[nodiscard]] task make_task(std::string taskName) { return task(*this, nextTaskId(), std::move(taskName)); }

			task_token add_task(task task) {
				return task_token(std::move(task));
			}

			template<typename F>
			task_token add_task(std::string taskName, F&& taskOp) {
				return add_task(task(*this, nextTaskId(), std::move(taskName), std::forward<F>(taskOp)));
			}

			void schedule_task(task task) {
				std::lock_guard<std::mutex> lock(m_taskMutex);
				uint64_t taskId = task.id();
				m_tasks.emplace(taskId, std::move(task));
			}

			void dump();
		};
	}
}