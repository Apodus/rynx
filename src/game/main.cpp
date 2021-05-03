
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


#include <rynx/tech/collision_detection.hpp>

struct ball_spawner_ruleset : public rynx::application::logic::iruleset {
	rynx::graphics::mesh_id circle_id;
	rynx::graphics::mesh_id square_id;
	rynx::graphics::texture_id tex_id;

	rynx::collision_detection::category_id dynamic;
	uint64_t frameCount = 0;
	uint64_t how_often_to_spawn = 300;
	float x_spawn = -20.9f;

	rynx::math::rand64 m_random;

	ball_spawner_ruleset(
		rynx::graphics::mesh_id ball_id,
		rynx::graphics::mesh_id square_id,
		rynx::graphics::texture_id tex_id,
		rynx::collision_detection::category_id dynamic
	) : circle_id(ball_id), square_id(square_id), tex_id(tex_id), dynamic(dynamic) {}

	virtual void serialize(rynx::scheduler::context& ctx, rynx::serialization::vector_writer& writer) {
		logmsg("serializing ball spawner: frameCount: %llu", frameCount);
		writer(frameCount);
	}

	virtual void deserialize(rynx::scheduler::context& ctx, rynx::serialization::vector_reader& reader) {
		reader(frameCount);
		logmsg("deserializing ball spawner: frameCount: %llu", frameCount);
	}

	virtual void onFrameProcess(rynx::scheduler::context& scheduler, float /* dt */) override {
		if ((++frameCount % how_often_to_spawn) == 0) {
			scheduler.add_task("spawn balls", [this](
				rynx::ecs::edit_view<
				rynx::components::collisions,
				rynx::components::position,
				rynx::components::motion,
				rynx::components::physical_body,
				rynx::components::radius,
				rynx::components::color,
				rynx::components::dampening,
				rynx::components::boundary,
				rynx::components::phys::joint,
				rynx::components::light_omni,
				rynx::components::mesh,
				rynx::components::light_directed,
				rynx::components::texture> ecs) {

					for (int i = 0; i < 1; ++i) {
						float x = x_spawn + m_random(0.0f, 4.0f);
						float y = +100.0f + m_random(0.0f, 4.0f);
						if (true || m_random() > 0.5f) {
							auto id1 = ecs.create(
								rynx::components::position({ x, y, 0 }),
								rynx::components::motion(),
								rynx::components::physical_body()
									.mass(5.0f)
									.elasticity(0.3f)
									.friction(2.0f)
									.moment_of_inertia(10.0f * 10.0f * 5.0f * 0.5f),
								rynx::components::radius(10.0f),
								rynx::components::collisions{ dynamic.value },
								rynx::components::color({ m_random(), m_random(), m_random(), 1.0f}),
								rynx::components::dampening({ 0.10f, 0.10f }),
								rynx::components::mesh{ circle_id },
								rynx::components::texture{"hero"}
							);

							// add lights.
							if constexpr (false) {
								float v = m_random();
								if (v > 0.97f) {
									rynx::components::light_omni light;
									light.color = rynx::floats4(m_random(0.0f, 1.0f), m_random(0.0f, 1.0f), m_random(0.0f, 1.0f), m_random(30.0f, 100.0f));
									light.ambient = m_random(0.0f, 0.1f);
									ecs.attachToEntity(id1, light);
								}
								else if (v > 0.94f) {
									rynx::components::light_directed light;
									light.color = rynx::floats4(m_random(0.0f, 1.0f), m_random(0.0f, 1.0f), m_random(0.0f, 1.0f), m_random(30.0f, 100.0f));
									light.ambient = m_random(0.0f, 0.1f);
									light.direction = rynx::vec3f(1, 1, 0).normalize();
									light.edge_softness = m_random(0.5f, 5.0f);
									light.angle = m_random(1.5f, 4.14f);
									ecs.attachToEntity(id1, light);
								}
							}

							auto moment_of_inertia_for_center_aligned_box = [](float width, float height, float mass) {
								return (width * width + height * height) * mass / 12.0f;
							};

							for (int k = 1; k < 1; ++k) {
								float v = m_random();
								float edge_width = 2.0f + m_random() * 5.0f;
								float mass = edge_width * edge_width * 1.25f;

								x += 2.2f * edge_width;
								y += 4.0f;
								auto id2 = ecs.create(
									rynx::components::position({ x, y, 0 }),
									rynx::components::motion(),
									rynx::components::physical_body()
										.mass(mass)
										.elasticity(0.1f)
										.friction(1.0f)
										.moment_of_inertia(edge_width * edge_width * mass * 0.5f).bias(10.0f),
									// rynx::components::boundary(rynx::Shape::makeBox(edge_width)),
									// rynx::components::radius(sqrtf(edge_width * edge_width * 0.25f)),
									rynx::components::radius(edge_width),
									rynx::components::collisions{ dynamic.value },
									rynx::components::color({ m_random(), m_random(), m_random(), 1.0f}),
									rynx::components::dampening({ 0.10f, 0.10f }),
									rynx::components::mesh{ circle_id },
									rynx::components::texture{ "hero" }
								);

								if constexpr (false) {
									rynx::components::phys::joint joint;
									joint.connect_with_rod()
										.rotation_free();

									joint.id_a = id1;
									joint.id_b = id2;

									joint.point_a = rynx::vec3<float>(0, 0, 0);
									joint.point_b = rynx::vec3<float>(0, 0, 0);
									joint.length = 5.1f;
									joint.strength = 0.1f;
									ecs.create(joint);
									id1 = id2;
								}
							}
						}
						else {
							float edge_length = 10.5f + 15.0f * m_random();
							float mass = 5000.0f;
							auto rectangle_moment_of_inertia = [](float mass, float width, float height) { return mass / 12.0f * (height * height + width * width); };
							float radius = 0.5f * rynx::math::sqrt_approx(4.0f * sqr(0.5f * edge_length));
							ecs.create(
								rynx::components::position(rynx::vec3<float>(x, y, 0.0f), i * 2.0f),
								rynx::components::collisions{ dynamic.value },
								rynx::components::boundary(rynx::Shape::makeBox(edge_length)),
								rynx::components::radius(radius),
								rynx::components::physical_body().mass(mass).moment_of_inertia(rectangle_moment_of_inertia(mass, edge_length, edge_length)).elasticity(0.2f).friction(1.0f),
								rynx::components::color({ 1,1,1,1 }),
								rynx::components::motion({ 0, 0, 0 }, 0),
								rynx::components::mesh({ square_id }),
								rynx::components::dampening({ 0.2f, 1.f })
							);
						}
					}
				});
		}
	}
};


int main(int argc, char** argv) {

	// uses this thread services of rynx, for example in cpu performance profiling.
	rynx::this_thread::rynx_thread_raii rynx_thread_services_required_token;

	rynx::application::Application application;
	application.openWindow(1920, 1080);

	auto meshes = application.renderer().meshes();

	application.loadTextures("../textures/");
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

	// setup game logic
	{
		auto ruleset_collisionDetection = base_simulation.rule_set(program_state_game_running).create<rynx::ruleset::physics_2d>();
		auto ruleset_particle_update = base_simulation.rule_set().create<rynx::ruleset::particle_system>();
		auto ruleset_frustum_culling = base_simulation.rule_set().create<rynx::ruleset::frustum_culling>(camera);
		auto ruleset_motion_updates = base_simulation.rule_set(program_state_game_running).create<rynx::ruleset::motion_updates>(rynx::vec3<float>(0, -160.8f, 0));
		auto ruleset_physical_springs = base_simulation.rule_set(program_state_game_running).create<rynx::ruleset::physics::springs>();
		auto ruleset_lifetime_updates = base_simulation.rule_set(program_state_game_running).create<rynx::ruleset::lifetime_updates>();
		// auto ruleset_minilisk_gen = base_simulation.rule_set(program_state_game_running).create<ball_spawner_ruleset>(circle_id, square_id, application.textures()->findTextureByName("hero"), collisionCategoryDynamic);
		auto ruleset_editor = base_simulation.rule_set(program_state_editor_running).create<rynx::editor_rules>(
			*base_simulation.m_context,
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

		ruleset_physical_springs->depends_on(ruleset_motion_updates);
		ruleset_collisionDetection->depends_on(ruleset_motion_updates);
		ruleset_frustum_culling->depends_on(ruleset_motion_updates);
	}
	
	// setup simulation initial state
	{
		rynx::math::rand64 random;

		// create some boxes for testing.
		if constexpr (false) {
			for (int i = 0; i < 16; ++i) {
				rynx::polygon shape = rynx::Shape::makeBox(1.0f + 2.0f * random());
				ecs.create(
					rynx::components::position(rynx::vec3<float>(-80.0f + i * 8.0f, 0.0f, 0.0f), i * 2.0f),
					rynx::components::collisions{ collisionCategoryDynamic.value },
					rynx::components::boundary({ shape }),
					rynx::components::radius(rynx::math::sqrt_approx(16 + 16)),
					rynx::components::color({ 1,1,0,1 }),
					rynx::components::motion({ 0, 0, 0 }, 0),
					rynx::components::dampening({ 0.93f, 0.98f }),
					rynx::components::ignore_gravity()
				);
			}
		}
		
		auto makeBox_inside = [&](rynx::vec3<float> pos, float angle, float edgeLength, float angular_velocity) {
			auto mesh_name = std::to_string(pos.y * pos.x - pos.y - pos.x);
			auto polygon = rynx::Shape::makeAAOval(0.5f, 40, edgeLength, edgeLength * 0.5f);
			polygon.invert();
			// auto mesh_p = meshes->create(rynx::polygon_triangulation().make_boundary_mesh(polygon, 1.0f));
			return base_simulation.m_ecs.create(
				rynx::components::position(pos, angle),
				rynx::components::collisions{ collisionCategoryStatic.value },
				rynx::components::boundary({ polygon }),
				// rynx::components::mesh(mesh_p),
				rynx::matrix4(),
				rynx::components::radius(polygon.radius()),
				rynx::components::color({ 0.5f, 0.2f, 1.0f, 1.0f }),
				rynx::components::motion({ 0, 0, 0 }, angular_velocity),
				rynx::components::physical_body().mass(std::numeric_limits<float>::max()).elasticity(0.0f).friction(2.0f).moment_of_inertia(std::numeric_limits<float>::max()),
				rynx::components::ignore_gravity()
			);
		};

		auto makeBox_outside = [&](rynx::vec3<float> pos, float angle, float edgeLength, float angular_velocity) {
			auto mesh_name = std::to_string(pos.y * pos.x);
			auto polygon = rynx::Shape::makeRectangle(edgeLength, 5.0f);
			// auto mesh_p = meshes->create(rynx::polygon_triangulation().make_boundary_mesh(polygon, 1.0f));
			float radius = polygon.radius();
			return base_simulation.m_ecs.create(
				rynx::components::position(pos, angle),
				rynx::components::collisions{ collisionCategoryStatic.value },
				rynx::components::boundary(polygon, pos, angle),
				// rynx::components::mesh(mesh_p),
				rynx::components::radius(radius),
				rynx::components::color({ 0.2f, 1.0f, 0.3f, 1.0f }),
				rynx::components::motion({ 0, 0, 0 }, angular_velocity),
				rynx::components::physical_body().mass(std::numeric_limits<float>::max()).elasticity(0.0f).friction(2.0f).moment_of_inertia(std::numeric_limits<float>::max()).bias(100.0f),
				rynx::components::ignore_gravity(),
				rynx::components::dampening{0.1f, 0.0f}
			);
		};

		auto makeChain = [&](rynx::vec3f pos, rynx::vec3f dir, float length, int numJoints) {
			float x = pos.x;
			float y = pos.y;
			auto id1 = ecs.create(
				rynx::components::position({ x, y, 0 }),
				rynx::components::motion(),
				rynx::components::ignore_gravity(),
				rynx::components::radius(2.0f),
				rynx::components::collisions{ collisionCategoryDynamic.value },
				rynx::components::color({1.0f, 1.0f, 1.0f, 1.0f}),
				rynx::components::mesh{ circle_id },
				rynx::components::dampening({ 0.50f, 0.30f }),
				rynx::matrix4()
			);

			auto rootId = id1;

			ecs.attachToEntity(id1, rynx::components::physical_body(rootId)
				.mass(std::numeric_limits<float>::max())
				.elasticity(0.0f)
				.friction(2.0f)
				.moment_of_inertia(std::numeric_limits<float>::max()));

			auto rectangle_moment_of_inertia = [](float mass, float width, float height) { return mass * (height * height + width * width) * (1.0f / 12.0f); };

			float rope_width = 2.0f;
			// float rope_segment_length = length / numJoints;
			float rope_segment_length = 3.0f * rope_width;
			numJoints = length / (rope_segment_length - rope_width);

			auto p = rynx::Shape::makeRectangle(rope_segment_length, rope_width);
			// auto m = meshes->create(rynx::polygon_triangulation().make_boundary_mesh(p));
			float piece_radius = sqrtf((rope_width * rope_width + rope_segment_length * rope_segment_length) * 0.25f);
			float mass = 5.0f;
			
			for (int k = 1; k <= numJoints; ++k) {
				auto id2 = ecs.create(
					rynx::components::position(pos + dir * (k * rope_segment_length)),
					rynx::components::motion(),
					rynx::components::physical_body(rootId).mass(mass).moment_of_inertia(rectangle_moment_of_inertia(mass, rope_segment_length, rope_width)).friction(1.0f).elasticity(0.3f),
					rynx::components::radius(piece_radius),
					rynx::components::collisions{ collisionCategoryDynamic.value },
					rynx::components::color(),
					// rynx::components::boundary(std::vector<rynx::polygon::segment>(bound)),
					rynx::components::dampening({ 0.60f, 0.60f }),
					rynx::components::mesh{ circle_id },
					rynx::matrix4()
				);

				rynx::components::phys::joint joint;
				joint.connect_with_rod().rotation_free();

				joint.id_a = id1;
				joint.id_b = id2;

				if(id1 == rootId)
					joint.point_a = rynx::vec3<float>(0, 0, 0);
				else
					joint.point_a = rynx::vec3<float>(+rope_segment_length * 0.5f - rope_width * 1.1f, 0, 0);
				joint.point_b = rynx::vec3<float>(-rope_segment_length * 0.5f + rope_width * 1.1f, 0, 0);
				joint.length = rope_segment_length * 0.5f; // rope_segment_length;
				joint.strength = 2.0f;
				ecs.create(joint);

				id1 = id2;
			}
		};

		// makeChain({ -500, +300, 0 }, { +1.0f, 0.0f, 0.0f }, 400.0f, 25);

		// makeBox_outside({ -15, -50, 0 }, -0.3f, 265.f, +0.58f);
		// makeBox_outside({ -65, -100, 0 }, -0.3f, 65.f, -0.24f);
		// makeBox_outside({ +25, -120, 0 }, -0.3f, 65.f, -0.12f);

		/*
		makeBox_inside({ -5, -30, 0 }, +0.3f, 40.f, -0.25f);
		makeBox_inside({ -65, -100, 0 }, 0.f, 60.f, -0.30f);
		makeBox_inside({ +25, -120, 0 }, +0.5f, 80.f, +0.15f);
		*/

		/*
		makeBox_outside({ 0, -170, 0 }, -0.0f, 100.0f, 0.f);
		makeBox_outside({ -80, -160, 0 }, -0.3f, 1000.0f, 0.f);
		makeBox_outside({ +80, -160, 0 }, +0.3f, 1000.0f, 0.f);
		*/
	}

	// setup some debug controls
	float sleepTime = 0.9f;
	auto slowTime = gameInput.generateAndBindGameKey('X', "slow time");
	auto fastTime = gameInput.generateAndBindGameKey('C', "fast time");

	auto zoomOut = gameInput.generateAndBindGameKey('1', "zoom out");
	auto zoomIn = gameInput.generateAndBindGameKey('2', "zoom in");

	auto menuCamera = std::make_shared<rynx::camera>();

	gameInput.generateAndBindGameKey(gameInput.getMouseKeyPhysical(0), "menuCursorActivation");
	
	auto cameraUp = gameInput.generateAndBindGameKey('I', "cameraUp");
	auto cameraLeft = gameInput.generateAndBindGameKey('J', "cameraLeft");
	auto cameraRight = gameInput.generateAndBindGameKey('L', "cameraRight");
	auto cameraDown = gameInput.generateAndBindGameKey('K', "cameraDown");
	
	auto key_toggle_game_simulation = gameInput.generateAndBindGameKey('P', "simulation enabled");
	auto key_toggle_editor_enabled = gameInput.generateAndBindGameKey('O', "editor enabled");
	
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

	program_state_editor_running.disable();
	render.debug_draw_binary_config(program_state_editor_running);

	auto camera_orientation_key = gameInput.generateAndBindGameKey(gameInput.getMouseKeyPhysical(1), "camera_orientation");

	camera->setUpVector({ 0, 1, 0 });
	camera->setDirection({ 0, 0, -1 });
	camera->setPosition({ 0, 0, +130 });

	rynx::smooth<rynx::vec3<float>> cameraPosition(0.0f, 0.0f, 300.0f);

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
			camera->setPosition(cameraPosition);
			camera->setDirection(direction);
			camera->setUpVector({ 0.0f, 1.0f, 0.0f });
			camera->setProjection(0.02f, 2000.0f, application.aspectRatio());
			camera->rebuild_view_matrix();
		}

		if (gameInput.isKeyPressed(cameraUp)) {
			// config = audio.play_sound(soundIndex, rynx::vec3f(), rynx::vec3f());
		}

		timer.reset();

		// menu input must happen first before tick. in case menu components
		// reserve some input as private.
		menuSystem.input(gameInput);

		{
			auto scoped_inhibitor = menuSystem.inhibit_dedicated_inputs(gameInput);
		
			// TODO: Input handling should probably not be here.
			{
				const float camera_translate_multiplier = 400.4f * dt;
				const float camera_zoom_multiplier = (1.0f - dt * 3.0f);
				if (gameInput.isKeyDown(cameraUp)) { cameraPosition += camera->local_forward() * camera_translate_multiplier; }
				if (gameInput.isKeyDown(cameraLeft)) { cameraPosition += camera->local_left() * camera_translate_multiplier; }
				if (gameInput.isKeyDown(cameraRight)) { cameraPosition -= camera->local_left() * camera_translate_multiplier; }
				if (gameInput.isKeyDown(cameraDown)) { cameraPosition -= camera->local_forward() * camera_translate_multiplier; }
				// if (gameInput.isKeyDown(zoomOut)) { cameraPosition *= vec3<float>(1, 1.0f, 1.0f * camera_zoom_multiplier); }
				// if (gameInput.isKeyDown(zoomIn)) { cameraPosition *= vec3<float>(1, 1.0f, 1.0f / camera_zoom_multiplier); }
				if (gameInput.isKeyClicked(rynx::key::physical('X'))) {
					rynx::profiling::write_profile_log();
				}

				if (gameInput.isKeyClicked(key_toggle_editor_enabled)) { program_state_editor_running.toggle(); }
				if (gameInput.isKeyClicked(key_toggle_game_simulation)) { program_state_game_running.toggle(); }
			}

			{
				rynx_profile("Main", "Construct frame tasks");
				base_simulation.generate_tasks(dt);
			}

			{
				rynx_profile("Main", "Start scheduler");
				scheduler.start_frame();
			}

			application.swapBuffers();
			auto swap_time_us = timer.time_since_last_access_us();
			swap_time.observe_value(swap_time_us / 1000.0f);

			{
				rynx_profile("Main", "Wait for frame end");
				scheduler.wait_until_complete();
			}
		

			auto logic_time_us = swap_time_us + timer.time_since_last_access_us();
			logic_time.observe_value(logic_time_us / 1000.0f); // down to milliseconds.
		}
		
		// menu updates are part of logic, not visualization. must tick every frame.
		menuSystem.update(dt, application.aspectRatio());

		// should we render or not.
		if (true) {
			timer.reset();
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

					rynx::graphics::renderable_text logic_fps_text;
					logic_fps_text
						.text("logic:  " + get_min_avg_max(logic_time))
						.pos({ 0.0f, 0.28f + info_text_pos_y, 0.0f })
						.font_size(0.025f)
						.color(Color::DARK_GREEN)
						.align_left()
						.font(&fontConsola);

					rynx::graphics::renderable_text render_fps_text = logic_fps_text;
					render_fps_text.text(std::string("draw:   ") + get_min_avg_max(render_time))
						.pos({ -0.0f, 0.37f + info_text_pos_y });

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
						application.renderer().drawText(render_fps_text);
						application.renderer().drawText(body_count_text);
						application.renderer().drawText(frustum_culler_text);
						application.renderer().drawText(fps_text);
					}
				}

				scheduler.wait_until_complete();
			}

			auto render_time_us = timer.time_since_last_access_us();
			render_time.observe_value(render_time_us / 1000.0f);

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
						m.scale(1.55f);
						application.renderer().drawMesh(circle_id, rynx::graphics::texture_id(), m);
					}
				}

				timer.reset();
			}
		}

		{
			rynx_profile("Main", "Clean up dead entitites");
			dt = std::min(0.016f, std::max(0.001f, frame_timer_dt.time_since_last_access_ms() * 0.001f));
			// ddddt = 0.016f;

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

		if constexpr(false)
		{
			rynx_profile("Main", "Sleep");
			// NOTE: Frame time can be edited during runtime for debugging reasons.
			if (gameInput.isKeyDown(slowTime)) { sleepTime *= 1.1f; }
			if (gameInput.isKeyDown(fastTime)) { sleepTime *= 0.9f; }

			std::this_thread::sleep_for(std::chrono::milliseconds(int(sleepTime)));
		}
	}

	return 0;
}
