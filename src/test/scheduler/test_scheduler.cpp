
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
	task1.required_for(task2);
	context->add_task(std::move(task2));
	context->add_task(std::move(task1));

	std::atomic<int> count_b = 0;
	context->add_task("First", [&]() { REQUIRE(count_b.fetch_add(1) == 0); })->then("Second", [&]() { REQUIRE(count_b.fetch_add(1) == 1); });
		
	scheduler.start_frame();
	scheduler.wait_until_complete();
}


TEST_CASE("scheduler", "[ecs component resource accesses respected]")
{
	struct component {
		int a;
	};

	rynx::ecs ecs;
	rynx::scheduler::task_scheduler scheduler;
	auto* context = scheduler.make_context();
	context->set_resource(&ecs);

	std::atomic<int> tasks_started = 0;
	std::atomic<int> result = 0;

	// read access to component. can be scheduled at same time.
	context->add_task("TestRead", [&](rynx::ecs::view<const component> kek) {
		++tasks_started;
		REQUIRE(tasks_started < 3); // read tasks are added first so they will be attempted to run first.

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		int v = 0;
		kek.query().for_each([&](const component& a) {
			v += a.a;
		});

		result += v;
	});

	// read access to component. can be scheduled at same time.
	context->add_task("TestRead", [&](rynx::ecs::view<const component> kek) {
		++tasks_started;
		REQUIRE(tasks_started < 3); // read tasks are added first so they will be attempted to run first.

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		int v = 0;
		kek.query().for_each([&](const component& a) {
			v += a.a;
		});

		result += v;
	});

	// write access to component. can't be scheduled while reads are running.
	context->add_task("TestWrite", [&](rynx::ecs::view<component> kek) {
		++tasks_started;
		REQUIRE(tasks_started == 3); // write task added last, will also run last in this case. because there's no reason to not run the tasks created before.

		int v = 0;
		kek.query().for_each([&](component& a) {
			a.a *= 2; // modify value of components.
			v += a.a;
		});

		result += v;
	});

	// couple of components
	ecs.create(component({ 1 }));
	ecs.create(component({ 1 }));

	scheduler.start_frame();
	scheduler.wait_until_complete();

	REQUIRE(tasks_started == 3);
	REQUIRE(result ==  2 + 2 + 2*2);
}
