#pragma once

#include <rynx/system/assert.hpp>
#include <rynx/std/function.hpp> // for opaque_unique_ptr deleter. maybe just allow dynamic deleter for unique ptr?

#include <cstddef>
#include <type_traits>
#include <atomic>

#define USE_RYNX_SMART_PTRS

#ifndef USE_RYNX_SMART_PTRS
#include <memory>
#endif

namespace rynx {

    template <class _Ty, class _Other = _Ty>
    constexpr _Ty exchange(_Ty& _Val, _Other&& _New_val) noexcept(
        std::conjunction_v<std::is_nothrow_move_constructible<_Ty>, std::is_nothrow_assignable<_Ty&, _Other>>) {
        _Ty _Old_val = static_cast<_Ty&&>(_Val);
        _Val = static_cast<_Other&&>(_New_val);
        return _Old_val;
    }

    namespace std_replacements {

        struct dynamic_deleter {
            virtual ~dynamic_deleter() {}
            virtual void operator()(void*) = 0;
        };
        
        template <class element_t>
        struct default_delete : dynamic_deleter {
            constexpr default_delete() noexcept = default;

            template <class convertible_element_t, std::enable_if_t<std::is_convertible_v<convertible_element_t*, element_t*>, int> = 0>
            default_delete(const default_delete<convertible_element_t>&) noexcept {}

            void operator()(void* ptr) {
                static_assert(0 < sizeof(element_t), "can't delete an incomplete type");
                delete static_cast<element_t*>(ptr);
            }
        };

        template <class element_t>
        struct default_delete<element_t[]> {
            constexpr default_delete() noexcept = default;

            template <class convertible_element_t, std::enable_if_t<std::is_convertible_v<convertible_element_t(*)[], element_t(*)[]>, int> = 0>
            default_delete(const default_delete<convertible_element_t[]>&) noexcept {}

            template <class convertible_element_t, std::enable_if_t<std::is_convertible_v<convertible_element_t(*)[], element_t(*)[]>, int> = 0>
            void operator()(convertible_element_t* _Ptr) const noexcept {
                static_assert(0 < sizeof(convertible_element_t), "can't delete an incomplete type");
                delete[] _Ptr;
            }
        };

        template <class element_t, class element_t_noref, class = void>
        struct unique_ptr_get_deleter_pointer_type {
            using type = element_t*;
        };

        template <class element_t, class element_t_noref>
        struct unique_ptr_get_deleter_pointer_type<element_t, element_t_noref, std::void_t<typename element_t_noref::pointer>> {
            using type = typename element_t_noref::pointer;
        };

        template <class convertible_element_deleter_t>
        using unique_ptr_enable_default_t =
            std::enable_if_t<std::conjunction_v<std::negation<std::is_pointer<convertible_element_deleter_t>>, std::is_default_constructible<convertible_element_deleter_t>>, int>;

        template <class element_t, class deleter_t = default_delete<element_t>>
        class unique_ptr {
        public:
            using pointer = typename unique_ptr_get_deleter_pointer_type<element_t, std::remove_reference_t<deleter_t>>::type;
            using element_type = element_t;
            using deleter_type = deleter_t;

            template <class convertible_element_deleter_t = deleter_t, unique_ptr_enable_default_t<convertible_element_deleter_t> = 0>
            constexpr unique_ptr() noexcept {};

            template <class convertible_element_deleter_t = deleter_t, unique_ptr_enable_default_t<convertible_element_deleter_t> = 0>
            constexpr unique_ptr(nullptr_t) noexcept : unique_ptr() {}

            unique_ptr& operator=(nullptr_t) noexcept {
                reset();
                return *this;
            }

            template <class convertible_element_deleter_t = deleter_t, unique_ptr_enable_default_t<convertible_element_deleter_t> = 0>
            explicit unique_ptr(pointer _Ptr) noexcept : m_deleter(), m_ptr(_Ptr) {}

            template <class convertible_element_deleter_t = deleter_t, std::enable_if_t<std::is_constructible_v<deleter_t, const convertible_element_deleter_t&>, int> = 0>
            unique_ptr(pointer _Ptr, const convertible_element_deleter_t& _Dt) noexcept : m_deleter(std::move(_Dt)), m_ptr(_Ptr) {}

            template <class convertible_element_deleter_t = deleter_t,
                std::enable_if_t<std::conjunction_v<std::negation<std::is_reference<convertible_element_deleter_t>>, std::is_constructible<deleter_t, convertible_element_deleter_t>>, int> = 0>
            unique_ptr(pointer _Ptr, convertible_element_deleter_t&& _Dt) noexcept : m_deleter(std::move(_Dt)), m_ptr(_Ptr) {}

            template <class convertible_element_deleter_t = deleter_t,
                std::enable_if_t<std::conjunction_v<std::is_reference<convertible_element_deleter_t>, std::is_constructible<deleter_t, std::remove_reference_t<convertible_element_deleter_t>>>, int> = 0>
            unique_ptr(pointer, std::remove_reference_t<convertible_element_deleter_t>&&) = delete;

            unique_ptr(unique_ptr&& other) noexcept {
                m_deleter = other.m_deleter;
                m_ptr = other.release();
            }

            template <class other_deleter_t = deleter_t>
            unique_ptr(unique_ptr<element_t, other_deleter_t>&& other) noexcept
                : m_deleter(std::forward<deleter_t>(other.m_deleter)), m_ptr(other.release()) {}

            template <class other_element_t, class other_deleter_t>
            unique_ptr(unique_ptr<other_element_t, other_deleter_t>&& other) noexcept
                : m_deleter(std::forward<other_deleter_t>(other.m_deleter)), m_ptr(other.release()) {}

            template <class other_element_t, class other_deleter_t>
            unique_ptr& operator=(unique_ptr<other_element_t, other_deleter_t>&& other) noexcept {
                reset(other.release());
                m_deleter = std::forward<other_deleter_t>(other.m_deleter);
                return *this;
            }

            template <class convertible_element_deleter_t = deleter_t, std::enable_if_t<std::is_move_assignable_v<convertible_element_deleter_t>, int> = 0>
            unique_ptr& operator=(unique_ptr<element_t, convertible_element_deleter_t>&& other) noexcept {
                rynx_assert(this != &other, "move to self not allowed");
                reset(other.release());
                m_deleter = std::forward<deleter_t>(other.m_deleter);
                return *this;
            }

            bool operator ==(nullptr_t) const noexcept {
                return m_ptr == nullptr;
            }

            bool operator !=(nullptr_t) const noexcept {
                return m_ptr != nullptr;
            }

            template<typename other_element_t>
            bool operator == (other_element_t const* const v) const noexcept {
                return m_ptr == v;
            }

            template<typename other_element_t>
            bool operator != (other_element_t const* const v) const noexcept {
                return m_ptr != v;
            }

            void swap(unique_ptr& other) noexcept {
                std::swap(m_ptr, other.m_ptr);
                std::swap(m_deleter, other.m_deleter);
            }

            ~unique_ptr() noexcept {
                if (m_ptr) {
                    m_deleter(m_ptr);
                }
            }

            [[nodiscard]] deleter_t& get_deleter() noexcept {
                return m_deleter;
            }

            [[nodiscard]] const deleter_t& get_deleter() const noexcept {
                return m_deleter;
            }

            [[nodiscard]] std::add_lvalue_reference_t<element_t> operator*() const noexcept(noexcept(*std::declval<pointer>())) {
                return *m_ptr;
            }

            [[nodiscard]] pointer operator->() const noexcept {
                return m_ptr;
            }

            [[nodiscard]] pointer get() const noexcept {
                return m_ptr;
            }

            explicit operator bool() const noexcept {
                return static_cast<bool>(m_ptr);
            }

            pointer release() noexcept {
                return rynx::exchange(m_ptr, nullptr);
            }

            void reset(pointer ptr = nullptr) noexcept {
                pointer old = rynx::exchange(m_ptr, ptr);
                if (old) {
                    m_deleter(old);
                }
            }

            unique_ptr(const unique_ptr&) = delete;
            unique_ptr& operator=(const unique_ptr&) = delete;

        private:
            template <class, class>
            friend class unique_ptr;

            deleter_t m_deleter;
            pointer m_ptr = nullptr;
        };

        template <class element_t, class deleter_t>
        class unique_ptr<element_t[], deleter_t> {
        public:
            using pointer = typename unique_ptr_get_deleter_pointer_type<element_t, std::remove_reference_t<deleter_t>>::type;
            using element_type = element_t;
            using deleter_type = deleter_t;

            template <class deleter_t2 = deleter_t>
            constexpr unique_ptr() noexcept : m_deleter(), m_ptr(nullptr) {}

            template <class other_ptr_t, class other_deleter_t = deleter_t>
            explicit unique_ptr(other_ptr_t ptr) noexcept : m_deleter(), m_ptr(ptr) {}

            template <class other_ptr_t, class other_deleter_t = deleter_t>
            unique_ptr(other_ptr_t ptr, const other_deleter_t& deleter) noexcept : m_deleter(deleter), m_ptr(ptr) {}

            template <class other_ptr_t, class other_deleter_t = deleter_t>
            unique_ptr(other_ptr_t ptr, other_deleter_t&& deleter) noexcept : m_deleter(std::move(deleter)), m_ptr(ptr) {}

            template <class other_ptr_t, class other_deleter_t = deleter_t>
            unique_ptr(other_ptr_t ptr, std::remove_reference_t<other_deleter_t>&&) = delete;

            template <class other_deleter_t = deleter_t, std::enable_if_t<std::is_move_constructible_v<other_deleter_t>, int> = 0>
            unique_ptr(unique_ptr<element_t, other_deleter_t>&& other) noexcept
                : m_deleter(std::forward<deleter_t>(other.get_deleter())), m_ptr(other.release()) {}

            template <class other_deleter_t = deleter_t, std::enable_if_t<std::is_move_assignable_v<other_deleter_t>, int> = 0>
            unique_ptr& operator=(unique_ptr<element_t, other_deleter_t>&& other) noexcept {
                rynx_assert(this != &other, "can't move to same object");
                reset(other.release());
                m_deleter = std::move(other.m_deleter);
                return *this;
            }

            template <class other_ptr_t, class other_deleter_t>
            unique_ptr(unique_ptr<other_ptr_t, other_deleter_t>&& other) noexcept
                : m_deleter(std::forward<other_deleter_t>(other.get_deleter())), m_ptr(other.release()) {}

            template <class other_ptr_t, class other_deleter_t>
            unique_ptr& operator=(unique_ptr<other_ptr_t, other_deleter_t>&& other) noexcept {
                reset(other.release());
                m_deleter = std::forward<other_deleter_t>(other.m_deleter);
                return *this;
            }

            template <class other_deleter_t = deleter_t>
            constexpr unique_ptr(nullptr_t) noexcept : m_deleter(), m_ptr(nullptr) {}

            unique_ptr& operator=(nullptr_t) noexcept {
                reset();
                return *this;
            }

            void reset(nullptr_t = nullptr) noexcept {
                reset(pointer());
            }

            void swap(unique_ptr& other) noexcept {
                std::swap(m_ptr, other.m_ptr);
                std::swap(m_deleter, other.m_deleter);
            }

            ~unique_ptr() noexcept {
                if (m_ptr) {
                    m_deleter(m_ptr);
                }
            }

            [[nodiscard]] deleter_t& get_deleter() noexcept {
                return m_deleter;
            }

            [[nodiscard]] const deleter_t& get_deleter() const noexcept {
                return m_deleter;
            }

            [[nodiscard]] element_t& operator[](size_t _Idx) const noexcept {
                return m_ptr[_Idx];
            }

            [[nodiscard]] pointer get() const noexcept {
                return m_ptr;
            }

            explicit operator bool() const noexcept {
                return static_cast<bool>(m_ptr);
            }

            pointer release() noexcept {
                return rynx::exchange(m_ptr, nullptr);
            }

            template <class _Uty>
            void reset(_Uty ptr) noexcept {
                pointer old = rynx::exchange(m_ptr, ptr);
                if (old) {
                    m_deleter(old);
                }
            }

            unique_ptr(const unique_ptr&) = delete;
            unique_ptr& operator=(const unique_ptr&) = delete;

        private:
            template <class, class>
            friend class unique_ptr;

            deleter_t m_deleter;
            pointer m_ptr;
        };



        // Shared ptr

        struct shared_ptr_control_block {
            std::atomic<int32_t> m_count = 1;
            std::atomic<int32_t> m_weak = 1;
            rynx::std_replacements::unique_ptr<rynx::std_replacements::dynamic_deleter> m_deleter;
        };

        template <typename T>
        class weak_ptr;

        template <typename T>
        class shared_ptr {
            template<typename U> friend class rynx::std_replacements::weak_ptr;
            template<typename U> friend class rynx::std_replacements::shared_ptr;

            T* m_ptr = nullptr;
            rynx::std_replacements::shared_ptr_control_block* m_control = nullptr;

            void release() {
                if (m_control && m_control->m_count.fetch_add(-1) == 1) {
                    m_control->m_deleter->operator()(m_ptr);
                    if (m_control->m_weak.fetch_add(-1) == 1)
                        delete m_control;
                }

                m_ptr = nullptr;
                m_control = nullptr;
            }

            inline void acquire() {
                if (m_control)
                    ++m_control->m_count;
            }

            shared_ptr(T* ptr, rynx::std_replacements::shared_ptr_control_block* control) : m_ptr(ptr), m_control(control) {}

        public:
            shared_ptr() {}
            shared_ptr(nullptr_t) {}
            template<typename U>
            shared_ptr(U* _ptr) {
                m_ptr = static_cast<T*>(_ptr);
                m_control = new rynx::std_replacements::shared_ptr_control_block();
                m_control->m_deleter = rynx::std_replacements::unique_ptr<rynx::std_replacements::dynamic_deleter>(new rynx::std_replacements::default_delete<T>());
            }

            shared_ptr(const shared_ptr& other) {
                if (this == &other)
                    return;

                m_ptr = static_cast<T*>(other.m_ptr);
                m_control = other.m_control;
                acquire();
            }

            shared_ptr(shared_ptr&& other) {
                if (this == &other)
                    return;
                
                m_ptr = other.m_ptr;
                m_control = other.m_control;
                other.m_ptr = nullptr;
                other.m_control = nullptr;
            }

            shared_ptr& operator=(shared_ptr&& other) {
                if (this == &other)
                    return *this;
                
                release();
                m_ptr = other.m_ptr;
                m_control = other.m_control;
                other.m_ptr = nullptr;
                other.m_control = nullptr;
                return *this;
            }

            shared_ptr& operator=(const shared_ptr& other) {
                if (this == &other)
                    return *this;
                
                release();
                m_ptr = other.m_ptr;
                m_control = other.m_control;
                acquire();
                return *this;
            }

            ~shared_ptr() {
                release();
            }

            int32_t do_not_use_counter_() const noexcept {
                if (m_control)
                    return m_control->m_count;
                return 0;
            }

            template<typename U>
            shared_ptr(const shared_ptr<U>& other) {
                m_ptr = static_cast<T*>(other.m_ptr);
                m_control = other.m_control;
                acquire();
            }

            template<typename U>
            shared_ptr<T>& operator=(const shared_ptr<U>& other) {
                release();
                m_ptr = static_cast<T*>(other.m_ptr);
                m_control = other.m_control;
                acquire();
                return *this;
            }
            
            template<typename U> shared_ptr<T>& operator=(shared_ptr<U>&& other) {
                release();
                m_ptr = static_cast<T*>(other.m_ptr);
                m_control = other.m_control;
                if constexpr (std::is_rvalue_reference_v<decltype(std::forward<shared_ptr<U>>(other))>) {
                    other.m_ptr = nullptr;
                    other.m_control = nullptr;
                }
                else {
                    acquire();
                }
            }
            
            template<typename U> shared_ptr(shared_ptr<U>&& other)
                : m_ptr(static_cast<T*>(other.m_ptr))
                , m_control(other.m_control)
            {
                if constexpr (std::is_rvalue_reference_v<decltype(std::forward<shared_ptr<U>>(other))>) {
                    other.m_ptr = nullptr;
                    other.m_control = nullptr;
                }
                else {
                    acquire();
                }
            }

            bool operator != (nullptr_t) const noexcept {
                return m_ptr != nullptr;
            }

            bool operator == (nullptr_t) const noexcept {
                return m_ptr == nullptr;
            }

            operator bool() const noexcept {
                return m_control && m_control->m_count > 0;
            }

            T& operator*() const noexcept {
                return *m_ptr;
            }

            T* operator->() const noexcept {
                return m_ptr;
            }

            T* get() const noexcept {
                return m_ptr;
            }

            shared_ptr& reset() noexcept {
                release();
                return *this;
            }
        };


        // weak ptr

        template <typename T>
        class weak_ptr {
            T* m_ptr = nullptr;
            rynx::std_replacements::shared_ptr_control_block* m_control = nullptr;

            void release() {
                // this is not exactly thread safe :(
                // TODO: fix this.
                if (m_control && m_control->m_weak.fetch_add(-1) == 1)
                    if (m_control->m_count == 0)
                        delete m_control;
            }

            void acquire() {
                if (m_control)
                    ++m_control->m_weak;
            }

        public:
            weak_ptr() {}
            weak_ptr(nullptr_t) {}

            weak_ptr(const weak_ptr& other) {
                m_ptr = other.m_ptr;
                m_control = other.m_control;
                acquire();
            }

            weak_ptr(weak_ptr&& other) {
                m_ptr = other.m_ptr;
                m_control = other.m_control;
                other.m_ptr = nullptr;
                other.m_control = nullptr;
            }

            weak_ptr& operator=(const weak_ptr& other) {
                if (this == &other)
                    return *this;
                
                release();
                m_ptr = other.m_ptr;
                m_control = other.m_control;
                acquire();
                return *this;
            }

            weak_ptr& operator=(weak_ptr&& other) {
                if (this == &other)
                    return *this;

                release();
                m_ptr = other.m_ptr;
                m_control = other.m_control;
                other.m_ptr = nullptr;
                other.m_control = nullptr;
                return *this;
            }

            template<typename U>
            weak_ptr(const rynx::std_replacements::shared_ptr<U>& ptr) {
                m_ptr = static_cast<T*>(ptr.get());
                m_control = ptr.m_control;
                acquire();
            }

            template<typename U>
            weak_ptr(const weak_ptr<U>& other) {
                m_ptr = static_cast<T*>(other.m_ptr);
                m_control = other.m_control;
                acquire();
            }

            template<typename U>
            weak_ptr(weak_ptr<U>&& other) {
                m_ptr = static_cast<T*>(other.m_ptr);
                m_control = other.m_control;
                if constexpr (std::is_rvalue_reference_v<decltype(std::forward<weak_ptr<U>>(other))>) {
                    other.m_ptr = nullptr;
                    other.m_control = nullptr;
                }
                else {
                    acquire();
                }
            }

            template<typename U>
            weak_ptr<T>& operator=(const rynx::std_replacements::shared_ptr<U>& other) {
                release();
                m_ptr = static_cast<T*>(other.m_ptr);
                m_control = other.m_control;
                acquire();
                return *this;
            }

            template<typename U>
            weak_ptr<T>& operator=(const rynx::std_replacements::weak_ptr<U>& other) {
                release();
                m_ptr = static_cast<T*>(other.m_ptr);
                m_control = other.m_control;
                acquire();
                return *this;
            }

            template<typename U>
            weak_ptr<T>& operator=(rynx::std_replacements::weak_ptr<U>&& other) {
                release();
                m_ptr = static_cast<T*>(other.m_ptr);
                m_control = other.m_control;
                
                if constexpr (std::is_rvalue_reference_v<decltype(std::forward<weak_ptr<U>>(other))>) {
                    other.m_ptr = nullptr;
                    other.m_control = nullptr;
                }
                else {
                    acquire();
                }

                return *this;
            }

            bool expired() const noexcept {
                return !m_control || (m_control->m_count == 0);
            }

            rynx::std_replacements::shared_ptr<T> lock() const {
                if (m_control) {
                    int32_t v = m_control->m_count.load(std::memory_order_relaxed);
                    while (v > 0) {
                        if (m_control->m_count.compare_exchange_weak(v, v + 1, std::memory_order_release, std::memory_order_relaxed))
                            return rynx::std_replacements::shared_ptr<T>{m_ptr, m_control};
                    }
                }
                return {};
            }

            bool operator != (nullptr_t) const noexcept {
                return m_ptr != nullptr;
            }

            bool operator == (nullptr_t) const noexcept {
                return m_ptr == nullptr;
            }

            operator bool() const noexcept {
                return m_ptr != nullptr;
            }

            weak_ptr& reset() noexcept {
                release();
                return *this;
            }
        };
    }
        
#ifdef USE_RYNX_SMART_PTRS
    template<typename T, typename Deleter = rynx::std_replacements::default_delete<T>> using unique_ptr = rynx::std_replacements::unique_ptr<T, Deleter>;
    template<typename T> using shared_ptr = rynx::std_replacements::shared_ptr<T>;
    template<typename T> using weak_ptr = rynx::std_replacements::weak_ptr<T>;
#else
    template<typename T, typename Deleter = rynx::std_replacements::default_delete<T>> using unique_ptr = std::unique_ptr<T, Deleter>;
    template<typename T> using shared_ptr = std::shared_ptr<T>;
    template<typename T> using weak_ptr = std::weak_ptr<T>;
#endif

    template <class element_t, class... args_t, std::enable_if_t<!std::is_array_v<element_t>, int> = 0>
    [[nodiscard]] unique_ptr<element_t> make_unique(args_t&&... args) { // make a unique_ptr
        return unique_ptr<element_t>(new element_t(std::forward<args_t>(args)...));
    }

    template <class element_t, std::enable_if_t<std::is_array_v<element_t>&& std::extent_v<element_t> == 0, int> = 0>
    [[nodiscard]] unique_ptr<element_t> make_unique(const size_t size) { // make a unique_ptr
        using internal_element_t = std::remove_extent_t<element_t>;
        return unique_ptr<element_t>(new internal_element_t[size]());
    }

	template<typename T> using opaque_unique_ptr = rynx::unique_ptr<T, rynx::function<void(T*)>>;
	template<typename T, typename...Args> opaque_unique_ptr<T> make_opaque_unique_ptr(Args&&... args) {
		return rynx::opaque_unique_ptr<T>(new T(std::forward<Args>(args)...), [](T* t) { delete t; });
	}
    
    template<typename T, typename... Args> rynx::shared_ptr<T> make_shared(Args&&... args) { return rynx::shared_ptr<T>(new T(std::forward<Args>(args)...)); }


	template<typename T>
	class observer_ptr {
		template<typename T> friend class observer_ptr;

	public:
		observer_ptr() = default;
		observer_ptr(nullptr_t) : value(nullptr) {}
		template <typename U> observer_ptr(U* u) : value(static_cast<T*>(u)) {}
		template <typename U> observer_ptr(const observer_ptr<U>& other) : value(static_cast<T*>(other.value)) {}
		template <typename U> observer_ptr(const rynx::shared_ptr<U>& other) : value(static_cast<T*>(other.get())) {}
		template <typename U> observer_ptr(const rynx::unique_ptr<U>& other) : value(static_cast<T*>(other.get())) {}
		template <typename U> observer_ptr(const rynx::weak_ptr<U>& other) : value(static_cast<T*>(other.lock().get())) {}

		template <typename U> observer_ptr& operator = (U* u) { value = static_cast<T*>(u); return *this; }
		template <typename U> observer_ptr& operator = (const observer_ptr<U>& other) { value = static_cast<T*>(other.value); return *this; }
		template <typename U> observer_ptr& operator = (const rynx::shared_ptr<U>& other) { value = static_cast<T*>(other.get()); return *this; }
		template <typename U> observer_ptr& operator = (const rynx::unique_ptr<U>& other) { value = static_cast<T*>(other.get()); return *this; }
		template <typename U> observer_ptr& operator = (const rynx::weak_ptr<U>& other) { value = static_cast<T*>(other.lock().get()); return *this; }

		bool operator == (nullptr_t) { return value == nullptr; }
		template <typename U> bool operator == (U* u) { return value == static_cast<T*>(u); }
		template <typename U> bool operator == (const observer_ptr<U>& other) { return value == static_cast<T*>(other.value); }
		template <typename U> bool operator == (const rynx::shared_ptr<U>& other) { return value == static_cast<T*>(other.get()); }
		template <typename U> bool operator == (const rynx::unique_ptr<U>& other) { return value == static_cast<T*>(other.get()); }
		template <typename U> bool operator == (const rynx::weak_ptr<U>& other) { return value == static_cast<T*>(other.lock().get()); }

		bool operator != (nullptr_t) { return value != nullptr; }
		template <typename U> bool operator != (U* u) { return value != static_cast<T*>(u); }
		template <typename U> bool operator != (const observer_ptr<U>& other) { return value != static_cast<T*>(other.value); }
		template <typename U> bool operator != (const rynx::shared_ptr<U>& other) { return value != static_cast<T*>(other.get()); }
		template <typename U> bool operator != (const rynx::unique_ptr<U>& other) { return value != static_cast<T*>(other.get()); }
		template <typename U> bool operator != (const rynx::weak_ptr<U>& other) { return value != static_cast<T*>(other.lock().get()); }

		template<typename U> observer_ptr<U> static_pointer_cast() { return observer_ptr<U>(static_cast<U*>(value)); }
		template<typename U> observer_ptr<U> const_pointer_cast() { return observer_ptr<U>(const_cast<U*>(value)); }
		template<typename U> observer_ptr<U> dynamic_pointer_cast() { return observer_ptr<U>(dynamic_cast<U*>(value)); }

		T* operator->() { return value; }
		const T* operator->() const { return value; }

		template<typename U = T>
		U& operator*() { return *value; }
		
		template<typename U = T>
		const U& operator*() const { return *value; }
		
		operator bool() const {
			return value != nullptr;
		}

		T* get() { return value; }
		const T* get() const { return value; }

	private:
		T* value = nullptr;
	};

	template<typename T> rynx::observer_ptr<T> as_observer(T& t) { return rynx::observer_ptr<T>(&t); }
	template<typename T> rynx::observer_ptr<T> as_observer(T* t) { return rynx::observer_ptr<T>(t); }
	template<typename T> rynx::observer_ptr<T> as_observer(rynx::shared_ptr<T>& t) { return rynx::observer_ptr<T>(t); }
	template<typename T> rynx::observer_ptr<T> as_observer(rynx::unique_ptr<T>& t) { return rynx::observer_ptr<T>(t); }
	template<typename T> rynx::observer_ptr<T> as_observer(rynx::weak_ptr<T>& t) { return rynx::observer_ptr<T>(t); }
	template<typename T> rynx::observer_ptr<T> as_observer(rynx::observer_ptr<T>& t) { return t; }
}
