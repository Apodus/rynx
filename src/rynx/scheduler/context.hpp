
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
		struct task_token;
		class task;
		class scoped_barrier_after;
		class scoped_barrier_before;

		// NOTE: adding new tasks is allowed while scheduler is running.
		class context {
			friend struct rynx::scheduler::task_token;
			friend class rynx::scheduler::task;
			friend class rynx::scheduler::task_scheduler;

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
			rynx::unordered_map<uint64_t, resource_state> m_resource_counters;

			rynx::object_storage m_resources;
			std::mutex m_taskMutex;
			std::shared_mutex mutable m_resourceMutex;

			void release_resources(const task& task) {
				std::shared_lock lock(m_resourceMutex);
				for (uint64_t readResource : task.resources().readRequirements()) {
					auto it = m_resource_counters.find(readResource);
					rynx_assert(it != m_resource_counters.end(), "task is using a resource that has not been set.");
					--it->second.readers;
				}
				for (uint64_t writeResource : task.resources().writeRequirements()) {
					auto it = m_resource_counters.find(writeResource);
					rynx_assert(it != m_resource_counters.end(), "task is using a resource that has not been set.");
					--it->second.writers;
				}
			}

			void reserve_resources(const task& task) {
				std::shared_lock lock(m_resourceMutex);
				for (uint64_t readResource : task.resources().readRequirements()) {
					auto it = m_resource_counters.find(readResource);
					if (it == m_resource_counters.end()) {
						// new resource kind.
						lock.unlock();
						{
							// it is possible that multiple threads execute this path consecutively, but that's ok because
							// the latter emplaces will not replace the value, and all increments will be stored.
							std::lock_guard write_lock(m_resourceMutex);
							auto emplace_result = m_resource_counters.emplace(readResource, resource_state());
							++emplace_result.first->second.readers;
						}
						lock.lock();
					}
					else {
						++it->second.readers;
					}
				}
				for (uint64_t writeResource : task.resources().writeRequirements()) {
					auto it = m_resource_counters.find(writeResource);
					if (it == m_resource_counters.end()) {
						// new resource kind.
						lock.unlock();
						{
							std::lock_guard write_lock(m_resourceMutex);
							auto emplace_result = m_resource_counters.emplace(writeResource, resource_state());
							++emplace_result.first->second.writers;
						}
						lock.lock();
					}
					else {
						++it->second.writers;
					}
				}
			}

			task findWork();
			uint64_t nextTaskId() { return m_nextTaskId.fetch_add(1); }

		public:

			bool resourcesAvailableFor(const task& t) const {
				bool okToStart = true;
				std::shared_lock lock(m_resourceMutex);
				
				for (uint64_t readResource : t.resources().readRequirements()) {
					auto it = m_resource_counters.find(readResource);
					if (it != m_resource_counters.end()) {
						okToStart &= (it->second.writers == 0);
					}
				}
				for (uint64_t writeResource : t.resources().writeRequirements()) {
					auto it = m_resource_counters.find(writeResource);
					if (it != m_resource_counters.end()) {
						auto& resourceState = it->second;
						okToStart &= ((resourceState.readers == 0 && resourceState.writers == 0));
					}
				}
				return okToStart;
			}

			context(context_id id) : m_id(id) {
				set_resource<context>(this);
			}

			bool isFinished() const { return m_tasks.empty(); }

			template<typename T> context& set_resource(T* t) {
				std::lock_guard lock(m_resourceMutex);

				m_resources.set_and_discard(t);
				uint64_t type_id = m_typeIndex.id<std::remove_cvref_t<T>>();
				auto emplace_result = m_resource_counters.emplace(type_id, resource_state());
				if (!emplace_result.second) {
					rynx_assert(
						emplace_result.first->second.readers == 0 &&
						emplace_result.first->second.writers == 0,
						"setting a resource for scheduler context, while the previous instance of the resource is in use! not ok."
					);
				}

				return *this;
			}

			template<typename T> T& get_resource() {
				return m_resources.get<T>();
			}

			template<typename F> task make_task(std::string taskName, F&& op) { return task(*this, nextTaskId(), std::move(taskName), std::forward<F>(op)); }
			task make_task(std::string taskName) { return task(*this, nextTaskId(), std::move(taskName)); }

			task_token add_task(task task) {
				std::lock_guard<std::mutex> lock(m_taskMutex);
				uint64_t taskId = task.id();
				m_tasks.emplace(taskId, std::move(task));
				return task_token(this, taskId);
			}

			template<typename F>
			task_token add_task(std::string taskName, F&& taskOp) {
				return add_task(task(*this, nextTaskId(), std::move(taskName), std::forward<F>(taskOp)));
			}

			void dump();
		};
	}
}