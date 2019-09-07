
#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include <catch.hpp>

#include <rynx/tech/ecs.hpp>
#include <rynx/scheduler/task_scheduler.hpp>

TEST_CASE("scheduler", "[tasks + barriers]")
{
	rynx::scheduler::task_scheduler scheduler;
	auto* context = scheduler.make_context();

	std::atomic<int> count_a = 0;
	auto task1 = context->make_task("First", [&]() { REQUIRE(count_a.fetch_add(1) == 0); });
	auto task2 = context->make_task("Second", [&]() { REQUIRE(count_a.fetch_add(1) == 1); });
	task1.requiredFor(task2);
	context->add_task(std::move(task2));
	context->add_task(std::move(task1));

	std::atomic<int> count_b = 0;
	context->add_task("First", [&]() { REQUIRE(count_b.fetch_add(1) == 0); })->then("Second", [&]() { REQUIRE(count_b.fetch_add(1) == 1); });
		
	scheduler.start_frame();
	scheduler.wait_until_complete();
}


TEST_CASE("scheduler_", "[ecs component resource accesses respected]")
{
	/*
	rynx::ecs ecs;
	rynx::scheduler::task_scheduler scheduler;
	auto* context = scheduler.make_context();
	context->set_resource(&ecs);

	context->add_task("TestRead", [](rynx::ecs::view<const rynx::components::position> kek) {
		kek.for_each([](const rynx::components::position& pos) {
			std::cout << pos.angle << std::endl;
		});
	});

	context->add_task("TestRead", [](rynx::ecs::view<const rynx::components::position> kek) {
		kek.for_each([](const rynx::components::position& pos) {
			std::cout << pos.angle << std::endl;
		});
	});

	context->add_task("TestWrite", [](rynx::ecs::view<rynx::components::position> kek) {
		kek.for_each([](rynx::components::position& pos) {
			pos.value *= 2;
			std::cout << pos.value.x << std::endl;
		});
	});
	*/
}