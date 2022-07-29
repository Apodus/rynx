
#include <rynx/profiling/profiling.hpp>
#include <rynx/std/unordered_map.hpp>
#include <rynx/thread/this_thread.hpp>

#include <array>
#include <atomic>
#include <mutex>
#include <sstream>
#include <chrono>
#include <sstream>
#include <fstream>

namespace rynx {
	namespace profiling {
		namespace internal {
			struct duration_event {
				duration_event() {
					m_time_point = 0;
					m_thread_id = 0;
					m_pid = 0;
				}

				rynx::string m_name;
				rynx::string m_category;

				double m_time_point = 0;
				uint64_t m_thread_id = 0;
				uint64_t m_pid = 0;
				char m_type = 0;

				operator bool() const {
					return !(m_time_point == 0 && m_thread_id == 0);
				}

				operator rynx::string() const {
					return "{\"name\": \"" + rynx::string(m_name) +
						"\", \"cat\" : \"" + rynx::string(m_category) +
						"\", \"ph\" : \"" + m_type + "\", \"ts\" : " + rynx::to_string(m_time_point) +
						", \"pid\" : " + rynx::to_string(m_pid) +
						", \"tid\" : " + rynx::to_string(m_thread_id) +
						", \"args\" : {}}";
				}
			};
		}

		constexpr size_t profiling_storage_size = 1024 * 1024;
		std::array<internal::duration_event, profiling_storage_size>* g_profiling_storage = nullptr;
		std::atomic<uint64_t> g_event_counter = 0;
		std::mutex m_init_mutex;
		double g_start_time = 0;

		double current_time() {
			return std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000.0;
		}

		void init_for_this_thread() {
			if (!g_profiling_storage) {
				std::unique_lock lock(m_init_mutex);
				if (!g_profiling_storage) {
					g_start_time = current_time();
					g_profiling_storage = new std::array<internal::duration_event, profiling_storage_size>();
				}
			}
		}

		void push_event_begin(
			rynx::string name,
			rynx::string category,
			uint64_t pid
		) {
			init_for_this_thread();
			auto my_index = g_event_counter.fetch_add(1) % g_profiling_storage->size();
			auto* my_event = &(*g_profiling_storage)[my_index];
			my_event->m_thread_id = rynx::this_thread::id();
			my_event->m_name = std::move(name);
			my_event->m_category = std::move(category);
			my_event->m_pid = pid;
			my_event->m_time_point = current_time() - g_start_time;
			my_event->m_type = 'B';
		}

		void push_event_end(
			uint64_t pid
		) {
			auto my_index = g_event_counter.fetch_add(1) % g_profiling_storage->size();
			auto* my_event = &(*g_profiling_storage)[my_index];
			my_event->m_thread_id = rynx::this_thread::id();
			my_event->m_pid = pid;
			my_event->m_time_point = current_time() - g_start_time;
			my_event->m_type = 'E';
		}

		void write_profile_log() {
			if (!g_profiling_storage)
				return;

			if constexpr (enabled) {
				std::stringstream out;
				out << "{" << "\"traceEvents\": [";

				size_t first = (g_event_counter.load() + 1) % g_profiling_storage->size();
				
				while (!g_profiling_storage->operator[](first)) first = ++first% g_profiling_storage->size();
				size_t next = first;

				out << g_profiling_storage->operator[](next).operator rynx::string();
				++next;
				
				for (; next < g_profiling_storage->size(); ++next) {
					if(g_profiling_storage->operator[](next))
						out << ", " << g_profiling_storage->operator[](next).operator rynx::string();
				}
				for (size_t from_beginning = 0; from_beginning < first; ++from_beginning) {
					if(g_profiling_storage->operator[](from_beginning))
						out << ", " << g_profiling_storage->operator[](from_beginning).operator rynx::string();
				}

				out << "]}";

				std::ofstream outfile("profile.json");
				std::string s = out.str();
				outfile.write(s.c_str(), s.length());
			}
		}
	}
}
