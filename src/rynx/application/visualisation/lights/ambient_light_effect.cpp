
#include <rynx/application/visualisation/lights/ambient_light_effect.hpp>
#include <rynx/graphics/opengl.hpp>
#include <rynx/graphics/renderer/screenspace.hpp>
#include <rynx/graphics/shader/shader.hpp>
#include <rynx/graphics/shader/shaders.hpp>

#include <rynx/application/components.hpp>
#include <rynx/ecs/ecs.hpp>
#include <rynx/math/geometry/math.hpp>
#include <rynx/profiling/profiling.hpp>
#include <rynx/scheduler/context.hpp>

#include <rynx/graphics/camera/camera.hpp>
#include <rynx/system/assert.hpp>

rynx::application::visualisation::ambient_light_effect::ambient_light_effect(
    rynx::shared_ptr<rynx::graphics::shaders> shader_manager) {
  m_lights_shader = shader_manager->load_shader(
      "fbo_light_ambient", "/shaders/screenspace.vs.glsl",
      "/shaders/screenspace_lights_ambient.fs.glsl");

  m_lights_shadow_shader = shader_manager->load_shader(
      "fbo_light_ambient_shadows", "/shaders/screenspace.vs.glsl",
      "/shaders/screenspace_lights_ambient_shadows.fs.glsl");

  m_lights_shader->activate();
  m_lights_shader->uniform("tex_color", 0);
  m_lights_shader->uniform("tex_normal", 1);
  m_lights_shader->uniform("tex_position", 2);
  m_light_direction.x = 1.0;

  m_lights_shadow_shader->activate();
  m_lights_shadow_shader->uniform("tex_color", 0);
  m_lights_shadow_shader->uniform("tex_normal", 1);
  m_lights_shadow_shader->uniform("tex_position", 2);
  m_lights_shadow_shader->uniform("tex_material", 3);
}

void rynx::application::visualisation::ambient_light_effect::prepare(
    rynx::scheduler::context *) {}

void rynx::application::visualisation::ambient_light_effect::set_global_ambient(
    rynx::floats4 color) {
  m_light_colors[1] = color;
}

void rynx::application::visualisation::ambient_light_effect::
    set_global_directed(rynx::floats4 color, rynx::vec3f direction) {
  m_light_colors[0] = color;
  m_light_direction = direction;
}

void rynx::application::visualisation::ambient_light_effect::execute() {
  auto active_shader =
      m_shadows_enabled ? m_lights_shadow_shader : m_lights_shader;
  active_shader->activate();

  if (m_shadows_enabled && m_camera) {
    rynx::matrix4 view_proj = m_camera->getProjection() * m_camera->getView();
    active_shader->uniform("view_projection", view_proj);
    // active_shader->uniform("view_matrix", m_camera->getView());
  }

  rynx::graphics::gl::blend_func_cumulative();
  active_shader->uniform("lights_colors", m_light_colors, 2);
  active_shader->uniform("light_direction", m_light_direction);
  rynx::graphics::screenspace_draws::draw_fullscreen();
  rynx::graphics::gl::blend_func_default();
}
