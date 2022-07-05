

#include <game/sample_application.hpp>

#include <rynx/graphics/texture/texturehandler.hpp>

#include <rynx/graphics/text/font.hpp>
#include <rynx/graphics/text/fontdata/consolamono.hpp>

#include <rynx/graphics/renderer/meshrenderer.hpp>

#include <rynx/tech/collision_detection.hpp>

#include <rynx/rulesets/collisions.hpp>
#include <rynx/rulesets/frustum_culling.hpp>
#include <rynx/rulesets/lifetime.hpp>
#include <rynx/rulesets/motion.hpp>
#include <rynx/rulesets/particles.hpp>
#include <rynx/rulesets/physics/springs.hpp>

#include <rynx/editor/editor.hpp>
#include <rynx/editor/tools/collisions_tool.hpp>
#include <rynx/editor/tools/instantiate_tool.hpp>
#include <rynx/editor/tools/joint_tool.hpp>
#include <rynx/editor/tools/mesh_tool.hpp>
#include <rynx/editor/tools/polygon_tool.hpp>
#include <rynx/editor/tools/selection_tool.hpp>
#include <rynx/editor/tools/texture_selection_tool.hpp>

#include <rynx/application/visualisation/debug_visualisation.hpp>

SampleApplication::SampleApplication() {
	m_reflections.load_generated_reflections();
	m_reflections.create<rynx::matrix4>().add_field("m", &rynx::matrix4::m);
}

void SampleApplication::startup_load() {
	loadTextures("../textures/");
}

void SampleApplication::set_resources() {
	auto* context = simulation_context();
	auto font = rynx::make_unique<Font>(
		Fonts::setFontConsolaMono(
			textures()->findTextureByName("consola1024")
		)
	);

	renderer().setDefaultFont(*font);

	context->set_resource(std::move(font));
	context->set_resource(rynx::make_unique<rynx::mapped_input>(input()));

	context->set_resource(m_reflections);
	context->set_resource(m_menuSystem);

	context->set_resource(renderer().meshes());
	context->set_resource(debugVis());
	context->set_resource(textures());

	context->set_resource<rynx::collision_detection>();
	auto& collisionDetection = context->get_resource<rynx::collision_detection>();

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
}

void SampleApplication::set_simulation_rules() {
	auto& simu = simulation();
	auto* context = simulation_context();
	auto program_state_game_running = context->access_state().generate_state_id();
	auto program_state_editor_running = context->access_state().generate_state_id();

	auto& vfs = context->get_resource<rynx::filesystem::vfs>();
	vfs.mount().native_directory("../scenes/", "/scenes/");
	vfs.mount().native_directory("../textures/", "/textures/");
	
	auto& scenes = context->get_resource<rynx::scenes>();
	scenes.scan_directory(vfs, "/scenes/");

	auto camera = context->get_resource_ptr<rynx::camera>();
	auto& reflections = context->get_resource<rynx::reflection::reflections>();
	auto& font = context->get_resource<Font>();
	auto* root = context->get_resource<rynx::menu::System>().root();

	// start with editor up and running, game paused.
	program_state_editor_running.enable();
	program_state_game_running.disable();

	// setup game logic
	{
		auto ruleset_collisionDetection = simu.rule_set(program_state_game_running).create<rynx::ruleset::physics_2d>();
		auto ruleset_particle_update = simu.rule_set().create<rynx::ruleset::particle_system>();
		auto ruleset_frustum_culling = simu.rule_set().create<rynx::ruleset::frustum_culling>(camera);
		auto ruleset_motion_updates = simu.rule_set(program_state_game_running).create<rynx::ruleset::motion_updates>(rynx::vec3<float>(0, -160.8f, 0));
		auto ruleset_physical_springs = simu.rule_set(program_state_game_running).create<rynx::ruleset::physics::springs>();
		auto ruleset_lifetime_updates = simu.rule_set(program_state_game_running).create<rynx::ruleset::lifetime_updates>();
		auto ruleset_editor = simu.rule_set(program_state_editor_running).create<rynx::editor_rules>(
			*context,
			program_state_game_running,
			reflections,
			&font,
			root,
			*textures()
		);

		ruleset_editor->add_tool<rynx::editor::tools::texture_selection>(*context);
		ruleset_editor->add_tool<rynx::editor::tools::polygon_tool>(*context);
		ruleset_editor->add_tool<rynx::editor::tools::collisions_tool>(*context);
		ruleset_editor->add_tool<rynx::editor::tools::instantiation_tool>(*context);
		ruleset_editor->add_tool<rynx::editor::tools::joint_tool>(*context);
		ruleset_editor->add_tool<rynx::editor::tools::mesh_tool>(*context);

		ruleset_physical_springs->depends_on(ruleset_motion_updates);
		ruleset_collisionDetection->depends_on(ruleset_motion_updates);
		ruleset_frustum_culling->depends_on(ruleset_motion_updates);
	}
}