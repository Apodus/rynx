
#include <rynx/std/frame_allocator.hpp>
#include <rynx/tech/parallel/queue.hpp>
#include <atomic>

namespace {
	constexpr size_t block_size = 4 * 1024 * 1024;
	template<size_t size> struct frame_allocator_block;
	frame_allocator_block<::block_size>* next_block();

	thread_local frame_allocator_block<::block_size>* thread_block = nullptr;

	template<size_t size>
	struct frame_allocator_block {
		std::byte buffer[size];
		int32_t m_head = 0;
		bool in_use = false;

		void* allocate(size_t bytes, size_t align) {
			auto over = (uint64_t(buffer) + m_head) % align;
			if (over)
				m_head += int32_t(align - over);
			if (size - m_head >= bytes) {
				void* result = buffer + m_head;
				m_head += int32_t(bytes);
				return result;
			}
			return nullptr;
		}
	};

	namespace frame_allocator_impl {
		::frame_allocator_block<block_size>* frame_allocator_blocks[1024] = {0,};
		std::atomic<int32_t> m_active_block = 0;
		rynx::parallel::queue<void*, 1024> large_allocs;
		
		void next_block() {
			if (thread_block)
				thread_block->in_use = false;
			
			while (true) {
				int32_t block_index = m_active_block.fetch_add(1);
				::frame_allocator_block<::block_size>* block_to_try = frame_allocator_blocks[block_index];
				if (block_to_try) {
					if (block_to_try->m_head == 0 && !block_to_try->in_use) {
						block_to_try->in_use = true;
						thread_block = block_to_try;
						return;
					}
					continue; // if block already used, get another one.
				}
				else {
					block_to_try = new ::frame_allocator_block<::block_size>();
					block_to_try->in_use = true;
					frame_allocator_blocks[block_index] = block_to_try;
					thread_block = block_to_try;
					return;
				}
			}
		}
	};
}

void* rynx::memory::frame_allocator::detail::allocate(size_t bytes, size_t align) {
	if (bytes + align > ::block_size) {
		// alloc is larger than our blocks.
		// perform a direct alloc instead, and store it in large allocs table.
		auto* result = new std::byte[bytes + align];
		::frame_allocator_impl::large_allocs.enque(static_cast<void*>(result));
		if (uint64_t(result) % align != 0)
			return result + align - (uint64_t(result) % align);
		return result;
	}
	
	if (!thread_block) {
		::frame_allocator_impl::next_block();
	}

	void* result = nullptr;
	do {
		result = thread_block->allocate(bytes, align);
		if (!result)
			::frame_allocator_impl::next_block();
	} while (result == nullptr);
	return result;
}

void rynx::memory::frame_allocator::start_frame() {
	for (auto* block_ptr : ::frame_allocator_impl::frame_allocator_blocks) {
		if (!block_ptr)
			break;
		block_ptr->m_head = 0;
	}
	
	// free large allocs
	{
		void* ptr;
		while (::frame_allocator_impl::large_allocs.deque(ptr)) {
			delete[] static_cast<std::byte*>(ptr);
		}
	}

	::frame_allocator_impl::m_active_block.store(0);
}