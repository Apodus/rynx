
#include <catch.hpp>
#include <rynx/std/memory.hpp>

TEST_CASE("shared_ptr", "copying increments use count")
{
	// assign & move to others
	{
		rynx::shared_ptr<int> a = rynx::make_shared<int>(5);
		REQUIRE(a.do_not_use_counter_() == 1);
		rynx::shared_ptr<int> b = a;
		REQUIRE(a.do_not_use_counter_() == b.do_not_use_counter_());
		REQUIRE(a.do_not_use_counter_() == 2);

		rynx::shared_ptr<int> c(std::move(b));
		REQUIRE(a.do_not_use_counter_() == c.do_not_use_counter_());
		REQUIRE(a.do_not_use_counter_() == 2);
		REQUIRE(b.do_not_use_counter_() == 0);
	}

	// assign & move to self
	{
		rynx::shared_ptr<int> a = rynx::make_shared<int>(5);
		a = a;
		REQUIRE(a.do_not_use_counter_() == 1);
		a = std::move(a);
		REQUIRE(a.do_not_use_counter_() == 1);
		rynx::shared_ptr<int> b;
		b = a;
		REQUIRE(a.do_not_use_counter_() == 2);
		b = std::move(a);
		REQUIRE(a.do_not_use_counter_() == 0);
		REQUIRE(b.do_not_use_counter_() == 1);
		REQUIRE(*b == 5);
	}

	// store in vector and fast pop
	{
		std::vector<rynx::shared_ptr<int>> ptrs;
		for (int i = 0; i < 100; ++i) {
			ptrs.emplace_back(rynx::make_shared<int>(i));
		}

		// pop back
		{
			rynx::shared_ptr<int> i = ptrs.back();
			REQUIRE(i.do_not_use_counter_() == 2);
			ptrs.back() = std::move(ptrs.back());
			REQUIRE(i.do_not_use_counter_() == 2);
			ptrs.pop_back();
			REQUIRE(i.do_not_use_counter_() == 1);
		}

		// pop front
		{
			rynx::shared_ptr<int> i = ptrs.front();
			REQUIRE(i.do_not_use_counter_() == 2);
			ptrs.front() = std::move(ptrs.back());
			REQUIRE(ptrs.front().do_not_use_counter_() == 1);
			REQUIRE(ptrs.back().do_not_use_counter_() == 0);
			ptrs.pop_back();
			REQUIRE(ptrs.front().do_not_use_counter_() == 1);
			REQUIRE(i.do_not_use_counter_() == 1);
		}
	}

	// cast down copy & move.
	{
		struct A {
			int a = 0;
		};

		struct B : public A {
			int b = 0;
		};

		rynx::shared_ptr<B> derived = rynx::make_shared<B>();
		derived->a = 5;
		derived->b = 10;

		REQUIRE(derived.do_not_use_counter_() == 1);

		{
			rynx::shared_ptr<A> base = derived;
			REQUIRE(derived.do_not_use_counter_() == 2);
			REQUIRE(base.do_not_use_counter_() == 2);
			
			REQUIRE(derived->a == 5);
			REQUIRE(derived->b == 10);
			REQUIRE(base->a == 5);

			{
				rynx::shared_ptr<B> derived_again = base;
				REQUIRE(derived.do_not_use_counter_() == 3);
				REQUIRE(base.do_not_use_counter_() == 3);
				REQUIRE(derived_again.do_not_use_counter_() == 3);
			}

			REQUIRE(derived.do_not_use_counter_() == 2);
			REQUIRE(base.do_not_use_counter_() == 2);

			rynx::shared_ptr<B> derived_again = std::move(base);
			REQUIRE(derived.do_not_use_counter_() == 2);
			REQUIRE(base.do_not_use_counter_() == 0);
			REQUIRE(derived_again.do_not_use_counter_() == 2);

			REQUIRE(derived->a == 5);
			REQUIRE(derived->b == 10);
			REQUIRE(derived_again->a == 5);
			REQUIRE(derived_again->b == 10);
		}

		REQUIRE(derived.do_not_use_counter_() == 1);
		REQUIRE(derived->a == 5);
		REQUIRE(derived->b == 10);
	}

	// cast to bool
	{
		rynx::shared_ptr<int> a = rynx::make_shared<int>(5);
		rynx::shared_ptr<int> b;
		
		int count = 0;
		if (a)
			count = 1;
		
		REQUIRE(count == 1);
		if (b)
			count = 2;
	}
}

TEST_CASE("weak_ptr", "lock success/fail")
{
	rynx::weak_ptr<int> t;
	{
		rynx::shared_ptr<int> a = rynx::make_shared<int>(5);
		t = a;

		REQUIRE(!t.expired());
		REQUIRE(*t.lock() == 5);
		
		// store in vector and fast pop
		{
			std::vector<rynx::shared_ptr<int>> ptrs;
			for (int i = 0; i < 100; ++i) {
				ptrs.emplace_back(rynx::make_shared<int>(i));
			}

			// pop back
			{
				rynx::shared_ptr<int> i = ptrs.back();
				REQUIRE(i.do_not_use_counter_() == 2);
				ptrs.back() = std::move(ptrs.back());
				REQUIRE(i.do_not_use_counter_() == 2);
				ptrs.pop_back();
				REQUIRE(i.do_not_use_counter_() == 1);
			}

			// pop front
			{
				rynx::shared_ptr<int> i = ptrs.front();
				REQUIRE(i.do_not_use_counter_() == 2);
				ptrs.front() = std::move(ptrs.back());
				REQUIRE(ptrs.front().do_not_use_counter_() == 1);
				REQUIRE(ptrs.back().do_not_use_counter_() == 0);
				ptrs.pop_back();
				REQUIRE(ptrs.front().do_not_use_counter_() == 1);
				REQUIRE(i.do_not_use_counter_() == 1);
			}
		}

		
		{
			rynx::shared_ptr<int> b = a;
			REQUIRE(!t.expired());
			REQUIRE(*t.lock() == 5);

			rynx::weak_ptr<int> other = t;
			REQUIRE(!other.expired());
			REQUIRE(*other.lock() == 5);

			rynx::weak_ptr<int> other2 = std::move(other);
			REQUIRE(other.expired());
			REQUIRE(other.lock() == nullptr);

			REQUIRE(!other2.expired());
			REQUIRE(*other2.lock() == 5);
		}
		REQUIRE(!t.expired());
		REQUIRE(*t.lock() == 5);
	}

	REQUIRE(t.expired());
	REQUIRE(t.lock() == nullptr);
}