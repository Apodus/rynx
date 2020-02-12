#include <game/view.hpp>
#include <game/gametypes.hpp>
#include <game/logic.hpp>

#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/ecs.hpp>

#include <iostream>
#include <thread>

#include <cmath>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/graphics/mesh/polygonTesselator.hpp>

#include <rynx/application/application.hpp>
#include <rynx/graphics/text/fontdata/lenka.hpp>
#include <rynx/graphics/text/fontdata/consolamono.hpp>

#include <rynx/application/logic.hpp>
#include <rynx/application/render.hpp>
#include <rynx/application/simulation.hpp>

#include <rynx/rulesets/collisions.hpp>
#include <rynx/rulesets/particles.hpp>

#include <rynx/input/mapped_input.hpp>
#include <rynx/tech/smooth_value.hpp>
#include <rynx/tech/timer.hpp>

#include <game/logic/shooting.hpp>
#include <game/logic/keyboard_movement.hpp>

#include <game/logic/enemies/minilisk.hpp>
#include <game/logic/enemies/minilisk_spawner.hpp>

#include <game/visual/bullet_rendering.hpp>
#include <rynx/scheduler/task_scheduler.hpp>

#include <memory>

#include <rynx/menu/Button.hpp>
#include <rynx/menu/Slider.hpp>
#include <rynx/menu/Div.hpp>

#include <rynx/graphics/framebuffer.hpp>
#include <rynx/graphics/renderer/screenspace.hpp>

#include <rynx/application/render.hpp>

#include <rynx/rulesets/frustum_culling.hpp>
#include <rynx/rulesets/motion.hpp>
#include <rynx/rulesets/physics/springs.hpp>

int main(int argc, char** argv) {
	
	// uses this thread services of rynx, for example in cpu performance profiling.
	rynx::this_thread::rynx_thread_raii rynx_thread_services_required_token;
	
	Font fontLenka(Fonts::setFontLenka());
	Font fontConsola(Fonts::setFontConsolaMono());

	rynx::application::Application application;
	application.openWindow(1920, 1080);
	application.loadTextures("../textures/textures.txt");
	application.meshRenderer().loadDefaultMesh("Empty");

	auto meshes = application.meshRenderer().meshes();
	{
		meshes->create("ball", Shape::makeCircle(1.0f, 32), "Hero");
		meshes->create("circle_empty", Shape::makeCircle(1.0f, 32), "Empty");
		meshes->create("square_empty", Shape::makeBox(1.0f), "Empty");
		meshes->create("particle_smoke", Shape::makeBox(1.0f), "Smoke");
		meshes->create("square_rope", Shape::makeBox(1.0f), "Rope");
	}

	rynx::scheduler::task_scheduler scheduler;
	rynx::application::simulation base_simulation(scheduler);

	std::shared_ptr<Camera> camera = std::make_shared<Camera>();
	camera->setProjection(0.02f, 20000.0f, application.aspectRatio());

	rynx::input::mapped_input gameInput(application.input());


	rynx::collision_detection* detection = new rynx::collision_detection();
	auto& collisionDetection = *detection;

	// setup collision detection
	rynx::collision_detection::category_id collisionCategoryDynamic = collisionDetection.add_category();
	rynx::collision_detection::category_id collisionCategoryStatic = collisionDetection.add_category();
	rynx::collision_detection::category_id collisionCategoryProjectiles = collisionDetection.add_category();

	{
		collisionDetection.enable_collisions_between(collisionCategoryDynamic, collisionCategoryDynamic); // enable dynamic <-> dynamic collisions
		collisionDetection.enable_collisions_between(collisionCategoryDynamic, collisionCategoryStatic.ignore_collisions()); // enable dynamic <-> static collisions

		collisionDetection.enable_collisions_between(collisionCategoryProjectiles, collisionCategoryStatic.ignore_collisions()); // projectile <-> static
		collisionDetection.enable_collisions_between(collisionCategoryProjectiles, collisionCategoryDynamic); // projectile <-> dynamic
	}

	// set additional resources your simulation wants to use.
	{
		base_simulation.set_resource(&collisionDetection);
	}

	// TODO: use std::observer_ptr
	game::logic::minilisk_test_spawner_logic* spawner = nullptr;

	// setup game logic
	{
		auto ruleset_collisionDetection = std::make_unique<rynx::ruleset::physics_2d>();
		auto ruleset_particle_update = std::make_unique<rynx::ruleset::particle_system>();
		
		auto ruleset_shooting = std::make_unique<game::logic::shooting_logic>(gameInput, collisionCategoryProjectiles);
		auto ruleset_keyboardMovement = std::make_unique<game::logic::keyboard_movement_logic>(gameInput);
		auto ruleset_frustum_culling = std::make_unique<rynx::ruleset::frustum_culling>(camera);

		auto ruleset_bullet_hits = std::make_unique<game::logic::bullet_hitting_logic>(collisionDetection, collisionCategoryDynamic, collisionCategoryStatic, collisionCategoryProjectiles);
		auto ruleset_minilisk = std::make_unique<game::logic::minilisk_logic>();
		auto ruleset_minilisk_gen = std::make_unique<game::logic::minilisk_test_spawner_logic>(application.meshRenderer().meshes(), collisionCategoryDynamic);
		
		auto ruleset_motion_updates = std::make_unique<rynx::ruleset::motion_updates>(vec3<float>(0, -60.8f, 0));
		auto ruleset_physical_springs = std::make_unique<rynx::ruleset::physics::springs>();

		spawner = ruleset_minilisk_gen.get();

		ruleset_bullet_hits->depends_on(*ruleset_collisionDetection);
		ruleset_physical_springs->depends_on(*ruleset_motion_updates);
		ruleset_collisionDetection->depends_on(*ruleset_motion_updates);
		ruleset_frustum_culling->depends_on(*ruleset_motion_updates);


		base_simulation.add_rule_set(std::move(ruleset_motion_updates));
		base_simulation.add_rule_set(std::move(ruleset_physical_springs));

		base_simulation.add_rule_set(std::move(ruleset_collisionDetection));
		base_simulation.add_rule_set(std::move(ruleset_particle_update));
		base_simulation.add_rule_set(std::move(ruleset_frustum_culling));

		base_simulation.add_rule_set(std::move(ruleset_shooting));
		base_simulation.add_rule_set(std::move(ruleset_keyboardMovement));

		base_simulation.add_rule_set(std::move(ruleset_bullet_hits));
		base_simulation.add_rule_set(std::move(ruleset_minilisk));
		base_simulation.add_rule_set(std::move(ruleset_minilisk_gen));
	}

	// gameRenderer.add_graphics_step(std::make_unique<game::hitpoint_bar_renderer>(&application.meshRenderer()));
	// gameRenderer.add_graphics_step(std::make_unique<game::visual::bullet_renderer>(&application.meshRenderer()));
	// gameRenderer.add_graphics_step(std::make_unique<game::visual::hero_renderer>(&application.meshRenderer()));

	rynx::smooth<vec3<float>> cameraPosition(0.0f, 0.0f, 300.0f);

	rynx::ecs& ecs = base_simulation.m_ecs;

	// setup simulation initial state
	{
		/*
		ecs.create(
			game::hero(),
			game::health(),
			rynx::components::position(),
			rynx::components::motion(),
			rynx::components::mass({ 1.0f }),
			rynx::components::radius(1.0f),
			rynx::components::collision_category(collisionCategoryDynamic),
			rynx::components::color(),
			rynx::components::dampening({ 0.86f, 0.86f }),
			game::components::weapon(),
			rynx::components::frame_collisions()
		);
		*/

		math::rand64 random;

		// create some boxes for testing.
		if constexpr (false) {
			for (int i = 0; i < 16; ++i)
				ecs.create(
					rynx::components::position(vec3<float>(-80.0f + i * 8.0f, 0.0f, 0.0f), i * 2.0f),
					rynx::components::collision_category(collisionCategoryDynamic),
					rynx::components::boundary({ Shape::makeBox(1.0f + 2.0f * (random() & 127) / 127.0f).generateBoundary_Outside() }),
					rynx::components::radius(math::sqrt_approx(16 + 16)),
					rynx::components::color({ 1,1,0,1 }),
					rynx::components::motion({ 0, 0, 0 }, 0),
					rynx::components::dampening({ 0.93f, 0.98f }),
					rynx::components::ignore_gravity()
				);
		}
		
		// TODO: radius calculation from boundary (bounding radius or something)
		auto makeBox_inside = [&](vec3<float> pos, float angle, float edgeLength, float angular_velocity) {
			return base_simulation.m_ecs.create(
				rynx::components::position(pos, angle),
				rynx::components::collision_category(collisionCategoryStatic),
				rynx::components::boundary({ Shape::makeAAOval(0.5f, 40, edgeLength, edgeLength * 0.5f).generateBoundary_Inside() }),
				rynx::components::radius(math::sqrt_approx(2 * (edgeLength * edgeLength * 0.25f))),
				rynx::components::color({ 0.5f, 0.2f, 1.0f, 1.0f }),
				rynx::components::motion({ 0, 0, 0 }, angular_velocity),
				rynx::components::physical_body(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 0.0f, 1.0f),
				rynx::components::ignore_gravity()
			);
		};

		auto makeBox_outside = [&](vec3<float> pos, float angle, float edgeLength, float angular_velocity) {
			return base_simulation.m_ecs.create(
				rynx::components::position(pos, angle),
				rynx::components::collision_category(collisionCategoryStatic),
				rynx::components::boundary({ Shape::makeRectangle(edgeLength, 5.0f).generateBoundary_Outside() }),
				rynx::components::radius(math::sqrt_approx(2 * (edgeLength * edgeLength * 0.25f))),
				rynx::components::color({ 0.2f, 1.0f, 0.3f, 1.0f }),
				rynx::components::motion({ 0, 0, 0 }, angular_velocity),
				rynx::components::physical_body(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), 0.0f, 1.0f),
				rynx::components::ignore_gravity(),
				rynx::components::dampening{0.50f, 1.0f}
			);
		};

		/*
		for (int i = 0; i < 20; ++i)
			makeBox_inside({ +20, +15 - i * 15.0f, 0 }, 0.0f + i * 0.1f, 2.0f, -0.05f);
		*/

		// makeBox_inside({ -5, -30, 0 }, +0.3f, 40.f, -0.025f);
		makeBox_outside({ -15, -50, 0 }, -0.3f, 265.f, +0.58f);

		// makeBox_inside({ -65, -100, 0 }, 0.f, 60.f, -0.030f);
		makeBox_outside({ -65, -100, 0 }, -0.3f, 65.f, -0.24f);

		// makeBox_inside({ +25, -120, 0 }, +0.5f, 80.f, +0.015f);
		makeBox_outside({ +25, -120, 0 }, -0.3f, 65.f, -0.12f);

		makeBox_outside({ 0, -170, 0 }, -0.0f, 100.0f, 0.f);
		makeBox_outside({ -80, -160, 0 }, -0.3f, 1000.0f, 0.f);
		makeBox_outside({ +80, -160, 0 }, +0.3f, 1000.0f, 0.f);
	}

	// setup some debug controls
	float sleepTime = 0.9f;
	auto slowTime = gameInput.generateAndBindGameKey('X', "slow time");
	auto fastTime = gameInput.generateAndBindGameKey('C', "fast time");

	auto zoomOut = gameInput.generateAndBindGameKey('1', "zoom out");
	auto zoomIn = gameInput.generateAndBindGameKey('2', "zoom in");

	auto menuCamera = std::make_shared<Camera>();

	gameInput.generateAndBindGameKey(gameInput.getMouseKeyPhysical(0), "menuCursorActivation");
	
	auto cameraUp = gameInput.generateAndBindGameKey('I', "cameraUp");
	auto cameraLeft = gameInput.generateAndBindGameKey('J', "cameraLeft");
	auto cameraRight = gameInput.generateAndBindGameKey('L', "cameraRight");
	auto cameraDown = gameInput.generateAndBindGameKey('K', "cameraDown");
	
	rynx::menu::Div root({ 1, 1, 0 });

	struct debug_conf {
		bool visualize_dynamic_collisions = false;
		bool visualize_static_collisions = false;
		bool visualize_projectile_collisions = false;
	};

	debug_conf conf;

	// construct menus
	{
		auto sampleButton = std::make_shared<rynx::menu::Button>(*application.textures(), "Frame", &root, vec3<float>(0.4f, 0.1f, 0), vec3<float>(), 0.14f);
		auto sampleButton2 = std::make_shared<rynx::menu::Button>(*application.textures(), "Frame", &root, vec3<float>(0.4f, 0.1f, 0), vec3<float>(), 0.16f);
		auto sampleButton3 = std::make_shared<rynx::menu::Button>(*application.textures(), "Frame", &root, vec3<float>(0.4f, 0.1f, 0), vec3<float>(), 0.18f);
		auto sampleSlider = std::make_shared<rynx::menu::SlideBarVertical>(*application.textures(), "Frame", "Selection", &root, vec3<float>(0.4f, 0.1f, 0));
		auto megaSlider = std::make_shared<rynx::menu::SlideBarVertical>(*application.textures(), "Frame", "Selection", &root, vec3<float>(0.4f, 0.1f, 0));

		sampleButton->text("Dynamics").font(&fontConsola);
		sampleButton->alignToInnerEdge(&root, rynx::menu::Align::BOTTOM_LEFT);
		sampleButton->color_frame(Color::RED);
		sampleButton->onClick([&conf, self = sampleButton.get()]() {
			bool new_value = !conf.visualize_dynamic_collisions;
			conf.visualize_dynamic_collisions = new_value;
			if (new_value) {
				self->color_frame(Color::GREEN);
			}
			else {
				self->color_frame(Color::RED);
			}
		});

		sampleButton2->text("Log Profile").font(&fontConsola);
		sampleButton2->alignToOuterEdge(sampleButton.get(), rynx::menu::Align::RIGHT);
		sampleButton2->alignToInnerEdge(sampleButton.get(), rynx::menu::Align::BOTTOM);
		sampleButton2->onClick([]() {
			rynx::profiling::write_profile_log();
		});

		sampleButton3->text("Statics").font(&fontConsola);
		sampleButton3->alignToOuterEdge(sampleButton2.get(), rynx::menu::Align::TOP);
		sampleButton3->alignToInnerEdge(sampleButton2.get(), rynx::menu::Align::LEFT);
		sampleButton3->onClick([&conf, self = sampleButton3.get(), &root]() {
			bool new_value = !conf.visualize_static_collisions;
			conf.visualize_static_collisions = new_value;
			if (new_value) {
				self->color_frame(Color::GREEN);
			}
			else {
				self->color_frame(Color::RED);
			}
		});

		sampleSlider->alignToInnerEdge(&root, rynx::menu::Align::TOP_RIGHT);
		sampleSlider->onValueChanged([spawner](float f) {
			spawner->how_often_to_spawn = uint64_t(1 + int(f * 300.0f));
		});

		megaSlider->alignToOuterEdge(sampleSlider.get(), rynx::menu::Align::BOTTOM);
		megaSlider->alignToInnerEdge(sampleSlider.get(), rynx::menu::Align::LEFT);
		megaSlider->onValueChanged([spawner](float f) {
			spawner->x_spawn = f * 200.0f - 100.0f;
		});

		root.addChild(sampleButton);
		root.addChild(sampleButton2);
		root.addChild(sampleButton3);
		root.addChild(sampleSlider);
		root.addChild(megaSlider);
	}

	std::atomic<size_t> tickCounter = 0;

	std::atomic<bool> dead_lock_detector_keepalive = true;
	std::thread dead_lock_detector([&]() {
		size_t prev_tick = 994839589;
		while (dead_lock_detector_keepalive) {
			if (prev_tick == tickCounter && dead_lock_detector_keepalive) {
				scheduler.dump();
				return;
			}
			prev_tick = tickCounter;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	});

	auto fbo_menu = rynx::graphics::framebuffer::config()
		.set_default_resolution(1920, 1080)
		.add_rgba8_target("color")
		.construct(application.textures(), "menu");

	rynx::graphics::screenspace_draws(); // initialize gpu buffers for screenspace ops.
	rynx::application::renderer render(application, camera);

	auto camera_orientation_key = gameInput.generateAndBindGameKey(gameInput.getMouseKeyPhysical(1), "camera_orientation");

	rynx::timer timer;
	rynx::numeric_property<float> logic_time;
	rynx::numeric_property<float> render_time;
	rynx::numeric_property<float> swap_time;
	rynx::numeric_property<float> total_time;

	rynx::timer frame_timer_dt;
	float dt = 1.0f / 120.0f;
	while (!application.isExitRequested()) {
		rynx_profile("Main", "frame");
		frame_timer_dt.reset();
		
		{
			rynx_profile("Main", "start frame");
			application.startFrame();
		}
		
		auto mousePos = application.input()->getCursorPosition();
		cameraPosition.tick(dt * 3);

		{
			rynx_profile("Main", "update camera");

			static vec3f camera_direction(0, 0, 0);
			
			if (gameInput.isKeyDown(camera_orientation_key)) {
				auto mouseDelta = gameInput.mouseDelta();
				camera_direction += mouseDelta;
			}

			rynx::matrix4 rotator_x;
			rynx::matrix4 rotator_y;
			rotator_x.discardSetRotation(camera_direction.x, 0, 1, 0);
			rotator_y.discardSetRotation(camera_direction.y, -1, 0, 0);

			vec3f direction = rotator_y * rotator_x * vec3f(0, 0, -1);

			camera->setPosition(cameraPosition);
			camera->setDirection(direction);
			camera->setProjection(0.02f, 2000.0f, application.aspectRatio());
			camera->rebuild_view_matrix();
		}

		{
			const float camera_translate_multiplier = 400.4f * dt;
			const float camera_zoom_multiplier = (1.0f - dt * 3.0f);
			if (gameInput.isKeyDown(cameraUp)) { cameraPosition += camera->local_forward() * camera_translate_multiplier; }
			if (gameInput.isKeyDown(cameraLeft)) { cameraPosition += camera->local_left() * camera_translate_multiplier; }
			if (gameInput.isKeyDown(cameraRight)) { cameraPosition -= camera->local_left() * camera_translate_multiplier; }
			if (gameInput.isKeyDown(cameraDown)) { cameraPosition -= camera->local_forward() * camera_translate_multiplier; }
			// if (gameInput.isKeyDown(zoomOut)) { cameraPosition *= vec3<float>(1, 1.0f, 1.0f * camera_zoom_multiplier); }
			// if (gameInput.isKeyDown(zoomIn)) { cameraPosition *= vec3<float>(1, 1.0f, 1.0f / camera_zoom_multiplier); }
		}

		{
			float cameraHeight = cameraPosition->z;
			gameInput.mouseWorldPosition(
				(cameraPosition* vec3<float>{1,1,0}) +
				mousePos * vec3<float>(cameraHeight, cameraHeight / application.aspectRatio(), 1.0f)
			);
		}

		{
			rynx_profile("Main", "Input handling");
			// TODO: Simulation API
			std::vector<std::unique_ptr<rynx::application::logic::iaction>> userActions = base_simulation.m_logic.onInput(gameInput, ecs);
			for (auto&& action : userActions) {
				action->apply(ecs);
			}
		}

		timer.reset();
		{
			rynx_profile("Main", "Construct frame tasks");
			base_simulation.generate_tasks(dt);
		}

		{
			rynx_profile("Main", "Start scheduler");
			scheduler.start_frame();
		}

		{
			rynx_profile("Main", "Wait for frame end");
			scheduler.wait_until_complete();
			++tickCounter;
		}

		auto logic_time_us = timer.time_since_last_access_us();
		logic_time.observe_value(logic_time_us / 1000.0f); // down to milliseconds.
		
		// menu input is part of logic, not visualization. must tick every frame.
		root.input(gameInput);
		root.tick(dt, application.aspectRatio());

		// should we render or not.
		if (true || tickCounter.load() % 16 == 3) {
			timer.reset();
			rynx_profile("Main", "graphics");

			{
				rynx_profile("Main", "prepare");
				render.prepare(base_simulation.m_context);
				scheduler.start_frame();

				// while waiting for computing to be completed, draw menus.
				{
					rynx_profile("Main", "Menus");
					fbo_menu->bind_as_output();
					fbo_menu->clear();

					application.meshRenderer().setDepthTest(false);

					// 2, 2 is the size of the entire screen (in case of 1:1 aspect ratio) for menu camera. left edge is [-1, 0], top right is [+1, +1], etc.
					// so we make it size 2,2 to cover all of that. and then take aspect ratio into account by dividing the y-size.
					root.scale_local({ 2 , 2 / application.aspectRatio(), 0 });
					menuCamera->setProjection(0.01f, 50.0f, application.aspectRatio());
					menuCamera->setPosition({ 0, 0, 1 });
					menuCamera->rebuild_view_matrix();

					application.meshRenderer().setCamera(menuCamera);
					application.textRenderer().setCamera(menuCamera);
					application.meshRenderer().cameraToGPU();
					root.visualise(application.meshRenderer(), application.textRenderer());

					auto num_entities = ecs.size();
					float info_text_pos_y = +0.1f;
					auto get_min_avg_max = [](rynx::numeric_property<float>& prop) {
						return std::to_string(prop.min()) + "/" + std::to_string(prop.avg()) + "/" + std::to_string(prop.max()) + "ms";
					};

					application.textRenderer().drawText(std::string("logic:    ") + get_min_avg_max(logic_time), -0.9f, 0.40f + info_text_pos_y, 0.05f, Color::DARK_GREEN, rynx::TextRenderer::Align::Left, fontConsola);
					application.textRenderer().drawText(std::string("draw:     ") + get_min_avg_max(render_time), -0.9f, 0.35f + info_text_pos_y, 0.05f, Color::DARK_GREEN, rynx::TextRenderer::Align::Left, fontConsola);
					application.textRenderer().drawText(std::string("swap:     ") + get_min_avg_max(swap_time), -0.9f, 0.30f + info_text_pos_y, 0.05f, Color::DARK_GREEN, rynx::TextRenderer::Align::Left, fontConsola);
					application.textRenderer().drawText(std::string("total:    ") + get_min_avg_max(total_time), -0.9f, 0.25f + info_text_pos_y, 0.05f, Color::DARK_GREEN, rynx::TextRenderer::Align::Left, fontConsola);
					application.textRenderer().drawText(std::string("bodies:   ") + std::to_string(ecs.query().in<rynx::components::physical_body>().count()), -0.9f, 0.20f + info_text_pos_y, 0.05f, Color::DARK_GREEN, rynx::TextRenderer::Align::Left, fontConsola);
					application.textRenderer().drawText(std::string("entities: ") + std::to_string(num_entities), -0.9f, 0.15f + info_text_pos_y, 0.05f, Color::DARK_GREEN, rynx::TextRenderer::Align::Left, fontConsola);
					application.textRenderer().drawText(std::string("frustum culled: ") + std::to_string(ecs.query().in<rynx::components::frustum_culled>().count()), -0.9f, 0.10f + info_text_pos_y, 0.05f, Color::DARK_GREEN, rynx::TextRenderer::Align::Left, fontConsola);
					application.textRenderer().drawText(std::string("visible: ") + std::to_string(ecs.query().notIn<rynx::components::frustum_culled>().count()), -0.9f, 0.05f + info_text_pos_y, 0.05f, Color::DARK_GREEN, rynx::TextRenderer::Align::Left, fontConsola);
				}

				scheduler.wait_until_complete();
			}

			auto render_time_us = timer.time_since_last_access_us();
			render_time.observe_value(render_time_us / 1000.0f);

			/*
			auto mpos = gameInput.mouseWorldPosition();
			{
				rynx_profile("Main", "draw cursor");

				rynx::matrix4 m;
				m.discardSetTranslate(mpos);
				m.scale(0.5f);
				application.meshRenderer().drawMesh(*meshes->get("circle_empty"), m, "Empty");
			}
			*/

			{
				rynx_profile("Main", "draw");

				render.execute();

				// application.meshRenderer().setDepthTest(true);

				// TODO: debug visualisations should be drawn on their own fbo.
				/*
				{
					// visualize collision detection structure.
					if (conf.visualize_dynamic_collisions) {
						std::array<vec4<float>, 5> node_colors{ vec4<float>{0, 1, 0, 0.2f}, {0, 0, 1, 0.2f}, {1, 0, 0, 0.2f}, {1, 1, 0, 0.2f}, {0, 1, 1, 0.2f} };
						collisionDetection.get(collisionCategoryDynamic)->forEachNode([&](vec3<float> pos, float radius, int depth) {
							matrix4 m;
							m.discardSetTranslate(pos);
							m.scale(radius);
							float sign[2] = { -1.0f, +1.0f };
							application.meshRenderer().drawMesh(*meshes->get("circle_empty"), m, "Empty", node_colors[depth % node_colors.size()]);
						});
					}

					// visualize collision detection structure.
					if (conf.visualize_static_collisions) {
						std::array<vec4<float>, 5> node_colors{ vec4<float>{0, 1, 0, 0.2f}, {0, 0, 1, 0.2f}, {1, 0, 0, 0.2f}, {1, 1, 0, 0.2f}, {0, 1, 1, 0.2f} };
						collisionDetection.get(collisionCategoryStatic)->forEachNode([&](vec3<float> pos, float radius, int depth) {
							matrix4 m;
							m.discardSetTranslate(pos);
							m.scale(radius);
							float sign[2] = { -1.0f, +1.0f };
							application.meshRenderer().drawMesh(*meshes->get("circle_empty"), m, "Empty", node_colors[depth % node_colors.size()]);
						});
					}
				}
				*/

				{
					application.shaders()->activate_shader("fbo_color_to_bb");
					fbo_menu->bind_as_input();
					rynx::graphics::screenspace_draws::draw_fullscreen();
				}

				timer.reset();
				application.swapBuffers();
				auto swap_time_us = timer.time_since_last_access_us();
				swap_time.observe_value(swap_time_us / 1000.0f);

				total_time.observe_value((logic_time_us + render_time_us + swap_time_us) / 1000.0f);
			}
		}

		{
			rynx_profile("Main", "Clean up dead entitites");
			dt = std::min(0.016f, std::max(0.001f, frame_timer_dt.time_since_last_access_ms() * 0.001f));
			
			{
				std::vector<rynx::ecs::id> ids;
				ecs.query().for_each([&ids, dt](rynx::ecs::id id, rynx::components::lifetime& time) {
					time.value -= dt;
					if (time.value <= 0) {
						ids.emplace_back(id);
					}
				});
				for(auto&& id : ids)
					ecs.attachToEntity(id, rynx::components::dead());
			}

			ecs.query().in<rynx::components::dead>().for_each(
				[&collisionDetection](rynx::ecs::id id, rynx::components::collision_category& cat) {
					collisionDetection.erase(id.value, cat.value);
				}
			);
			
			auto erasedIds = ecs.query().in<rynx::components::dead>().ids();
			ecs.erase(erasedIds);
			base_simulation.m_logic.entities_erased(*base_simulation.m_context, erasedIds);
		}

		{
			rynx_profile("Main", "Sleep");
			// NOTE: Frame time can be edited during runtime for debugging reasons.
			if (gameInput.isKeyDown(slowTime)) { sleepTime *= 1.1f; }
			if (gameInput.isKeyDown(fastTime)) { sleepTime *= 0.9f; }

			std::this_thread::sleep_for(std::chrono::milliseconds(int(sleepTime)));
		}
	}

	dead_lock_detector_keepalive = false;
	dead_lock_detector.join();

	// PROFILE_RECORD(500); // Record last 500 ms profiling to build/bin
	return 0;
}
