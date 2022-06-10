#pragma once

#include <memory>
#include <functional>

namespace rynx {
	template<typename T> using opaque_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;
	template<typename T, typename...Args> opaque_unique_ptr<T> make_opaque_unique_ptr(Args&&... args) {
		return rynx::opaque_unique_ptr<T>(new T(args...), [](T* t) { delete t; });
	}

	template<typename T>
	class observer_ptr {
		template<typename T> friend class observer_ptr;

	public:
		observer_ptr() = default;
		observer_ptr(nullptr_t) : value(nullptr) {}
		template <typename U> observer_ptr(U* u) : value(static_cast<T*>(u)) {}
		template <typename U> observer_ptr(const observer_ptr<U>& other) : value(static_cast<T*>(other.value)) {}
		template <typename U> observer_ptr(const std::shared_ptr<U>& other) : value(static_cast<T*>(other.get())) {}
		template <typename U> observer_ptr(const std::unique_ptr<U>& other) : value(static_cast<T*>(other.get())) {}
		template <typename U> observer_ptr(const std::weak_ptr<U>& other) : value(static_cast<T*>(other.lock().get())) {}

		template <typename U> observer_ptr& operator = (U* u) { value = static_cast<T*>(u); return *this; }
		template <typename U> observer_ptr& operator = (const observer_ptr<U>& other) { value = static_cast<T*>(other.value); return *this; }
		template <typename U> observer_ptr& operator = (const std::shared_ptr<U>& other) { value = static_cast<T*>(other.get()); return *this; }
		template <typename U> observer_ptr& operator = (const std::unique_ptr<U>& other) { value = static_cast<T*>(other.get()); return *this; }
		template <typename U> observer_ptr& operator = (const std::weak_ptr<U>& other) { value = static_cast<T*>(other.lock().get()); return *this; }

		bool operator == (nullptr_t) { return value == nullptr; }
		template <typename U> bool operator == (U* u) { return value == static_cast<T*>(u); }
		template <typename U> bool operator == (const observer_ptr<U>& other) { return value == static_cast<T*>(other.value); }
		template <typename U> bool operator == (const std::shared_ptr<U>& other) { return value == static_cast<T*>(other.get()); }
		template <typename U> bool operator == (const std::unique_ptr<U>& other) { return value == static_cast<T*>(other.get()); }
		template <typename U> bool operator == (const std::weak_ptr<U>& other) { return value == static_cast<T*>(other.lock().get()); }

		bool operator != (nullptr_t) { return value != nullptr; }
		template <typename U> bool operator != (U* u) { return value != static_cast<T*>(u); }
		template <typename U> bool operator != (const observer_ptr<U>& other) { return value != static_cast<T*>(other.value); }
		template <typename U> bool operator != (const std::shared_ptr<U>& other) { return value != static_cast<T*>(other.get()); }
		template <typename U> bool operator != (const std::unique_ptr<U>& other) { return value != static_cast<T*>(other.get()); }
		template <typename U> bool operator != (const std::weak_ptr<U>& other) { return value != static_cast<T*>(other.lock().get()); }

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
	template<typename T> rynx::observer_ptr<T> as_observer(std::shared_ptr<T>& t) { return rynx::observer_ptr<T>(t); }
	template<typename T> rynx::observer_ptr<T> as_observer(std::unique_ptr<T>& t) { return rynx::observer_ptr<T>(t); }
	template<typename T> rynx::observer_ptr<T> as_observer(std::weak_ptr<T>& t) { return rynx::observer_ptr<T>(t); }
	template<typename T> rynx::observer_ptr<T> as_observer(rynx::observer_ptr<T>& t) { return t; }
}
