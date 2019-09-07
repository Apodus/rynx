#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include <catch.hpp>

#include <rynx/tech/ecs.hpp>

namespace rynx {
	namespace components {
		struct position {};
	}
}

TEST_CASE("ecs views usable", "verify compiles")
{
	// these must compile. that is all.
	{
		rynx::ecs ecs;
		rynx::ecs::view<rynx::components::position> view = ecs;
		view.for_each([](const rynx::components::position&) {}); // is ok.	
		view.for_each([](rynx::components::position&) {}); // is ok.
	}

	{
		rynx::ecs ecs;
		rynx::ecs::view<const rynx::components::position> view = ecs;
		view.for_each([](const rynx::components::position&) {}); // is ok.	
		// view.for_each([](rynx::components::position&) {}); // error C2338:  You promised to access this type in only const mode!
		// view.for_each([](rynx::components::radius&) {}); // error C2338:  You have promised to not access this type in your ecs view.
	}
}

// you can run the same benchmarks against entt if you want. find out how to setup entt from their github pages.
namespace entt {
	class registry;
}

template<typename...Ts> int64_t ecs_for_each(rynx::ecs& ecs) {
	int64_t count = 0;
	ecs.for_each([&](Ts& ... ts) {
		count += (static_cast<int64_t>(ts) + ...);
		});
	return count;
}

template<typename...Ts> int64_t ecs_for_each(entt::registry& ecs) {
	int64_t count = 0;
	ecs.view<Ts...>().each([&](Ts& ... ts) {
		count += (static_cast<int64_t>(ts) + ...);
		});
	return count;
}

TEST_CASE("rynx :: 50% random components") {
	constexpr int numEntities = 100000;
	constexpr int fillrate = 50;

	{
		rynx::ecs r;
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
}