
#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include <catch.hpp>

#include <rynx/tech/unordered_map.hpp>

namespace rynx {
	namespace components {
		struct position {};
	}
}

TEST_CASE("unordered_map", "verify insert/remove")
{
	{
		// verify inserted elements are found, next element is not.
		rynx::unordered_map<int, int> map;
		for (int i = 0; i < 10000; ++i) {
			map.insert({ i, i });
			REQUIRE(map.find(i) != map.end());
			REQUIRE(map.find(i + 1) == map.end());
		}

		// verify size matches
		auto verifySizeMatches = [&](size_t expectedNumber)
		{
			size_t count = 0;
			for (auto&& entry : map) {
				(void)(entry);
				++count;
			}
			REQUIRE(count == expectedNumber);
			REQUIRE(map.size() == expectedNumber);
		};

		verifySizeMatches(10000);

		// verify erased elements can't be found. next can.
		for (int i = 0; i < 10000; i += 2) {
			REQUIRE(map.find(i) != map.end());
			map.erase(i);
			REQUIRE(map.find(i) == map.end());
			REQUIRE(map.find(i + 1) != map.end());
		}

		// verify size matches
		verifySizeMatches(10000 / 2);
	}
}


TEST_CASE("unordered_map benchmark", "std vs rynx") {

	auto bench_insert = [](auto& map) {
		for (int i = 0; i < 100000; ++i) {
			map.insert({ i, i });
		}
	};

	// insert
	BENCHMARK("rynx: construct & insert") {
		rynx::unordered_map<int, int> rynxmap;
		bench_insert(rynxmap);
		return rynxmap.size();
	};

	BENCHMARK("std: construct & insert") {
		std::unordered_map<int, int> stdmap;
		bench_insert(stdmap);
		return stdmap.size();
	};

	// find
	{
		rynx::unordered_map<int, int> rynxmap;
		for (int i = 0; i < 1000000; ++i) {
			rynxmap.insert({ i, i });
		}
		BENCHMARK("rynx: find") {
			int target = 1000000 - 1;
			int sum = 0;
			while (target > 0) {
				sum += rynxmap.find(target)->second;
				target -= 10;
			}
			return sum;
		};
	}

	{
		std::unordered_map<int, int> stdmap;
		for (int i = 0; i < 1000000; ++i) {
			stdmap.insert({ i, i });
		}
		BENCHMARK("std: find") {
			int target = 1000000 - 1;
			int sum = 0;
			while (target > 0) {
				sum += stdmap.find(target)->second;
				target -= 10;
			}
			return sum;
		};
	}

	// iteration
	{
		rynx::unordered_map<int, int> rynxmap;
		for (int i = 0; i < 1000000; ++i) {
			rynxmap.insert({ i, i });
		}
		BENCHMARK("rynx: iterate") {
			int sum = 0;
			for(auto&& entry : rynxmap)
				sum += entry.second;
			return sum;
		};
	}

	{
		std::unordered_map<int, int> stdmap;
		for (int i = 0; i < 1000000; ++i) {
			stdmap.insert({ i, i });
		}
		BENCHMARK("std: iterate") {
			int sum = 0;
			for (auto&& entry : stdmap)
				sum += entry.second;
			return sum;
		};
	}


}