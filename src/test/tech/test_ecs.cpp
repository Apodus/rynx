#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include <catch.hpp>

#include <rynx/tech/ecs.hpp>

namespace rynx {
	namespace components {
		struct position {};
	}
}

TEST_CASE("rynx ecs", "sort")
{
	rynx::ecs r;
	r.type_index_using<int>();

	r.create(3);
	r.create(7);
	r.create(-100);
	r.create(+1000);
	r.create(-101);
	r.create(-120);
	r.create(-110);
	r.create(-111);
	r.create(-113);
	r.create(-112);

	r.query().sort_by<int>();

	int prev = -1000000;
	r.query().for_each([&prev](int v) {
		REQUIRE(prev < v);
		prev = v;
	});
}

TEST_CASE("ecs views usable", "verify compiles")
{
	// these must compile. that is all.
	{
		rynx::ecs ecs;
		rynx::ecs::view<rynx::components::position> view = ecs;
		view.query().for_each([](const rynx::components::position&) {}); // is ok.
		view.query().for_each([](rynx::components::position&) {}); // is ok.
	}

	{
		rynx::ecs ecs;
		rynx::ecs::view<const rynx::components::position> view = ecs;
		view.query().for_each([](const rynx::components::position&) {}); // is ok.
		// view.for_each([](rynx::components::position&) {}); // error C2338:  You promised to access this type in only const mode!
		// view.for_each([](rynx::components::radius&) {}); // error C2338:  You have promised to not access this type in your ecs view.
	}
}

TEST_CASE("ecs for_each iterates correct amount of entities")
{
	{
		struct a {
			int value;
		};

		struct b {
			int value;
		};

		rynx::ecs db;
		db.create(a{ 1 });
		db.create(a{ 2 });
		db.create(b{ 3 });
		db.create(b{ 4 });
		db.create(b{ 5 });
		db.create(a{ 6 }, b{ 7 });
		db.create(a{ 8 }, b{ 9 });

		{
			uint32_t count = 0;
			rynx::ecs::view<a> view = db;
			view.query().for_each([&count](a&) {
				++count;
			});

			REQUIRE(count == 4); // 2x {a} + 2x{a+b} = 4x a
		}

		{
			uint32_t count = 0;
			rynx::ecs::view<b> view = db;
			view.query().for_each([&count](b&) {
				++count;
			});

			REQUIRE(count == 5); // 3x {b} + 2x{a+b} = 4x a
		}

		{
			uint32_t count = 0;
			rynx::ecs::view<a, b> view = db;
			view.query().for_each([&count](a&, b&) {
				++count;
			});

			REQUIRE(count == 2); // 2x{a+b}
		}

	}
}

// you can run the same benchmarks against entt if you want. find out how to setup entt from their github pages.
namespace entt {
	class registry;
}

template<typename...Ts> int64_t ecs_for_each(rynx::ecs& ecs) {
	int64_t count = 0;
	ecs.type_index_using<Ts...>();
	ecs.query().for_each([&](Ts ... ts) {
		count += (static_cast<int64_t>(ts) + ...);
	});
	return count;
}

template<typename...Ts> int64_t ecs_for_each(entt::registry& ecs) {
	int64_t count = 0;
	ecs.view<Ts...>().each([&](Ts ... ts) {
		count += (static_cast<int64_t>(ts) + ...);
	});
	return count;
}

TEST_CASE("rynx ecs: 50% random components") {
	constexpr int numEntities = 10000;
	constexpr int fillrate = 50;
	
	/*
	{
		rynx::ecs r;
		r.type_index_using<int, float, double, uint32_t>();
		
		srand(1234);
		for (int i = 0; i < numEntities; ++i) {
			auto entityId = r.create();
			auto entity = r[entityId];
			if (rand() % 100 < fillrate) {
				entity.add<float>(1.0f);
			}
			if (rand() % 100 < fillrate) {
				entity.add<int>(1);
			}
			if (rand() % 100 < fillrate) {
				entity.add<double>(1.0f);
			}
			if (rand() % 100 < fillrate) {
				entity.add<uint32_t>(1);
			}
		}

		BENCHMARK("query int") {
			return ecs_for_each<int>(r);
		};

		BENCHMARK("query uint") {
			return ecs_for_each<int>(r);
		};

		BENCHMARK("query float") {
			return ecs_for_each<float>(r);
		};

		BENCHMARK("query int float") {
			return ecs_for_each<int, float>(r);
		};

		BENCHMARK("query float int") {
			return ecs_for_each<float, int>(r);
		};

		BENCHMARK("query float int uint32") {
			return ecs_for_each<float, int, uint32_t>(r);
		};

		BENCHMARK("query float int uint32 double") {
			return ecs_for_each<float, int, uint32_t, double>(r);
		};
	}
	*/
}

TEST_CASE("rynx ecs: insert & delete")
{
	/*
	struct two_ints
	{
		int a, b;
	};

	BENCHMARK("insert & delete")
	{
		rynx::ecs r;
		r.type_index_using<two_ints>();

		for (int i = 0; i < 1000; ++i) {
			r.create(two_ints{ i, i + 1 });
		}

		r.erase(r.query().in<two_ints>().ids());
		return r.size();
	};
	*/
}
