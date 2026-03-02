
#pragma once

#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/math/vector.hpp>

namespace rynx {
class camera;
namespace graphics {
class shader;
class shaders;
class screenspace_draws;
} // namespace graphics

namespace application {
namespace visualisation {
struct omnilights_effect
    : public rynx::application::graphics_step::igraphics_step {
public:
  omnilights_effect(rynx::shared_ptr<rynx::graphics::shaders> shader_manager);

  virtual ~omnilights_effect() {}
  virtual void prepare(rynx::scheduler::context *ctx) override;
  virtual void execute() override;

  void set_camera(rynx::observer_ptr<rynx::camera> cam) { m_camera = cam; }
  void enable_shadows(bool enable) { m_shadows_enabled = enable; }
  void set_shadow_density(float density) { m_shadow_density = density; }

private:
  std::vector<floats4> m_light_colors;
  std::vector<floats4> m_light_settings;

public:
  std::vector<vec3f> m_light_positions;

private:
  rynx::shared_ptr<rynx::graphics::shader> m_lights_shader;
  rynx::shared_ptr<rynx::graphics::shader> m_lights_shadow_shader;

  rynx::observer_ptr<rynx::camera> m_camera;
  bool m_shadows_enabled = true;
  float m_shadow_density = 30.0f;
};
} // namespace visualisation
} // namespace application
} // namespace rynx
