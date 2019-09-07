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

#include <rynx/application/visualisation/ball_renderer.hpp>
#include <rynx/application/visualisation/boundary_renderer.hpp>
#include <rynx/application/visualisation/mesh_renderer.hpp>

#include <rynx/input/mapped_input.hpp>
#include <rynx/tech/smooth_value.hpp>
 
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


int main(int argc, char** argv) {

	Font fontLenka(Fonts::setFontLenka());
	Font fontConsola(Fonts::setFontConsolaMono());

	rynx::application::Application application;
	application.openWindow();
	application.loadTextures("../textures/textures.txt");

	// Meshes should be stored somewhere? Constructing them should be a bit more pretty.
	auto mesh = PolygonTesselator<vec3<float>>().tesselate(Shape::makeCircle(1.0f, 32), application.textures().textureLimits("Hero", { +0.5f, -0.5f, 0.5f, 0.5f }));
	mesh->build();

	auto emptyMesh = PolygonTesselator<vec3<float>>().tesselate(Shape::makeCircle(1.0f, 32), application.textures().textureLimits("Empty", { +0.5f, -0.5f, 0.5f, 0.5f }));
	emptyMesh->build();

	auto emptySquare = PolygonTesselator<vec3<float>>().tesselate(Shape::makeBox(1.0f), application.textures().textureLimits("Empty", { +0.5f, -0.5f, 0.5f, 0.5f }));
	emptySquare->build();

	rynx::scheduler::task_scheduler scheduler;
	rynx::application::simulation base_simulation(scheduler);

	std::shared_ptr<Camera> camera = std::make_shared<Camera>();
	camera->setProjection(0.02f, 200.0f, application.aspectRatio());

	rynx::input::mapped_input gameInput(application.input());


	rynx::collision_detection* detection = new rynx::collision_detection();
	auto& collisionDetection = *detection;

	// setup collision detection
	rynx::collision_detection::category_id collisionCategoryDynamic = collisionDetection.addCategory();
	rynx::collision_detection::category_id collisionCategoryStatic = collisionDetection.addCategory();
	rynx::collision_detection::category_id collisionCategoryProjectiles = collisionDetection.addCategory();

	{
		// TODO: Do not update static category sphere tree every frame.
		collisionDetection.enableCollisionsBetweenCategories(collisionCategoryDynamic, collisionCategoryDynamic); // enable dynamic <-> dynamic collisions
		collisionDetection.enableCollisionsBetweenCategories(collisionCategoryDynamic, collisionCategoryStatic); // enable dynamic <-> static collisions

		collisionDetection.enableCollisionsBetweenCategories(collisionCategoryProjectiles, collisionCategoryStatic); // projectile <-> static
		collisionDetection.enableCollisionsBetweenCategories(collisionCategoryProjectiles, collisionCategoryDynamic); // projectile <-> dynamic
	}

	rynx::application::renderer gameRenderer;

	// set additional resources your simulation wants to use.
	{
		base_simulation.set_resource(&collisionDetection);
	}

	// setup game logic
	{
		auto ruleset_collisionDetection = std::make_unique<rynx::ruleset::collisions>();
		auto ruleset_shooting = std::make_unique<game::logic::shooting_logic>(gameInput, collisionCategoryProjectiles);
		auto ruleset_keyboardMovement = std::make_unique<game::logic::keyboard_movement_logic>(gameInput);

		auto ruleset_bullet_hits = std::make_unique<game::logic::bullet_hitting_logic>(collisionDetection, collisionCategoryDynamic, collisionCategoryStatic, collisionCategoryProjectiles);
		auto ruleset_minilisk = std::make_unique<game::logic::minilisk_logic>();
		auto ruleset_minilisk_gen = std::make_unique<game::logic::minilisk_test_spawner_logic>(collisionCategoryDynamic);

		ruleset_bullet_hits->dependsOn(*ruleset_collisionDetection);

		base_simulation.add_rule_set(std::move(ruleset_collisionDetection));
		base_simulation.add_rule_set(std::move(ruleset_shooting));
		base_simulation.add_rule_set(std::move(ruleset_keyboardMovement));

		base_simulation.add_rule_set(std::move(ruleset_bullet_hits));
		base_simulation.add_rule_set(std::move(ruleset_minilisk));
		base_simulation.add_rule_set(std::move(ruleset_minilisk_gen));
	}

	gameRenderer.addRenderer(std::make_unique<rynx::application::visualisation::ball_renderer>(emptyMesh.get(), &application.meshRenderer()));
	gameRenderer.addRenderer(std::make_unique<rynx::application::visualisation::boundary_renderer>(&application.meshRenderer()));
	gameRenderer.addRenderer(std::make_unique<rynx::application::visualisation::mesh_renderer>(&application.meshRenderer()));
	gameRenderer.addRenderer(std::make_unique<game::hitpoint_bar_renderer>(&application.meshRenderer()));
	gameRenderer.addRenderer(std::make_unique<game::visual::bullet_renderer>(&application.meshRenderer()));

	rynx::smooth<float> cameraHeight(30);

	rynx::ecs& ecs = base_simulation.m_ecs;

	// setup simulation initial state
	{
		ecs.create(
			game::hero(),
			game::health(),
			rynx::components::position(),
			rynx::components::motion(),
			rynx::components::mass({ 1.0f }),
			rynx::components::radius(0.2f),
			rynx::components::collision_category(collisionCategoryDynamic),
			rynx::components::color(),
			rynx::components::dampening({ 0.86f, 0.86f }),
			game::components::weapon(),
			rynx::components::frame_collisions()
		);

		// TODO: radius calculation from boundary (bounding radius or something)
		auto makeBox = [&](vec3<float> pos, float angle, float edgeLength) {
			base_simulation.m_ecs.create(
				rynx::components::position(pos, angle),
				rynx::components::collision_category(collisionCategoryStatic),
				rynx::components::boundary({ Shape::makeBox(edgeLength).generateBoundary() }),
				rynx::components::radius(math::sqrt_approx(2 * (edgeLength * edgeLength * 0.25f))),
				rynx::components::color({ 0,1,0,1 })
			);
		};

		for (int i = 0; i < 20; ++i)
			makeBox({ +10, +15 - i * 2.0f, 0 }, 0.0f + i * 0.1f, 2.0f);
	}

	// setup some debug controls
	float sleepTime = 10.0f;
	auto slowTime = gameInput.generateAndBindGameKey('X', "slow time");
	auto fastTime = gameInput.generateAndBindGameKey('C', "fast time");

	auto zoomOut = gameInput.generateAndBindGameKey('1', "zoom out");
	auto zoomIn = gameInput.generateAndBindGameKey('2', "zoom in");
	

	application.meshRenderer().setDepthTest(false);

	auto menuCamera = std::make_shared<Camera>();

	gameInput.generateAndBindGameKey(gameInput.getMouseKeyPhysical(0), "menuCursorActivation");

	rynx::menu::Div root({ 1, 1, 0 });
	
	// construct menus
	{
		auto sampleButton = std::make_shared<rynx::menu::Button>(application.textures(), "Empty", &root, vec3<float>(0.4f, 0.1f, 0));
		auto sampleButton2 = std::make_shared<rynx::menu::Button>(application.textures(), "Empty", &root, vec3<float>(0.4f, 0.1f, 0));
		auto sampleButton3 = std::make_shared<rynx::menu::Button>(application.textures(), "Empty", &root, vec3<float>(0.4f, 0.1f, 0));
		auto sampleSlider = std::make_shared<rynx::menu::SlideBarVertical>(application.textures(), "Empty", "Empty", &root, vec3<float>(0.4f, 0.1f, 0));

		sampleButton->text("Button 1").font(&fontConsola);
		sampleButton->alignToInnerEdge(&root, rynx::menu::Align::BOTTOM_LEFT);

		sampleButton2->text("Button 2").font(&fontConsola);
		sampleButton2->alignToOuterEdge(sampleButton.get(), rynx::menu::Align::RIGHT);
		sampleButton2->alignToInnerEdge(sampleButton.get(), rynx::menu::Align::BOTTOM);

		sampleButton3->text("Button 3").font(&fontConsola);
		sampleButton3->alignToOuterEdge(sampleButton2.get(), rynx::menu::Align::TOP);
		sampleButton3->alignToInnerEdge(sampleButton2.get(), rynx::menu::Align::LEFT);
		sampleButton3->onClick([sampleButton3, &root]() {
			sampleButton3->alignToInnerEdge(&root, rynx::menu::Align::TOP | rynx::menu::Align::RIGHT);
			});

		sampleSlider->alignToInnerEdge(&root, rynx::menu::Align::TOP_RIGHT);

		//sampleButton->m_positionAlign = rynx::menu::Component::PositionAlign::RIGHT | rynx::menu::Component::PositionAlign::BOTTOM;
		root.addChild(sampleButton);
		root.addChild(sampleButton2);
		root.addChild(sampleButton3);
		root.addChild(sampleSlider);
	}

	std::atomic<size_t> tickCounter = 0;

	std::atomic<bool> dead_lock_detector_keepalive = true;
	std::thread dead_lock_detector([&]() {
		size_t prev_tick = 994839589;
		while (dead_lock_detector_keepalive) {
			if (prev_tick == tickCounter && dead_lock_detector_keepalive) {
				scheduler.dump();
				// PROFILE_RECORD(500); // Record last 500 ms profiling to build/bin
				return;
			}
			prev_tick = tickCounter;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});

	while (!application.isExitRequested()) {
		rynx_profile(Game, "Main");
		application.startFrame();

		auto mousePos = application.input()->getCursorPosition();
		
		cameraHeight.tick(0.016f * 3);

		{
			camera->setPosition(vec3<float>(0, 0, cameraHeight));
			camera->setProjection(0.02f, 200.0f, application.aspectRatio());

			application.meshRenderer().setCamera(camera);
			application.textRenderer().setCamera(camera);
			application.meshRenderer().cameraToGPU();
		}

		gameInput.mouseWorldPosition(mousePos * vec3<float>(cameraHeight, cameraHeight / application.aspectRatio(), 1.0f));
		auto mpos = gameInput.mouseWorldPosition();

		{
			matrix4 m;
			m.discardSetTranslate(mpos);
			m.scale(0.5f);
			application.meshRenderer().drawMesh(*emptyMesh, m, "Empty");
		}

		{
			rynx_profile(Game, "Input handling");
			// TODO: Simulation API
			std::vector<std::unique_ptr<rynx::application::logic::iaction>> userActions = base_simulation.m_logic.onInput(gameInput, ecs);
			for (auto&& action : userActions) {
				action->apply(ecs);
			}
		}

		{
			rynx_profile(Game, "Construct frame tasks");
			base_simulation.generate_tasks();
		}

		{
			rynx_profile(Game, "Start scheduler");
			scheduler.start_frame();
		}

		{
			rynx_profile(Game, "Wait for frame end");
			scheduler.wait_until_complete();
			++tickCounter;
		}

		{
			rynx_profile(Game, "Render");
			gameRenderer.render(ecs);
		}

		// visualize collision detection structure.
		if constexpr (false) {
			collisionDetection.get(collisionCategoryDynamic)->forEachNode([&](vec3<float> pos, float radius, int depth) {
				matrix4 m;
				m.discardSetTranslate(pos);
				m.scale(radius);
				float sign[2] = { -1.0f, +1.0f };
				application.meshRenderer().drawMesh(*emptyMesh, m, "Empty",
					{
						0.5f - 0.2f * (1 + (depth >> 1)) * sign[depth & 1],
						0.5f + 0.2f * (1 + (depth >> 1)) * sign[depth & 1],
						1,
						0.1f
					}
				);
			});
		}

		{
			rynx_profile(Game, "Menus");
			
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
		}

		{
			rynx_profile(Game, "Clean up dead entitites");
			ecs.for_each([&ecs](rynx::ecs::id id, rynx::components::lifetime& time) {
				--time.value;
				if (time.value <= 0) {
					ecs.attachToEntity(id, game::dead());
				}
			});

			ecs.for_each([&ecs, &collisionDetection](rynx::ecs::id id, game::dead&, rynx::components::collision_category& cat) {
				collisionDetection.erase(id.value, cat.value);
				ecs.erase(id);
			});
		}

		{
			rynx_profile(Game, "Sleep");
			// NOTE: Frame time can be edited during runtime for debugging reasons.
			if (gameInput.isKeyDown(slowTime)) { sleepTime *= 1.1f; }
			if (gameInput.isKeyDown(fastTime)) { sleepTime *= 0.9f; }
			
			if (gameInput.isKeyDown(zoomOut)) { cameraHeight *= 1.1f; }
			if (gameInput.isKeyDown(zoomIn)) { cameraHeight *= 0.9f; }
			
			std::this_thread::sleep_for(std::chrono::milliseconds(int(sleepTime)));
		}

		application.swapBuffers();
	}

	dead_lock_detector_keepalive = false;
	dead_lock_detector.join();

	// PROFILE_RECORD(500); // Record last 500 ms profiling to build/bin
	return 0;
}
