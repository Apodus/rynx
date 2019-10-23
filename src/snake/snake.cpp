
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/graphics/mesh/polygonTesselator.hpp>

#include <rynx/application/application.hpp>
#include <rynx/graphics/text/fontdata/lenka.hpp>
#include <rynx/graphics/text/fontdata/consolamono.hpp>

#include <rynx/application/logic.hpp>
#include <rynx/application/render.hpp>
#include <rynx/application/simulation.hpp>

#include <rynx/application/visualisation/ball_renderer.hpp>
#include <rynx/application/visualisation/boundary_renderer.hpp>
#include <rynx/application/visualisation/mesh_renderer.hpp>

#include <rynx/rulesets/collisions.hpp>

#include <rynx/input/mapped_input.hpp>
#include <rynx/tech/smooth_value.hpp>

#include <rynx/scheduler/task_scheduler.hpp>

#include <rynx/menu/Button.hpp>
#include <rynx/menu/Slider.hpp>
#include <rynx/menu/Div.hpp>

#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/ecs.hpp>

#include <iostream>
#include <thread>
#include <memory>

int main(int argc, char** argv) {
	Font fontLenka(Fonts::setFontLenka());
	Font fontConsola(Fonts::setFontConsolaMono());

	rynx::application::Application application;
	application.openWindow(1920, 1080);
	application.loadTextures("../textures/textures.txt");

	auto mesh = PolygonTesselator<vec3<float>>().tesselate(Shape::makeCircle(1.0f, 64), application.textures().textureLimits("Hero"));
	auto emptyMesh = PolygonTesselator<vec3<float>>().tesselate(Shape::makeCircle(1.0f, 64), application.textures().textureLimits("Empty"));
	
	mesh->build();
	emptyMesh->build();

	rynx::scheduler::task_scheduler scheduler;
	rynx::application::simulation base_simulation(scheduler);

	std::shared_ptr<Camera> camera = std::make_shared<Camera>();
	camera->setProjection(0.02f, 2000.0f, application.aspectRatio());

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

	rynx::application::renderer gameRenderer;

	// set additional resources your simulation wants to use.
	{
		base_simulation.set_resource(&collisionDetection);
	}

	struct snake_head {};
	struct dead {};
	struct apple {}; // snakes collect apples
	struct health {
		int hp = 100;
	};

	// setup game logic
	{
		class snake_rules : public rynx::application::logic::iruleset {
			rynx::collision_detection::category_id m_collision_category;
			math::rand64 random;
		
		public:
			snake_rules(rynx::collision_detection::category_id id) : m_collision_category(id) {}

			virtual void onFrameProcess(rynx::scheduler::context& context) override {
				context.add_task("snake move",
					[this](rynx::ecs::edit_view<
						snake_head,
						apple,
						dead,
						rynx::components::radius,
						rynx::components::color,
						rynx::components::frame_collisions,
						rynx::components::collision_category,
						rynx::components::lifetime,
						rynx::components::motion,
						rynx::components::position> ecs, rynx::scheduler::task& task_context) {

						ecs.query().in<snake_head>().execute([this, ecs](rynx::components::position pos) mutable {
							ecs.create(
								rynx::components::position({ pos.value }),
								rynx::components::lifetime(150),
								rynx::components::radius(1.0f),
								rynx::components::collision_category(m_collision_category),
								rynx::components::color(Color::WHITE)
							);
						});

						ecs.query().in<snake_head>().execute([](rynx::components::position& pos) {
							constexpr float velocity = 0.5f;
							float sin_v = math::sin(pos.angle);
							float cos_v = math::cos(pos.angle);
							pos.value.x += cos_v * velocity;
							pos.value.y += sin_v * velocity;
						});

						std::vector<rynx::ecs::id> killed_entities;
						ecs.query().in<snake_head>().execute([&, ecs](rynx::components::frame_collisions& collisions) {
							for (auto& collision : collisions.collisions) {
								if (ecs[collision.idOfOther.value].has<apple>()) {
									// TODO: award points to snake_head player or something.
									
									// erase the collected apple
									killed_entities.emplace_back(collision.idOfOther);
								}
							}
						});

						for (auto& id : killed_entities) {
							ecs.attachToEntity(id, dead());

							// replace the collected apple with a new one.
							ecs.create(
								apple{},
								rynx::components::position({ random(-400.0f, +400.0f), random(-400.0f, +400.0f), 0.0f }, 0),
								rynx::components::motion(),
								rynx::components::radius(10.0f),
								rynx::components::collision_category(m_collision_category),
								rynx::components::color(Color::GREEN),
								rynx::components::frame_collisions()
							);
						}
					}
				);
			}
		};

		auto ruleset_collisionDetection = std::make_unique<rynx::ruleset::physics_2d>();
		auto ruleset_snake = std::make_unique<snake_rules>(collisionCategoryDynamic);
		ruleset_snake->depends_on(*ruleset_collisionDetection);
		
		base_simulation.add_rule_set(std::move(ruleset_collisionDetection));
		base_simulation.add_rule_set(std::move(ruleset_snake));
	}

	gameRenderer.addRenderer(std::make_unique<rynx::application::visualisation::ball_renderer>(emptyMesh.get(), &application.meshRenderer(), &(*camera)));
	gameRenderer.addRenderer(std::make_unique<rynx::application::visualisation::boundary_renderer>(&application.meshRenderer()));
	gameRenderer.addRenderer(std::make_unique<rynx::application::visualisation::mesh_renderer>(&application.meshRenderer()));
	
	rynx::smooth<vec3<float>> cameraPosition(0.0f, 0.0f, 1000.0f);
	rynx::ecs& ecs = base_simulation.m_ecs;

	// setup simulation initial state
	{
		math::rand64 random;

		auto createSnake = [&](float x, float y, float angle) {
			ecs.create(
				snake_head{},
				health{},
				rynx::components::position({x, y, 0.0f}, angle),
				rynx::components::motion(),
				rynx::components::radius(1.0f),
				rynx::components::collision_category(collisionCategoryDynamic),
				rynx::components::color(),
				rynx::components::frame_collisions()
			);
		};

		createSnake(random(-100.0f, 100.0f), random(-100.0f, 100.0f), random(3.0f));
		createSnake(random(-100.0f, 100.0f), random(-100.0f, 100.0f), random(3.0f));
		createSnake(random(-100.0f, 100.0f), random(-100.0f, 100.0f), random(3.0f));
		createSnake(random(-100.0f, 100.0f), random(-100.0f, 100.0f), random(3.0f));

		auto createApple = [&](float x, float y) {
			ecs.create(
				apple{},
				rynx::components::position({ x, y, 0.0f }, 0),
				rynx::components::motion(),
				rynx::components::radius(10.0f),
				rynx::components::collision_category(collisionCategoryDynamic),
				rynx::components::color(Color::GREEN),
				rynx::components::frame_collisions()
			);
		};

		for (size_t i = 0; i < 10; ++i) {
			createApple(random(-200.0f, +200.0f), random(-200.0f, +200.0f));
		}

		/*
		auto makeBox_outside = [&](vec3<float> pos, float angle, float edgeLength) {
			base_simulation.m_ecs.create(
				rynx::components::position(pos, angle),
				rynx::components::collision_category(collisionCategoryStatic),
				rynx::components::boundary({ Shape::makeRectangle(edgeLength, 5.0f).generateBoundary_Outside() }),
				rynx::components::radius(math::sqrt_approx(2 * (edgeLength * edgeLength * 0.25f))),
				rynx::components::color({ 0,1,0,1 }),
				rynx::components::motion()
			);
		};

		makeBox_outside({ -15, -50, 0 }, -0.3f, 65.f);
		makeBox_outside({ -65, -100, 0 }, -0.3f, 65.f);
		makeBox_outside({ +25, -120, 0 }, -0.3f, 65.f);
		makeBox_outside({ 0, -170, 0 }, -0.0f, 100.0f);
		makeBox_outside({ -80, -160, 0 }, -0.3f, 100.0f);
		makeBox_outside({ +80, -160, 0 }, +0.3f, 100.0f);
		*/
	}

	
	auto zoomOut = gameInput.generateAndBindGameKey('1', "zoom out");
	auto zoomIn = gameInput.generateAndBindGameKey('2', "zoom in");
	application.meshRenderer().setDepthTest(false);

	auto menuCamera = std::make_shared<Camera>();
	gameInput.generateAndBindGameKey(gameInput.getMouseKeyPhysical(0), "menuCursorActivation");

	auto cameraUp = gameInput.generateAndBindGameKey('I', "cameraUp");
	auto cameraLeft = gameInput.generateAndBindGameKey('J', "cameraLeft");
	auto cameraRight = gameInput.generateAndBindGameKey('L', "cameraRight");
	auto cameraDown = gameInput.generateAndBindGameKey('K', "cameraDown");

	rynx::menu::Div root({ 1, 1, 0 });

	// construct menus
	{
		auto sampleButton = std::make_shared<rynx::menu::Button>(application.textures(), "Frame", &root, vec3<float>(0.4f, 0.1f, 0), vec3<float>(), 0.14f);
		auto sampleButton2 = std::make_shared<rynx::menu::Button>(application.textures(), "Frame", &root, vec3<float>(0.4f, 0.1f, 0), vec3<float>(), 0.16f);
		auto sampleButton3 = std::make_shared<rynx::menu::Button>(application.textures(), "Frame", &root, vec3<float>(0.4f, 0.1f, 0), vec3<float>(), 0.18f);
		auto sampleSlider = std::make_shared<rynx::menu::SlideBarVertical>(application.textures(), "Frame", "Selection", &root, vec3<float>(0.4f, 0.1f, 0));
		auto megaSlider = std::make_shared<rynx::menu::SlideBarVertical>(application.textures(), "Frame", "Selection", &root, vec3<float>(0.4f, 0.1f, 0));

		sampleButton->text("-").font(&fontConsola);
		sampleButton->alignToInnerEdge(&root, rynx::menu::Align::BOTTOM_LEFT);
		sampleButton->color_frame(Color::RED);
		sampleButton->onClick([self = sampleButton.get()]() {});

		sampleButton2->text("-").font(&fontConsola);
		sampleButton2->alignToOuterEdge(sampleButton.get(), rynx::menu::Align::RIGHT);
		sampleButton2->alignToInnerEdge(sampleButton.get(), rynx::menu::Align::BOTTOM);
		sampleButton2->onClick([]() {});

		sampleButton3->text("-").font(&fontConsola);
		sampleButton3->alignToOuterEdge(sampleButton2.get(), rynx::menu::Align::TOP);
		sampleButton3->alignToInnerEdge(sampleButton2.get(), rynx::menu::Align::LEFT);
		sampleButton3->onClick([self = sampleButton3.get()]() {});

		sampleSlider->alignToInnerEdge(&root, rynx::menu::Align::TOP_RIGHT);
		sampleSlider->onValueChanged([](float f) {
		});

		megaSlider->alignToOuterEdge(sampleSlider.get(), rynx::menu::Align::BOTTOM);
		megaSlider->alignToInnerEdge(sampleSlider.get(), rynx::menu::Align::CENTER_HORIZONTAL);
		megaSlider->onValueChanged([](float f) {
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
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});

	while (!application.isExitRequested()) {
		rynx_profile("Main", "frame");
		application.startFrame();

		auto mousePos = application.input()->getCursorPosition();
		cameraPosition.tick(0.016f * 3);

		{
			camera->setPosition(cameraPosition);
			camera->setProjection(0.02f, 2000.0f, application.aspectRatio());

			application.meshRenderer().setCamera(camera);
			application.textRenderer().setCamera(camera);
			application.meshRenderer().cameraToGPU();
		}

		{
			constexpr float camera_translate_multiplier = 0.04f;
			constexpr float camera_zoom_multiplier = 1.05f;
			if (gameInput.isKeyDown(cameraUp)) { cameraPosition += vec3<float>{0, camera_translate_multiplier* cameraPosition->z, 0}; }
			if (gameInput.isKeyDown(cameraLeft)) { cameraPosition += vec3<float>{-camera_translate_multiplier * cameraPosition->z, 0.0f, 0}; }
			if (gameInput.isKeyDown(cameraRight)) { cameraPosition += vec3<float>{camera_translate_multiplier* cameraPosition->z, 0.0f, 0}; }
			if (gameInput.isKeyDown(cameraDown)) { cameraPosition += vec3<float>{0, -camera_translate_multiplier * cameraPosition->z, 0}; }
			if (gameInput.isKeyDown(zoomOut)) { cameraPosition *= vec3<float>(1, 1.0f, 1.0f * camera_zoom_multiplier); }
			if (gameInput.isKeyDown(zoomIn)) { cameraPosition *= vec3<float>(1, 1.0f, 1.0f / camera_zoom_multiplier); }
		}

		{
			float cameraHeight = cameraPosition->z;
			gameInput.mouseWorldPosition(
				(cameraPosition * vec3<float>{1, 1, 0}) +
				mousePos * vec3<float>(cameraHeight, cameraHeight / application.aspectRatio(), 1.0f)
			);
		}

		auto time_logic_start = std::chrono::high_resolution_clock::now();
		{
			rynx_profile("Main", "Construct frame tasks");
			base_simulation.generate_tasks();
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
		auto time_logic_end = std::chrono::high_resolution_clock::now();

		auto time_render_start = std::chrono::high_resolution_clock::now();
		{
			rynx_profile("Main", "Render");
			gameRenderer.render(ecs);
		}
		auto time_render_end = std::chrono::high_resolution_clock::now();

		{
			rynx_profile("Main", "Menus");

			// 2, 2 is the size of the entire screen (in case of 1:1 aspect ratio) for menu camera. left edge is [-1, 0], top right is [+1, +1], etc.
			// so we make it size 2,2 to cover all of that. and then take aspect ratio into account by dividing the y-size.
			root.scale_local({ 2 , 2 / application.aspectRatio(), 0 });
			menuCamera->setProjection(0.01f, 50.0f, application.aspectRatio());
			menuCamera->setPosition({ 0, 0, 1 });

			application.meshRenderer().setCamera(menuCamera);
			application.textRenderer().setCamera(menuCamera);
			application.meshRenderer().cameraToGPU();

			root.input(gameInput);
			root.tick(0.016f, application.aspectRatio());
			root.visualise(application.meshRenderer(), application.textRenderer());

			auto num_entities = ecs.size();
			auto logic_us = std::chrono::duration_cast<std::chrono::microseconds>(time_logic_end - time_logic_start).count();
			auto render_us = std::chrono::duration_cast<std::chrono::microseconds>(time_render_end - time_render_start).count();

			float info_text_pos_y = +0.1f;
			application.textRenderer().drawText(std::string("entities: ") + std::to_string(num_entities), -0.9f, 0.5f + info_text_pos_y, 0.09f, Color::DARK_GREEN, TextRenderer::Align::Left, fontConsola);
			application.textRenderer().drawText(std::string("logic:    ") + std::to_string(float(logic_us / 10) / 100.0f) + "ms", -0.9f, 0.4f + info_text_pos_y, 0.09f, Color::DARK_GREEN, TextRenderer::Align::Left, fontConsola);
			application.textRenderer().drawText(std::string("render:   ") + std::to_string(float(render_us / 10) / 100.0f) + "ms", -0.9f, 0.3f + info_text_pos_y, 0.09f, Color::DARK_GREEN, TextRenderer::Align::Left, fontConsola);
		}

		{
			rynx_profile("Main", "Clean up dead entitites");
			std::vector<rynx::ecs::id> kill_entities;
			
			ecs.query().in<dead>().execute([&](rynx::ecs::id id) {
				kill_entities.emplace_back(id);
			});
			
			ecs.for_each([&](rynx::ecs::id id, rynx::components::lifetime& time) {
				--time.value;
				if (time.value <= 0) {
					kill_entities.emplace_back(id);
				}
			});

			for (auto& id : kill_entities) {
				ecs.attachToEntity(id, dead());
			}

			ecs.query().in<dead>().execute([&ecs, &collisionDetection](rynx::ecs::id id, rynx::components::collision_category& cat) {
				collisionDetection.erase(id.value, cat.value);
			});

			for (auto& id : kill_entities) {
				ecs.erase(id);
			}
		}

		application.swapBuffers();
	}

	dead_lock_detector_keepalive = false;
	dead_lock_detector.join();

	// PROFILE_RECORD(500); // Record last 500 ms profiling to build/bin
	return 0;
}