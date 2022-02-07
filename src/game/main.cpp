

#include <rynx/application/application.hpp>
#include <rynx/application/visualisation/debug_visualisation.hpp>
#include <rynx/application/logic.hpp>
#include <rynx/application/render.hpp>
#include <rynx/application/simulation.hpp>

#include <rynx/input/mapped_input.hpp>

#include <rynx/tech/timer.hpp>
#include <rynx/tech/collision_detection.hpp>

#include <rynx/scheduler/task_scheduler.hpp>

#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/graphics/framebuffer.hpp>
#include <rynx/graphics/renderer/screenspace.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>

#include <game/sample_application.hpp>
#include <rynx/audio/audio.hpp>

#include <iostream>
#include <thread>

#include <cmath>
#include <memory>



#include <rynx/filesystem/virtual_filesystem.hpp>

int main(int /* argc */, char** /* argv */) {

	rynx::filesystem::vfs fs;
	
	fs.mount().memory_directory("/mem/");
	fs.mount().native_directory("./", "/wd/");
	// fs.mount().native_directory("./folder/", "/wd/");

	{
		rynx::unordered_map<std::string, std::vector<std::string>> test_data;
		test_data["123"].emplace_back("456");
		test_data["abc"].emplace_back("def");

		auto file = fs.open_write("/mem/test.lol");
		fs.open_write("/mem/fol/der/test.lol");
		rynx::serialize(test_data, *file);
	}

	{
		auto file = fs.open_read("/mem/test.lol");
		auto recovered = rynx::deserialize<rynx::unordered_map<std::string, std::vector<std::string>>>(*file);
		for (auto&& entry : recovered) {
			std::cout << entry.first << ": " << entry.second.front() << std::endl;
		}
	}

	{
		auto file = fs.open_write("/wd/test.lol");
		file->write(3);
		file->write(4);
	}

	{
		auto file = fs.open_read("/wd/test.lol");
		std::cout << file->read<int32_t>() << std::endl;
		std::cout << file->read<int32_t>() << std::endl;
	}

	fs.mount().native_file("./test.lol", "/wd/hehe.lol");

	{
		auto file = fs.open_read("/wd/hehe.lol");
		std::cout << file->read<int32_t>() << std::endl;
		std::cout << file->read<int32_t>() << std::endl;
	}

	std::cout << std::endl << "enumerating /" << std::endl;
	for (auto&& file : fs.enumerate_files("/", rynx::filesystem::recursive::yes)) {
		std::cout << file << std::endl;
	}

	std::cout << std::endl << "enumerating /" << std::endl;
	for (auto&& file : fs.enumerate_directories("/", rynx::filesystem::recursive::yes)) {
		std::cout << file << std::endl;
	}

	std::cout << std::endl << "enumerating /" << std::endl;
	for (auto&& file : fs.enumerate("/", rynx::filesystem::recursive::yes)) {
		std::cout << file << std::endl;
	}
	
	std::cout << std::endl << "enumerating /wd/" << std::endl;
	for (auto&& file : fs.enumerate_files("/wd", rynx::filesystem::recursive::yes)) {
		std::cout << file << std::endl;
	}
	
	std::cout << std::endl << "enumerating /wd/folder/" << std::endl;
	for (auto&& file : fs.enumerate_files("/wd/folder/", rynx::filesystem::recursive::yes)) {
		std::cout << file << std::endl;
	}

	std::cout << std::endl << "enumerating /mem/" << std::endl;
	for (auto&& file : fs.enumerate_files("/mem/", rynx::filesystem::recursive::yes)) {
		std::cout << file << std::endl;
	}

	std::cout << std::endl << "enumerating /mem/fol/" << std::endl;
	for (auto&& file : fs.enumerate_files("/mem/fol/", rynx::filesystem::recursive::yes)) {
		std::cout << file << std::endl;
	}

	fs.remove("/wd/hehe.lol");
	fs.remove("/wd/test.lol");
	fs.remove("/mem/test.lol");
	
	std::cout << std::endl << "enumerating /" << std::endl;
	for (auto&& file : fs.enumerate("/", rynx::filesystem::recursive::yes)) {
		std::cout << file << std::endl;
	}

	return 0;


	// uses this thread services of rynx, for example in cpu performance profiling.
	rynx::this_thread::rynx_thread_raii rynx_thread_services_required_token;

	SampleApplication application;
	application.openWindow(1920, 1080);
	application.startup_load();
	application.set_default_frame_processing();

	rynx::scheduler::task_scheduler& scheduler = application.scheduler();
	rynx::application::simulation& base_simulation = application.simulation();
	rynx::ecs& ecs = *base_simulation.m_ecs;

	application.set_resources();
	application.set_simulation_rules();

	rynx::mapped_input& gameInput = application.simulation_context()->get_resource<rynx::mapped_input>();
	rynx::observer_ptr<rynx::camera> camera = application.simulation_context()->get_resource_ptr<rynx::camera>();
	camera->setProjection(0.02f, 20000.0f, application.aspectRatio());
	camera->setPosition({ 0, 0, 300 });
	camera->setDirection({ 0, 0, -1 });
	camera->tick(1.0f);

	// setup some debug controls
	
	auto menuCamera = std::make_shared<rynx::camera>();
	menuCamera->setPosition({ 0, 0, 1 });
	menuCamera->setDirection({ 0, 0, -1 });
	menuCamera->setUpVector({ 0, 1, 0 });
	menuCamera->tick(1.0f);


	// TODO: Move under editor ruleset? or somewhere
	auto cameraUp = gameInput.generateAndBindGameKey('I', "cameraUp");
	auto cameraLeft = gameInput.generateAndBindGameKey('J', "cameraLeft");
	auto cameraRight = gameInput.generateAndBindGameKey('L', "cameraRight");
	auto cameraDown = gameInput.generateAndBindGameKey('K', "cameraDown");
	auto zoomIn = gameInput.generateAndBindGameKey('U', "zoomIn");
	auto zoomOut = gameInput.generateAndBindGameKey('O', "zoomOut");

	// TODO - MOVE AUDIO SETUP SOMEWHERE ELSE
	// setup sound system
	rynx::sound::audio_system audio;
	
	// uint32_t soundIndex = audio.load("test.ogg");
	rynx::sound::configuration config;
	audio.open_output_device(64, 256);

	// TODO
	// render.debug_draw_binary_config(program_state_editor_running);

	auto camera_orientation_key = gameInput.generateAndBindGameKey(gameInput.getMouseKeyPhysical(1), "camera_orientation");

	rynx::timer dt_timer;
	float dt = 1.0f / 120.0f;
	
	/*
	if (gameInput.isKeyPressed(cameraUp)) {
		config = audio.play_sound(soundIndex, rynx::vec3f(), rynx::vec3f());
	}
	*/

	// TODO: Main loop should probably be implemented under application?
	//       User should not need to worry about logic/render frame rate decouplings.
	while (!application.isExitRequested()) {
		
		dt = std::min(0.016f, std::max(0.001f, dt_timer.time_since_last_access_ms() * 0.001f));

		rynx_profile("Main", "frame");
		
		auto mousePos = application.input()->getCursorPosition();

		{
			rynx_profile("Main", "update camera");

			if (gameInput.isKeyDown(camera_orientation_key)) {
				camera->rotate(gameInput.mouseDelta());
			}

			camera->tick(3.0f * dt);
			camera->rebuild_view_matrix();
		}

		application.user().frame_begin();
		application.user().menu_input();
		application.user().logic_frame_start();
		application.user().logic_frame_wait_end();
		application.user().menu_frame_execute();
		application.user().wait_gpu_done_and_swap();
		application.user().render();
		application.user().frame_end();

		// menu input must happen first before tick. in case menu components
		// reserve some input as private.


		{
			// TODO: Input handling should probably not be here.
			{
				// TODO: inhibit menu dedicated inputs
				const float camera_translate_multiplier = 400.4f * dt;
				const float camera_zoom_multiplier = (1.0f - dt);
				if (gameInput.isKeyDown(cameraUp)) { camera->translate(camera->local_up() * camera_translate_multiplier); }
				if (gameInput.isKeyDown(cameraLeft)) { camera->translate(camera->local_left() * camera_translate_multiplier); }
				if (gameInput.isKeyDown(cameraRight)) { camera->translate(-camera->local_left() * camera_translate_multiplier); }
				if (gameInput.isKeyDown(cameraDown)) { camera->translate(-camera->local_up() * camera_translate_multiplier); }
				if (gameInput.isKeyDown(zoomOut)) {
					camera->translate(
						camera->position() *
						rynx::vec3f{ 0, 0, 1.0f - 1.0f * camera_zoom_multiplier }
					);
				}
				if (gameInput.isKeyDown(zoomIn)) {
					camera->translate(
						camera->position() *
						rynx::vec3f{ 0, 0, 1.0f - 1.0f / camera_zoom_multiplier }
					);
				}
				if (gameInput.isKeyClicked(rynx::key::physical('X'))) {
					rynx::profiling::write_profile_log();
				}
			}
		}
		
		{
			rynx_profile("Main", "Clean up dead entitites");
			auto ids_dead = ecs.query().in<rynx::components::dead>().ids();
			base_simulation.m_logic.entities_erased(*base_simulation.m_context, ids_dead);
			ecs.erase(ids_dead);
		}

		// lets avoid using all computing power for now.
		// TODO: Proper time tracking for frames.
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return 0;
}
