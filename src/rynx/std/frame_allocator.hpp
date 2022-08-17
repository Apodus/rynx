
#pragma once

#include <cstdint>
#include <type_traits>

// addressof implementation (to avoid including <memory>)
namespace rynx {
	template<class T> typename std::enable_if<std::is_object<T>::value, T*>::type addressof(T& arg) noexcept { return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(arg))); }
	template<class T> typename std::enable_if<!std::is_object<T>::value, T*>::type addressof(T& arg) noexcept { return &arg; }
}

namespace rynx::memory {
	namespace frame_allocator {
		namespace detail {
			void* allocate(size_t bytes, size_t align);
		}

		void start_frame();

		template<typename T>
		struct allocator_t {
			using value_type = T;
			using pointer = T*;
			using reference = T&;
			using const_reference = const T&;
			using const_pointer = const T*;
			using propagate_on_container_move_assignment = std::true_type;
			using size_type = size_t;
			using is_always_equal = std::true_type;
			using difference_type = std::ptrdiff_t;

			allocator_t() {}
			template<typename U> allocator_t(allocator_t<U>) {}

			// c++23, no need to support for now
			// [[nodiscard]] constexpr std::allocation_result<T*> allocate_at_least(std::size_t n);
			[[nodiscard]] static constexpr T* allocate(size_t n) {
				return static_cast<T*>(rynx::memory::frame_allocator::detail::allocate(n * sizeof(T), alignof(T)));
			}

			static constexpr void deallocate(pointer, size_t) {}

			static pointer address(reference x) noexcept { return rynx::addressof(x); }
			static const_pointer address(const_reference x) noexcept { return rynx::addressof(x); }
			static size_type max_size() noexcept { return 1 * 1024 * 1024 * 1024; } // report a 1 GiB maximum alloc size.
		};

		template<typename T> T* construct(size_t n = 1) {
			auto* memory = ::rynx::memory::frame_allocator::allocator_t<T>::allocate(n);
			auto* memory_it = memory;
			if constexpr (std::is_object_v<T>)
				do { new (memory_it++) T(); } while (--n);
			return memory;
		}
		template<typename T> void destruct(T* ptr, size_t n) {
			if constexpr (std::is_object_v<T>)
				do { (*ptr++).~T(); } while (--n);
		}

		static void* malloc(size_t bytes, size_t align) { return ::rynx::memory::frame_allocator::detail::allocate(bytes, align); } // no need to call free for frame_malloced memory.
	}
}

// not sure what this is used for in std containers etc. the memory source for all frame allocators is the same, and deallocate is no-op, so probably this doesn't really matter anyway.
template<class T1, class T2> constexpr bool operator==(const rynx::memory::frame_allocator::allocator_t<T1>& lhs, const rynx::memory::frame_allocator::allocator_t<T2>& rhs) noexcept { return true; }