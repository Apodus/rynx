
#pragma once

#include <rynx/system/assert.hpp>
#include <cstddef>
#include <type_traits>

#include <utility> // for pair : todo, remove
#include <cstring> // for memcpy

namespace rynx {
	namespace std_replacements {
        template <size_t size, bool alloc_allowed, class F>
        class function_inplace;

        namespace detail {
            template <class... Args>
            struct function_base {};
            template <class T>
            struct function_base<T> {
                using argument_type [[deprecated]] = T;
            };
            template <class T, class U>
            struct function_base<T, U> {
                using first_argument_type [[deprecated]] = T;
                using second_argument_type [[deprecated]] = U;
            };

            template <class T> struct is_function_pointer : std::false_type {};
            template <class T> struct is_function_pointer<T*> : std::is_function<T> {};
            template <class T> inline constexpr bool is_function_pointer_v = is_function_pointer<T>::value;

            template <class T> struct is_function : std::false_type {};
            template <size_t size, bool v, class T> struct is_function<function_inplace<size, v, T>> : std::true_type {};
            template <class T> inline constexpr bool is_function_v = is_function<T>::value;

            template <class R, class F, class... Args>
            R invoke_r(F&& f, Args&&... args) noexcept(std::is_nothrow_invocable_r_v<R, F, Args...>) {
                if constexpr (std::is_void_v<R>)
                    std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
                else
                    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
            }

            template<size_t size>
            struct callable_storage {
                std::byte buffer[size];
            };

            template <class R, class... Args>
            struct callable_base {
                virtual ~callable_base() {}
                virtual R invoke(Args...) const = 0;
                virtual callable_base* clone() const = 0;
                virtual std::pair<callable_base<R, Args...>*, bool> clone_into(void* buffer, size_t size) const = 0;
            };

            // callable wrapper
            template <class F, class R, class... Args>
            struct callable : callable_base<R, Args...> {
            public:
                callable(F f) : func{ std::move(f) }
                {
                }

                ~callable() {}

                R invoke(Args... args) const override
                {
                    return rynx::std_replacements::detail::invoke_r<R>(func, std::forward<Args>(args)...);
                }

                callable_base<R, Args...>* clone() const override
                {
                    return { new callable{ func } };
                }

                std::pair<callable_base<R, Args...>*, bool> clone_into(void* buffer, size_t size) const override {
                    if (sizeof(callable) <= size)
                        return { new (buffer) callable{ func }, true };
                    else
                        return { nullptr, false };
                }
            
            private:
                mutable F func;
            };
        }

        // [func.wrap.func], class template function
        template <size_t buffer_size, bool dynamic_alloc_allowed, class R, class... Args>
        class function_inplace<buffer_size, dynamic_alloc_allowed, R(Args...)> : public detail::function_base<Args...> {
        public:
            using result_type = R;
            
            function_inplace() noexcept {}
            function_inplace(std::nullptr_t) noexcept {}
            function_inplace(const function_inplace& f)
            {
                func = nullptr;
                if (f) {
                    auto [ptr, success] = f.func->clone_into(storage.buffer, buffer_size);
                    if (success) {
                        func = ptr;
                    }
                    else {
                        if constexpr (dynamic_alloc_allowed) {
                            func = f.func->clone();
                        }
                        else {
                            rynx_assert(false, "dynamic allocation not allowed and cant fit function obj clone.");
                        }
                    }
                }
            }

            function_inplace(function_inplace&& f) noexcept {
                swap(f);
            }

            template <class F, std::enable_if_t<std::is_invocable_r_v<R, F&, Args...>, int> = 0>
            function_inplace(F f) {
                if constexpr (detail::is_function_pointer_v<F> ||
                    std::is_member_pointer_v<F> ||
                    detail::is_function_v<F>) {
                    if (!f) return;
                }

                static_assert(dynamic_alloc_allowed || sizeof(detail::callable<F, R, Args...>) <= buffer_size, "function object does not fit into inplace function. select a larger buffer size.");
                if constexpr (sizeof(detail::callable<F, R, Args...>) <= buffer_size) {
                    reset(new (storage.buffer) detail::callable<F, R, Args...>{ std::move(f) });
                }
                else {
                    // if we don't fit in inplace buffer, then do dynamic alloc.
                    reset(new detail::callable<F, R, Args...>{ std::move(f) });
                }
            }

            function_inplace& operator=(const function_inplace& f) {
                function_inplace{ f }.swap(*this);
                return *this;
            }

            function_inplace& operator=(function_inplace&& f) {
                swap(f);
                return *this;
            }

            function_inplace& operator=(std::nullptr_t) noexcept {
                reset(nullptr);
                return *this;
            }

            template <class F, std::enable_if_t<std::is_invocable_r_v<R, std::decay_t<F>&, Args...>, int> = 0>
            function_inplace& operator=(F&& f) {
                function_inplace{ std::forward<F>(f) }.swap(*this);
                return *this;
            }

            template <class F>
            function_inplace& operator=(std::reference_wrapper<F> f) noexcept {
                function_inplace{ f }.swap(*this);
                return *this;
            }

            ~function_inplace() {
                reset(nullptr);
            }

            void swap(function_inplace& other) noexcept {
                const bool other_inplace = other.is_inplace();
                const bool self_inplace = is_inplace();

                // store self state.
                detail::callable_storage<buffer_size> copy;
                std::memcpy(copy.buffer, storage.buffer, buffer_size);
                auto* func_copy = func;

                // copy other state over mine.
                if (other_inplace) {
                    std::memcpy(storage.buffer, other.storage.buffer, buffer_size);
                    func = reinterpret_cast<detail::callable_base<R, Args...>*>(storage.buffer + (int64_t(other.func) - int64_t(other.storage.buffer)));
                }
                else {
                    func = other.func;
                }

                // copy stored state over other.
                if (self_inplace) {
                    std::memcpy(other.storage.buffer, copy.buffer, buffer_size);
                    other.func = reinterpret_cast<detail::callable_base<R, Args...>*>(other.storage.buffer + (int64_t(func_copy) - int64_t(storage.buffer)));
                }
                else {
                    other.func = func_copy;
                }
            }

            explicit operator bool() const noexcept {
                return static_cast<bool>(func);
            }

            R operator()(Args... args) const {
                rynx_assert(func != nullptr, "invoking empty function not allowed");
                return func->invoke(std::forward<Args>(args)...);
            }

            bool is_inplace() const {
                return uint64_t(func) >= uint64_t(&storage) && uint64_t(func) < uint64_t(&storage + sizeof(storage));
            }

        private:
            void reset(detail::callable_base<R, Args...>* ptr) noexcept {
                if constexpr (dynamic_alloc_allowed) {
                    if (func && !is_inplace())
                        delete func;
                }
                func = ptr;
            }

            detail::callable_storage<buffer_size> storage;
            detail::callable_base<R, Args...>* func = nullptr;
        };

        namespace detail {
            template <typename T> struct deduce_function {};
            template <typename T> using deduce_function_t = typename deduce_function<T>::type;

            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...)> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) const> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) volatile> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) const volatile> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...)&> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) const&> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) volatile&> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) const volatile&> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) noexcept> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) const noexcept> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) volatile noexcept> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) const volatile noexcept> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) & noexcept> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) const& noexcept> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) volatile& noexcept> { using type = R(Args...); };
            template <typename G, typename R, typename... Args> struct deduce_function<R(G::*)(Args...) const volatile& noexcept> { using type = R(Args...); };
        }

        template <size_t size, bool alloc, class R, class... Args> function_inplace(R(*)(Args...))->function_inplace<size, alloc, R(Args...)>;
        template <size_t size, bool alloc, class F, class Op = decltype(&F::operator())> function_inplace(F)->function_inplace<size, alloc, detail::deduce_function_t<Op>>;
        template <size_t size, bool alloc, class R, class... Args> bool operator==(const function_inplace<size, alloc, R(Args...)>& f, nullptr_t) noexcept { return !f; }
        template <size_t size, bool alloc, class R, class... Args> bool operator==(nullptr_t, const function_inplace<size, alloc, R(Args...)>& f) noexcept { return !f; }
        template <size_t size, bool alloc, class R, class... Args> bool operator!=(const function_inplace<size, alloc, R(Args...)>& f, nullptr_t) noexcept { return static_cast<bool>(f); }
        template <size_t size, bool alloc, class R, class... Args> bool operator!=(nullptr_t, const function_inplace<size, alloc, R(Args...)>& f) noexcept { return static_cast<bool>(f); }
        template <size_t size, bool alloc, class R, class... Args> void swap(function_inplace<size, alloc,  R(Args...)>& lhs, function_inplace<size, alloc, R(Args...)>& rhs) noexcept { lhs.swap(rhs); }
	}

    template<size_t inplace_buffer_size, bool dynamic_alloc_allowed, typename T> using function_inplace = rynx::std_replacements::function_inplace<inplace_buffer_size, dynamic_alloc_allowed, T>;
    template<typename T> using function = rynx::std_replacements::function_inplace<1, true, T>;
}