
#include <rynx/tech/ecs.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/math/geometry/polygon_triangulation.hpp>

#include <rynx/application/application.hpp>
#include <rynx/application/visualisation/debug_visualisation.hpp>

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

#include <rynx/scheduler/task_scheduler.hpp>

#include <rynx/menu/Button.hpp>
#include <rynx/menu/Slider.hpp>
#include <rynx/menu/Div.hpp>

#include <rynx/graphics/framebuffer.hpp>
#include <rynx/graphics/renderer/screenspace.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>

#include <rynx/application/render.hpp>

#include <rynx/rulesets/frustum_culling.hpp>
#include <rynx/rulesets/motion.hpp>
#include <rynx/rulesets/physics/springs.hpp>
#include <rynx/rulesets/lifetime.hpp>

#include <rynx/math/geometry/ray.hpp>
#include <rynx/math/geometry/plane.hpp>

#include <iostream>
#include <thread>

#include <cmath>
#include <memory>

#include <rynx/audio/audio.hpp>

#include <rynx/editor/editor.hpp>
#include <rynx/editor/tools/texture_selection_tool.hpp>
#include <rynx/editor/tools/collisions_tool.hpp>
#include <rynx/editor/tools/instantiate_tool.hpp>
#include <rynx/editor/tools/joint_tool.hpp>
#include <rynx/editor/tools/polygon_tool.hpp>
#include <rynx/editor/tools/mesh_tool.hpp>

#include <rynx/tech/filesystem/filesystem.hpp>
#include <rynx/tech/collision_detection.hpp>

int main(int argc, char** argv) {

	// uses this thread services of rynx, for example in cpu performance profiling.
	rynx::this_thread::rynx_thread_raii rynx_thread_services_required_token;

	rynx::application::Application application;
	application.openWindow(1920, 1080);

	auto meshes = application.renderer().meshes();

	application.loadTextures("../textures/");
	
	{
		auto src_data = rynx::filesystem::read_file("../configs/textures.dat");
		if (!src_data.empty()) {
			rynx::serialization::vector_reader tex_configs(src_data);
			application.textures()->deserialize(tex_configs);
		}
	}

	Font fontLenka(Fonts::setFontLenka(application.textures()->findTextureByName("lenka1024")));
	Font fontConsola(Fonts::setFontConsolaMono(application.textures()->findTextureByName("consola1024")));

	application.renderer().setDefaultFont(fontConsola);

	meshes->load_all_meshes_from_disk("../meshes");
	auto circle_id = meshes->findByName("circle32");
	auto square_id = meshes->findByName("box_1x1");
	auto circle_border_id = meshes->findByName("circle_border64");
	auto box_border_id = meshes->findByName("box_boundary1x1");

	if (!circle_id) {
		circle_id = meshes->create("circle32", rynx::Shape::makeCircle(1.0f, 32));
	}
	if (!square_id) {
		square_id = meshes->create("box_1x1", rynx::Shape::makeBox(1.0f));
	}
	if (!circle_border_id) {
		circle_border_id = meshes->create("circle_border64",
			rynx::polygon_triangulation().make_boundary_mesh(
				rynx::Shape::makeSphere(0.5f, 64),
				0.01f
			)
		);
	}
	if (!box_border_id) {
		box_border_id = meshes->create("box_boundary1x1",
			rynx::polygon_triangulation().make_boundary_mesh(
				rynx::Shape::makeBox(1.0f),
				0.01f
			)
		);
	}

	meshes->save_all_meshes_to_disk("../meshes");

	rynx::scheduler::task_scheduler scheduler;
	rynx::application::simulation base_simulation(scheduler);
	rynx::ecs& ecs = base_simulation.m_ecs;

	rynx::reflection::reflections reflections(ecs.get_type_index());
	reflections.load_generated_reflections();
	{
		auto refl = reflections.create<rynx::matrix4>(); // TODO: this should not be necessary
		refl.add_field("m", &rynx::matrix4::m);

		auto tex_ref = reflections.create<rynx::graphics::texture_id>();
		tex_ref.add_field("value", &rynx::graphics::texture_id::value);
	}
	rynx_assert(reflections.find(typeid(rynx::components::color).name()) != nullptr, "yup");

	std::shared_ptr<rynx::camera> camera = std::make_shared<rynx::camera>();
	camera->setProjection(0.02f, 20000.0f, application.aspectRatio());

	rynx::mapped_input gameInput(application.input());

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
		base_simulation.set_resource(application.textures().get());
		base_simulation.set_resource(&gameInput);
		base_simulation.set_resource(camera.get());
		base_simulation.set_resource(meshes.get());
		base_simulation.set_resource(application.debugVis().get());
		base_simulation.set_resource(&reflections);
	}

	rynx::menu::System menuSystem;
	rynx::menu::Component& root = *menuSystem.root();

	auto program_state_game_running = base_simulation.m_context->access_state().generate_state_id();
	auto program_state_editor_running = base_simulation.m_context->access_state().generate_state_id();
	
	// start with editor up and running, game paused.
	program_state_editor_running.enable();
	program_state_game_running.disable();

	// setup game logic
	{
		auto ruleset_collisionDetection = base_simulation.rule_set(program_state_game_running).create<rynx::ruleset::physics_2d>();
		auto ruleset_particle_update = base_simulation.rule_set().create<rynx::ruleset::particle_system>();
		auto ruleset_frustum_culling = base_simulation.rule_set().create<rynx::ruleset::frustum_culling>(camera);
		auto ruleset_motion_updates = base_simulation.rule_set(program_state_game_running).create<rynx::ruleset::motion_updates>(rynx::vec3<float>(0, -160.8f, 0));
		auto ruleset_physical_springs = base_simulation.rule_set(program_state_game_running).create<rynx::ruleset::physics::springs>();
		auto ruleset_lifetime_updates = base_simulation.rule_set(program_state_game_running).create<rynx::ruleset::lifetime_updates>();
		auto ruleset_editor = base_simulation.rule_set(program_state_editor_running).create<rynx::editor_rules>(
			*base_simulation.m_context,
			program_state_game_running,
			reflections,
			&fontConsola,
			&root,
			*application.textures(),
			gameInput
		);

		ruleset_editor->add_tool<rynx::editor::tools::texture_selection>(*base_simulation.m_context);
		ruleset_editor->add_tool<rynx::editor::tools::polygon_tool>(*base_simulation.m_context);
		ruleset_editor->add_tool<rynx::editor::tools::collisions_tool>(*base_simulation.m_context);
		ruleset_editor->add_tool<rynx::editor::tools::instantiation_tool>(*base_simulation.m_context);
		ruleset_editor->add_tool<rynx::editor::tools::joint_tool>(*base_simulation.m_context);
		ruleset_editor->add_tool<rynx::editor::tools::mesh_tool>(*base_simulation.m_context);

		ruleset_physical_springs->depends_on(ruleset_motion_updates);
		ruleset_collisionDetection->depends_on(ruleset_motion_updates);
		ruleset_frustum_culling->depends_on(ruleset_motion_updates);
	}

	// setup some debug controls
	auto menuCamera = std::make_shared<rynx::camera>();
	gameInput.generateAndBindGameKey(gameInput.getMouseKeyPhysical(0), "menuCursorActivation");
	
	auto cameraUp = gameInput.generateAndBindGameKey('I', "cameraUp");
	auto cameraLeft = gameInput.generateAndBindGameKey('J', "cameraLeft");
	auto cameraRight = gameInput.generateAndBindGameKey('L', "cameraRight");
	auto cameraDown = gameInput.generateAndBindGameKey('K', "cameraDown");
	auto zoomIn = gameInput.generateAndBindGameKey('U', "zoomIn");
	auto zoomOut = gameInput.generateAndBindGameKey('O', "zoomOut");

	struct debug_conf {
		bool visualize_dynamic_collisions = false;
		bool visualize_static_collisions = false;
		bool visualize_projectile_collisions = false;
	};

	debug_conf conf;

	// setup sound system
	rynx::sound::audio_system audio;
	// uint32_t soundIndex = audio.load("test.ogg");
	rynx::sound::configuration config;
	audio.open_output_device(64, 256);

	auto fbo_menu = rynx::graphics::framebuffer::config()
		.set_default_resolution(1920, 1080)
		.add_rgba8_target("color")
		.construct(application.textures(), "menu");

	rynx::graphics::screenspace_draws(); // initialize gpu buffers for screenspace ops.
	rynx::application::renderer render(application, camera);

	render.debug_draw_binary_config(program_state_editor_running);

	auto camera_orientation_key = gameInput.generateAndBindGameKey(gameInput.getMouseKeyPhysical(1), "camera_orientation");

	camera->setUpVector({ 0, 1, 0 });
	camera->setDirection({ 0, 0, -1 });
	camera->setPosition({ 0, 0, +300 });

	rynx::timer dt_timer;
	
	rynx::numeric_property<float> logic_time;
	rynx::numeric_property<float> swap_time;
	rynx::numeric_property<float> total_time;


	float dt = 1.0f / 120.0f;
	
	while (!application.isExitRequested()) {
		
		dt = std::min(0.016f, std::max(0.001f, dt_timer.time_since_last_access_ms() * 0.001f));

		rynx_profile("Main", "frame");
		{
			rynx_profile("Main", "start frame");
			application.startFrame();
		}
		
		auto mousePos = application.input()->getCursorPosition();

		{
			rynx_profile("Main", "update camera");

			static rynx::vec3f camera_direction(0, 0, 0);
			
			if (gameInput.isKeyDown(camera_orientation_key)) {
				auto mouseDelta = gameInput.mouseDelta();
				camera_direction += mouseDelta;
			}

			rynx::matrix4 rotator_x;
			rynx::matrix4 rotator_y;
			rotator_x.discardSetRotation(camera_direction.x, 0, 1, 0);
			rotator_y.discardSetRotation(camera_direction.y, -1, 0, 0);

			rynx::vec3f direction = rotator_y * rotator_x * rynx::vec3f(0, 0, -1);

			camera->tick(3.0f * dt);
			camera->setDirection(direction);
			camera->setUpVector({ 0.0f, 1.0f, 0.0f });
			camera->setProjection(0.02f, 2000.0f, application.aspectRatio());
			camera->rebuild_view_matrix();
		}

		if (gameInput.isKeyPressed(cameraUp)) {
			// config = audio.play_sound(soundIndex, rynx::vec3f(), rynx::vec3f());
		}

		// menu input must happen first before tick. in case menu components
		// reserve some input as private.
		menuSystem.input(gameInput);

		{
			auto scoped_inhibitor = menuSystem.inhibit_dedicated_inputs(gameInput);
		
			// TODO: Input handling should probably not be here.
			{
				const float camera_translate_multiplier = 400.4f * dt;
				const float camera_zoom_multiplier = (1.0f - dt * 3.0f);
				if (gameInput.isKeyDown(cameraUp)) { camera->translate(camera->local_up() * camera_translate_multiplier); }
				if (gameInput.isKeyDown(cameraLeft)) { camera->translate(camera->local_left() * camera_translate_multiplier); }
				if (gameInput.isKeyDown(cameraRight)) { camera->translate(-camera->local_left() * camera_translate_multiplier); }
				if (gameInput.isKeyDown(cameraDown)) { camera->translate(-camera->local_up() * camera_translate_multiplier); }
				if (gameInput.isKeyDown(zoomOut)) {
					camera->translate(
						camera->position() *
						rynx::vec3f{1, 1, 1.0f * camera_zoom_multiplier }
					);
				}
				if (gameInput.isKeyDown(zoomIn)) {
					camera->translate(
						camera->position() *
						rynx::vec3f{ 1, 1, 1.0f / camera_zoom_multiplier }
					);
				}
				if (gameInput.isKeyClicked(rynx::key::physical('X'))) {
					rynx::profiling::write_profile_log();
				}
			}

			{
				rynx_profile("Main", "Construct frame tasks");
				base_simulation.generate_tasks(dt);
			}

			{
				rynx_profile("Main", "Start scheduler");
				scheduler.start_frame();
			}

			{
				rynx::timer swap_buffers_timer;
				application.swapBuffers();
				auto swap_time_us = swap_buffers_timer.time_since_last_access_us();
				swap_time.observe_value(swap_time_us / 1000.0f);
			}

			{
				rynx::timer logic_timer;
				rynx_profile("Main", "Wait for frame end");
				scheduler.wait_until_complete();

				auto logic_time_us = logic_timer.time_since_last_access_us();
				logic_time.observe_value(swap_time.avg() + logic_time_us / 1000.0f); // down to milliseconds.
			}
		}
		
		// menu updates are part of logic, not visualization. must tick every frame.
		menuSystem.update(dt, application.aspectRatio());

		// should we render or not.
		if (true) {
			rynx::timer render_calls_timer;
			rynx_profile("Main", "graphics");

			{
				rynx_profile("Main", "prepare");
				render.light_global_ambient({ 1.0f, 1.0f, 1.0f, 1.0f });
				render.light_global_directed({ 1.0f, 1.0f, 1.0f, 1.0f }, {1.0f, 0.0f, 0.0f});
				render.prepare(base_simulation.m_context);
				scheduler.start_frame();

				// while waiting for computing to be completed, draw menus.
				{
					rynx_profile("Main", "Menus");
					fbo_menu->bind_as_output();
					fbo_menu->clear();

					application.renderer().setDepthTest(false);

					// 2, 2 is the size of the entire screen (in case of 1:1 aspect ratio) for menu camera. left edge is [-1, 0], top right is [+1, +1], etc.
					// so we make it size 2,2 to cover all of that. and then take aspect ratio into account by dividing the y-size.
					root.scale_local({ 2 , 2 / application.aspectRatio(), 0 });
					menuCamera->setProjection(0.01f, 50.0f, application.aspectRatio());
					menuCamera->setPosition({ 0, 0, 1 });
					menuCamera->setDirection({ 0, 0, -1 });
					menuCamera->setUpVector({ 0, 1, 0 });
					menuCamera->tick(1.0f);
					menuCamera->rebuild_view_matrix();

					application.renderer().setCamera(menuCamera);
					application.renderer().cameraToGPU();
					root.visualise(application.renderer());

					auto num_entities = ecs.size();
					float info_text_pos_y = +0.1f;
					auto get_min_avg_max = [](rynx::numeric_property<float>& prop) {
						return std::to_string(prop.min()) + "/" + std::to_string(prop.avg()) + "/" + std::to_string(prop.max()) + "ms";
					};

					// TODO: Move these somewhere else.
					rynx::graphics::renderable_text logic_fps_text;
					logic_fps_text
						.text("logic:  " + get_min_avg_max(logic_time))
						.pos({ 0.0f, 0.28f + info_text_pos_y, 0.0f })
						.font_size(0.025f)
						.color(Color::DARK_GREEN)
						.align_left()
						.font(&fontConsola);


					rynx::graphics::renderable_text body_count_text = logic_fps_text;
					body_count_text.text(std::string("bodies: ") + std::to_string(ecs.query().in<rynx::components::physical_body>().count()))
						.pos({ -0.0f, 0.34f + info_text_pos_y });

					rynx::graphics::renderable_text frustum_culler_text = logic_fps_text;
					frustum_culler_text.text(std::string("visible/culled: ") + std::to_string(ecs.query().notIn<rynx::components::frustum_culled>().count()) + "/" + std::to_string(ecs.query().in<rynx::components::frustum_culled>().count()))
						.pos({ -0.0f, 0.31f + info_text_pos_y });

					rynx::graphics::renderable_text fps_text = logic_fps_text;
					fps_text.text(std::string("fps: ") + std::to_string(1000.0f / total_time.avg()))
						.pos({ -0.0f, 0.40f + info_text_pos_y });

					if (false) {
						application.renderer().drawText(logic_fps_text);
						application.renderer().drawText(body_count_text);
						application.renderer().drawText(frustum_culler_text);
						application.renderer().drawText(fps_text);
					}
				}

				scheduler.wait_until_complete();
			}

			
			{
				rynx_profile("Main", "draw");
				render.execute();

				// TODO: debug visualisations should be drawn on their own fbo.
				application.debugVis()->prepare(base_simulation.m_context);
				{
					// visualize collision detection structure.
					if (conf.visualize_dynamic_collisions) {
						std::array<rynx::vec4<float>, 5> node_colors{ rynx::vec4<float>{0, 1, 0, 0.2f}, {0, 0, 1, 0.2f}, {1, 0, 0, 0.2f}, {1, 1, 0, 0.2f}, {0, 1, 1, 0.2f} };
						collisionDetection.get(collisionCategoryDynamic)->forEachNode([&](rynx::vec3<float> pos, float radius, int depth) {
							rynx::matrix4 m;
							m.discardSetTranslate(pos);
							m.scale(radius);
							float sign[2] = { -1.0f, +1.0f };
							application.debugVis()->addDebugCircle(m, node_colors[depth % node_colors.size()]);
						});
					}

					// visualize collision detection structure.
					if (conf.visualize_static_collisions) {
						std::array<rynx::vec4<float>, 5> node_colors{ rynx::vec4<float>{0, 1, 0, 0.2f}, {0, 0, 1, 0.2f}, {1, 0, 0, 0.2f}, {1, 1, 0, 0.2f}, {0, 1, 1, 0.2f} };
						collisionDetection.get(collisionCategoryStatic)->forEachNode([&](rynx::vec3<float> pos, float radius, int depth) {
							rynx::matrix4 m;
							m.discardSetTranslate(pos);
							m.scale(radius);
							float sign[2] = { -1.0f, +1.0f };
							application.debugVis()->addDebugCircle(m, node_colors[depth % node_colors.size()]);
						});
					}
				}
				
				{
					application.debugVis()->execute();
				}

				{
					application.shaders()->activate_shader("fbo_color_to_bb");
					fbo_menu->bind_as_input();
					rynx::graphics::screenspace_draws::draw_fullscreen();
				}

				// find mouse pos in xy-plane and draw a circle there.
				{
					auto ray = camera->ray_cast(mousePos.x, mousePos.y);
					
					rynx::plane myplane;
					myplane.set_coefficients(0, 0, 1, 0);

					auto collision = ray.intersect(myplane);
					if (collision.second) {
						rynx::matrix4 m;
						m.discardSetTranslate(collision.first);
						m.scale(camera->position().z * 0.002f);
						application.renderer().drawMesh(circle_id, rynx::graphics::texture_id(), m);
					}
				}
			}
		}

		{
			rynx_profile("Main", "Clean up dead entitites");
			
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

			auto ids_dead = ecs.query().in<rynx::components::dead>().ids();
			
			for (auto id : ids_dead) {
				if (ecs[id].has<rynx::components::collisions>()) {
					auto collisions = ecs[id].get<rynx::components::collisions>();
					collisionDetection.erase(ecs, id.value, collisions.category);
				}
			}
			
			base_simulation.m_logic.entities_erased(*base_simulation.m_context, ids_dead);
			ecs.erase(ids_dead);
		}

		// lets avoid using all computing power for now.
		// TODO: Proper time tracking for frames.
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return 0;
}
