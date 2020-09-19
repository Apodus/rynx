#pragma once

namespace rynx {
	template<typename T>
	class smooth {
	public:
		smooth() = default;
		smooth(T t) : current(t), target(t) {}
		template<typename...Ts> smooth(Ts&&... ts) : smooth(T(std::forward<Ts>(ts)...)) {}

		template<typename U> smooth& operator = (const U& other) { target = other; return *this; }
		template<typename U> smooth& operator +=(const U& other) { target += other; return *this; }
		template<typename U> smooth& operator -=(const U& other) { target -= other; return *this; }
		template<typename U> smooth& operator *=(const U& other) { target *= other; return *this; }
		template<typename U> smooth& operator /=(const U& other) { target /= other; return *this; }

		template<typename U> smooth& operator = (U&& other) { target = other; return *this; }
		template<typename U> smooth& operator +=(U&& other) { target += other; return *this; }
		template<typename U> smooth& operator -=(U&& other) { target -= other; return *this; }
		template<typename U> smooth& operator *=(U&& other) { target *= other; return *this; }
		template<typename U> smooth& operator /=(U&& other) { target /= other; return *this; }

		template<typename U> T operator +(const U& other) { return current + other; }
		template<typename U> T operator -(const U& other) { return current - other; }
		template<typename U> T operator *(const U& other) { return current * other; }
		template<typename U> T operator /(const U& other) { return current / other; }

		smooth& setCurrent(T t) { current = t; return *this; }
		smooth& tick(float dt) { current += (target - current) * dt; return *this; }

		operator T() const { return current; }
		
		T* operator -> () { return &current; }
		const T* operator -> () const { return &current; }

		T& operator * () { return current; }
		const T& operator * () const { return current; }

		T& value() { return current; }
		T value() const { return current; }

		T& target_value() { return target; }
		T target_value() const { return target; }

	private:
		T current;
		T target;
	};
}