
#include <rynx/thread/this_thread.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>

namespace {
	std::atomic<uint64_t> g_tid_counter = 0;
	rynx::unordered_map<std::thread::id, uint64_t> g_thread_id_map;
	thread_local uint64_t t_my_tid = ~uint64_t(0);
	std::vector<uint64_t> g_free_tids;
	std::mutex m_init_mutex;
}

void init_for_this_thread() {
	std::unique_lock lock(m_init_mutex);
	uint64_t my_tid;
	if (!g_free_tids.empty()) {
		my_tid = g_free_tids.back();
		g_free_tids.pop_back();
	}
	else {
		my_tid = g_tid_counter.fetch_add(1);
	}
	g_thread_id_map.insert_or_assign(std::this_thread::get_id(), my_tid);
	t_my_tid = my_tid;
}

void release_for_this_thread() {
	std::unique_lock lock(m_init_mutex);
	g_thread_id_map.erase(std::this_thread::get_id());
	g_free_tids.emplace_back(t_my_tid);
}

int64_t rynx::this_thread::id() {
	rynx_assert(t_my_tid != ~uint64_t(0), "this thread has not initialized for use with rynx thread ids. create a rynx::this_thread::rynx_thread_raii object in your thread for the duration for which rynx id services are required.");
	return t_my_tid;
}

rynx::this_thread::rynx_thread_raii::rynx_thread_raii() {
	init_for_this_thread();
}

rynx::this_thread::rynx_thread_raii::~rynx_thread_raii() {
	release_for_this_thread();
}