
#include <rynx/application/visualisation/default_graphics_passes.hpp>

#include <rynx/application/visualisation/geometry/ball_renderer.hpp>
#include <rynx/application/visualisation/geometry/boundary_renderer.hpp>
#include <rynx/application/visualisation/geometry/mesh_renderer.hpp>

#include <rynx/application/visualisation/lights/omnilights_effect.hpp>
#include <rynx/application/visualisation/lights/directed_lights_effect.hpp>
#include <rynx/application/visualisation/lights/ambient_light_effect.hpp>

#include <memory>

std::unique_ptr<rynx::application::graphics_step> rynx::application::visualisation::default_geometry_pass(MeshRenderer* pRenderer, Camera* pCamera) {
	auto geometry_pass = std::make_unique<graphics_step>();
	geometry_pass->add_graphics_step(std::make_unique<rynx::application::visualisation::ball_renderer>(pRenderer->meshes()->get("circle_empty"), pRenderer, pCamera));
	geometry_pass->add_graphics_step(std::make_unique<rynx::application::visualisation::boundary_renderer>(pRenderer->meshes()->get("square_tube_normals"), pRenderer));
	geometry_pass->add_graphics_step(std::make_unique<rynx::application::visualisation::mesh_renderer>(pRenderer));
	return geometry_pass;
}

std::unique_ptr<rynx::application::graphics_step> rynx::application::visualisation::default_lighting_pass(std::shared_ptr<rynx::graphics::shaders> shaders) {
	auto lighting_pass = std::make_unique<graphics_step>();
	lighting_pass->add_graphics_step(std::make_unique<rynx::application::visualisation::omnilights_effect>(shaders));
	lighting_pass->add_graphics_step(std::make_unique<rynx::application::visualisation::directed_lights_effect>(shaders));
	lighting_pass->add_graphics_step(std::make_unique<rynx::application::visualisation::ambient_light_effect>(shaders));
	return lighting_pass;
}