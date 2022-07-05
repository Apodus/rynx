
#include <rynx/application/visualisation/default_graphics_passes.hpp>

#include <rynx/application/visualisation/geometry/ball_renderer.hpp>
#include <rynx/application/visualisation/geometry/boundary_renderer.hpp>
#include <rynx/application/visualisation/geometry/mesh_renderer.hpp>
#include <rynx/application/visualisation/geometry/model_matrix_updates.hpp>

#include <rynx/application/visualisation/lights/omnilights_effect.hpp>
#include <rynx/application/visualisation/lights/directed_lights_effect.hpp>
#include <rynx/application/visualisation/lights/ambient_light_effect.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/tech/std/memory.hpp>

rynx::unique_ptr<rynx::application::graphics_step> rynx::application::visualisation::default_geometry_pass(rynx::graphics::renderer* pRenderer) {
	rynx::graphics::mesh_id tube_mesh_id = pRenderer->meshes()->create_transient(rynx::Shape::makeBox(1.0f));
	auto* tube_mesh = pRenderer->meshes()->get(tube_mesh_id);
	tube_mesh->normals.clear();
	tube_mesh->putNormal(0, +1, 0);
	tube_mesh->putNormal(0, -1, 0);
	tube_mesh->putNormal(0, -1, 0);
	tube_mesh->putNormal(0, +1, 0);
	tube_mesh->bind();
	tube_mesh->rebuildNormalBuffer();
	
	auto geometry_pass = rynx::make_unique<rynx::application::graphics_step>();
	geometry_pass->add_graphics_step(rynx::make_unique<rynx::application::visualisation::model_matrix_updates>());
	geometry_pass->add_graphics_step(rynx::make_unique<rynx::application::visualisation::mesh_renderer>(pRenderer));
	geometry_pass->add_graphics_step(rynx::make_unique<rynx::application::visualisation::boundary_renderer>(tube_mesh_id, pRenderer));
	
	rynx::graphics::mesh_id circle_mesh_id = pRenderer->meshes()->create_transient(rynx::Shape::makeCircle(0.5f, 64));
	geometry_pass->add_graphics_step(rynx::make_unique<rynx::application::visualisation::ball_renderer>(pRenderer->meshes()->get(circle_mesh_id), pRenderer));
	return geometry_pass;
}

rynx::unique_ptr<rynx::application::graphics_step> rynx::application::visualisation::default_lighting_pass(rynx::shared_ptr<rynx::graphics::shaders> shaders) {
	auto lighting_pass = rynx::make_unique<rynx::application::graphics_step>();
	lighting_pass->add_graphics_step(rynx::make_unique<rynx::application::visualisation::omnilights_effect>(shaders));
	lighting_pass->add_graphics_step(rynx::make_unique<rynx::application::visualisation::directed_lights_effect>(shaders));
	lighting_pass->add_graphics_step(rynx::make_unique<rynx::application::visualisation::ambient_light_effect>(shaders));
	return lighting_pass;
}