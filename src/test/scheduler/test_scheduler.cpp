
#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include <catch.hpp>

#include <rynx/tech/ecs.hpp>
#include <rynx/scheduler/task_scheduler.hpp>
#include <thread>

TEST_CASE("tasks and barriers", "scheduler")
{
	rynx::this_thread::rynx_thread_raii obj;
	rynx::scheduler::task_scheduler scheduler;
	auto* context = scheduler.make_context();

	std::atomic<int> count_a = 0;
	auto task1 = context->add_task("First", [&]() { REQUIRE(count_a.fetch_add(1) == 0); });
	auto task2 = context->add_task("Second", [&]() { REQUIRE(count_a.fetch_add(1) == 1); });
	task1.required_for(task2);
	
	std::atomic<int> count_b = 0;
	context->add_task("First", [&]() { REQUIRE(count_b.fetch_add(1) == 0); }).then("Second", [&]() { REQUIRE(count_b.fetch_add(1) == 1); });
		
	scheduler.start_frame();
	scheduler.wait_until_complete();
}


TEST_CASE("ecs component resource accesses respected", "scheduler")
{
	rynx::this_thread::rynx_thread_raii obj;
	
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


TEST_CASE("task extensions respected", "scheduler")
{
	rynx::this_thread::rynx_thread_raii obj;
	struct component {
		int a;
	};

	rynx::scheduler::task_scheduler scheduler;
	auto* context = scheduler.make_context();
	
	auto extension_task_completed = std::make_shared<std::atomic<int>>(0);

	context->add_task("TestRead", [=](rynx::scheduler::task& context) mutable {
		context.extend_task_independent("extension", [=]() mutable {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			*extension_task_completed = 1;
		});

		auto dependent_task = context.make_task("dependent task", [=]() {
			REQUIRE(extension_task_completed->load() == 1);
			*extension_task_completed = 2;
		});
		dependent_task.depends_on(context);
	});

	scheduler.start_frame();
	scheduler.wait_until_complete();

	REQUIRE(extension_task_completed->load() == 2);
}

TEST_CASE("task extensions respected nested", "scheduler")
{
	rynx::this_thread::rynx_thread_raii obj;
	struct component {
		int a;
	};

	rynx::scheduler::task_scheduler scheduler;
	auto* context = scheduler.make_context();

	auto extension_task_completed = std::make_shared<std::atomic<int>>(0);

	context->add_task("TestRead", [=](rynx::scheduler::task& context) mutable {
		auto task1 = context.extend_task_independent("extension", [=](rynx::scheduler::task& inner_context) mutable {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			REQUIRE(extension_task_completed->load() == 0);
			*extension_task_completed = 1;

			inner_context.extend_task_independent("inner extension", [=]() mutable {
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
				REQUIRE(extension_task_completed->load() == 1);
				*extension_task_completed = 2;
			});
		});

		auto dependent_task = context.make_task("dependent task", [=]() {
			REQUIRE(extension_task_completed->load() == 2);
			*extension_task_completed = 3;
		});
		dependent_task.depends_on(context);
	});

	scheduler.start_frame();
	scheduler.wait_until_complete();

	REQUIRE(extension_task_completed->load() == 3);
}

TEST_CASE("task parallel for dependencies", "scheduler")
{
	rynx::this_thread::rynx_thread_raii obj;

	auto test_state = std::make_shared<std::atomic<int>>(0);

	std::vector<int> data = {1, 2, 3, 4};
	
	rynx::scheduler::task_scheduler scheduler;
	auto* context = scheduler.make_context();
	context->add_task("test", [&data, test_state](rynx::scheduler::task& task_context) {
		auto parfor_barrier = task_context.parallel().for_each(0, data.size()).deferred_work().for_each([&data, test_state](int64_t index) mutable {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			*test_state += 1;
		});
	});
}

TEST_CASE("ecs parallel for dependencies", "scheduler")
{
	rynx::this_thread::rynx_thread_raii obj;
	
	struct component {
		int a;
	};

	rynx::ecs ecs;
	ecs.create(component{ 1 });
	ecs.create(component{ 2 });
	ecs.create(component{ 3 });

	auto test_state = std::make_shared<std::atomic<int>>(0);

	rynx::scheduler::task_scheduler scheduler;
	auto* context = scheduler.make_context();
	context->add_task("test", [&ecs, test_state](rynx::scheduler::task& task_context) {
		auto task1 = ecs.query().for_each_parallel(task_context, [test_state](component& c) {
			REQUIRE(test_state->load() >= 3);
			*test_state += 1;
		});

		auto task2 = ecs.query().for_each_parallel(task_context, [test_state](component& c) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			REQUIRE(test_state->load() < 3);
			*test_state += 1;
		});

		task1.depends_on(task2);
	});

	scheduler.start_frame();
	scheduler.wait_until_complete();

	REQUIRE(test_state->load() == 6);
}

TEST_CASE("ecs parallel for dependencies to outside task", "scheduler")
{
	rynx::this_thread::rynx_thread_raii obj;

	struct component {
		int a;
	};

	rynx::ecs ecs;
	ecs.create(component{ 1 });
	ecs.create(component{ 2 });
	ecs.create(component{ 3 });

	auto test_state = std::make_shared<std::atomic<int>>(0);

	rynx::scheduler::task_scheduler scheduler;
	auto* context = scheduler.make_context();
	
	{
		auto actual_work_task = context->add_task("test", [&ecs, test_state](rynx::scheduler::task& task_context) {
			auto inner_task = task_context.extend_task_execute_parallel("test", [&ecs, test_state](rynx::scheduler::task& task_context) {
				ecs.query().for_each_parallel(task_context, [test_state](component& c) {
					std::this_thread::sleep_for(std::chrono::milliseconds(5));
					REQUIRE(test_state->load() < 3);
					*test_state += 1;
					});
				});
		});

		auto outer_task = context->add_task("test", [&ecs, test_state](rynx::scheduler::task& task_context) {
			REQUIRE(test_state->load() == 3);
		});

		outer_task.depends_on(actual_work_task);
	}

	scheduler.start_frame();
	scheduler.wait_until_complete();

	REQUIRE(test_state->load() == 3);
}