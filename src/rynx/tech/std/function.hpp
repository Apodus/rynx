
#pragma once

#include <cstddef>
#include <type_traits>

namespace rynx {
	namespace std_replacements {
        template <class F>
        class function;

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

            template <class T> struct is_function :std::false_type {};
            template <class T> struct is_function<function<T>> :std::true_type {};
            template <class T> inline constexpr bool is_function_v = is_function<T>::value;

            template <class R, class F, class... Args>
            R invoke_r(F&& f, Args&&... args) noexcept(std::is_nothrow_invocable_r_v<R, F, Args...>) {
                if constexpr (std::is_void_v<R>)
                    std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
                else
                    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
            }

            template <class R, class... Args>
            struct callable_base {
                virtual ~callable_base() {}
                virtual R invoke(Args...) const = 0;
                virtual callable_base* clone() const = 0;
            };

            // callable wrapper
            template <class F, class R, class... Args>
            struct callable :callable_base<R, Args...> {
                using Base = callable_base<R, Args...>;
            public:
                callable(F f)
                    :func{ std::move(f) }
                {
                }

                ~callable() {}

                R invoke(Args... args) const override
                {
                    return rynx::std_replacements::detail::invoke_r<R>(func, std::forward<Args>(args)...);
                }

                Base* clone() const override
                {
                    return { new callable{ func } };
                }

            private:
                mutable F func;
            };
        }

        // [func.wrap.func], class template function
        template <class R, class... Args>
        class function<R(Args...)> :public detail::function_base<Args...> {
        public:
            using result_type = R;

            // [func.wrap.func.con], construct/copy/destroy
            function() noexcept
            {
            }

            function(std::nullptr_t) noexcept
            {
            }

            function(const function& f)
                :func{ f ? f.func->clone() : nullptr }
            {
            }

            function(function&& f) noexcept // strengthened
            {
                swap(f);
            }

            template <class F,
                std::enable_if_t<std::is_invocable_r_v<R, F&, Args...>, int> = 0>
            function(F f)
            {
                if constexpr (detail::is_function_pointer_v<F> ||
                    std::is_member_pointer_v<F> ||
                    detail::is_function_v<F>) {
                    if (!f) return;
                }

                reset(new detail::callable<F, R, Args...>{ std::move(f) });
            }

            function& operator=(const function& f)
            {
                function{ f }.swap(*this);
                return *this;
            }

            function& operator=(function&& f)
            {
                swap(f);
                return *this;
            }

            function& operator=(std::nullptr_t) noexcept
            {
                reset(nullptr);
                return *this;
            }

            template <class F,
                std::enable_if_t<
                std::is_invocable_r_v<R, std::decay_t<F>&, Args...>,
                int> = 0>
            function& operator=(F&& f)
            {
                function{ std::forward<F>(f) }.swap(*this);
                return *this;
            }

            template <class F>
            function& operator=(std::reference_wrapper<F> f) noexcept
            {
                function{ f }.swap(*this);
                return *this;
            }

            ~function() {
                reset(nullptr);
            }

            void swap(function& other) noexcept
            {
                using std::swap;
                swap(func, other.func);
            }

            explicit operator bool() const noexcept
            {
                return static_cast<bool>(func);
            }

            R operator()(Args... args) const
            {
                rynx_assert(func != nullptr, "invoking empty function not allowed");
                return func->invoke(std::forward<Args>(args)...);
            }

        private:
            void reset(detail::callable_base<R, Args...>* ptr) noexcept {
                if (func)
                    delete func;
                func = ptr;
            }

            detail::callable_base<R, Args...>* func = nullptr;
        };

        namespace detail {
            template <typename T>
            struct deduce_function {};
            template <typename T>
            using deduce_function_t = typename deduce_function<T>::type;

            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...)> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) const> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) volatile> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) const volatile> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...)&> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) const&> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) volatile&> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) const volatile&> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) noexcept> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) const noexcept> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) volatile noexcept> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) const volatile noexcept> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) & noexcept> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) const& noexcept> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) volatile& noexcept> {
                using type = R(Args...);
            };
            template <typename G, typename R, typename... Args>
            struct deduce_function<R(G::*)(Args...) const volatile& noexcept> {
                using type = R(Args...);
            };
        }

        template <class R, class... Args>
        function(R(*)(Args...))->function<R(Args...)>;

        template <class F, class Op = decltype(&F::operator())>
        function(F)->function<detail::deduce_function_t<Op>>;

        // [func.wrap.func.nullptr], null pointer comparisons
        template <class R, class... Args>
        bool operator==(const function<R(Args...)>& f, nullptr_t) noexcept
        {
            return !f;
        }
        template <class R, class... Args>
        bool operator==(nullptr_t, const function<R(Args...)>& f) noexcept
        {
            return !f;
        }
        template <class R, class... Args>
        bool operator!=(const function<R(Args...)>& f, nullptr_t) noexcept
        {
            return static_cast<bool>(f);
        }
        template <class R, class... Args>
        bool operator!=(nullptr_t, const function<R(Args...)>& f) noexcept
        {
            return static_cast<bool>(f);
        }

        // [func.wrap.func.alg], specialized algorithms
        template <class R, class... Args>
        void swap(function<R(Args...)>& lhs, function<R(Args...)>& rhs) noexcept
        {
            lhs.swap(rhs);
        }
	}

    template<typename T> using function = rynx::std_replacements::function<T>;
}