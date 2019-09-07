
#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include <catch.hpp>

#include <rynx/tech/ecs.hpp>

namespace rynx {
	namespace components {
		struct position {};
	}
}

TEST_CASE("ecs views usable", "")
{
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
