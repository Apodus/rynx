
#pragma once

#include <rynx/application/visualisation/renderer.hpp>
#include <rynx/graphics/camera/camera.hpp>
#include <rynx/math/vector.hpp>

namespace rynx {
namespace graphics {
class shader;
class shaders;
class screenspace_draws;
} // namespace graphics

namespace application {
namespace visualisation {
struct directed_lights_effect
    : public rynx::application::graphics_step::igraphics_step {
public:
  directed_lights_effect(
      rynx::shared_ptr<rynx::graphics::shaders> shader_manager);

  virtual ~directed_lights_effect() {}
  virtual void prepare(rynx::scheduler::context *ctx) override;
  virtual void execute() override;

  void set_camera(rynx::observer_ptr<rynx::camera> cam) { m_camera = cam; }
  void enable_shadows(bool enable) { m_shadows_enabled = enable; }
  void set_shadow_density(float density) { m_shadow_density = density; }

private:
  std::vector<floats4> m_light_colors;
  std::vector<floats4> m_light_directions;
  std::vector<floats4>
      m_light_settings; // x=edge softness[0...inf], y=linear
                        // attenuation[0..inf], z=quadratic attenuation[0..inf],
                        // a=backside lighting (penetrating) [0..1]
  std::vector<vec3f> m_light_positions;

  rynx::shared_ptr<rynx::graphics::shader> m_lights_shader;
  rynx::shared_ptr<rynx::graphics::shader> m_lights_shadow_shader;

  rynx::observer_ptr<rynx::camera> m_camera;
  bool m_shadows_enabled = true;
  float m_shadow_density = 30.0f;
};
} // namespace visualisation
} // namespace application
} // namespace rynx
