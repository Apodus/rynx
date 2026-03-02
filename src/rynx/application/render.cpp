
#include <rynx/application/application.hpp>
#include <rynx/application/render.hpp>
#include <rynx/graphics/framebuffer.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/graphics/renderer/screenspace.hpp>
#include <rynx/graphics/shader/shaders.hpp>

#include <rynx/application/visualisation/lights/ambient_light_effect.hpp>
#include <rynx/application/visualisation/lights/directed_lights_effect.hpp>
#include <rynx/application/visualisation/lights/omnilights_effect.hpp>

#include <rynx/application/visualisation/geometry/ball_renderer.hpp>
#include <rynx/application/visualisation/geometry/boundary_renderer.hpp>
#include <rynx/application/visualisation/geometry/mesh_renderer.hpp>
#include <rynx/application/visualisation/geometry/model_matrix_updates.hpp>

#include <rynx/graphics/opengl.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/graphics/renderer/textrenderer.hpp>

#include <rynx/math/geometry/ray.hpp>

rynx::application::renderer::renderer(
    rynx::shared_ptr<rynx::graphics::GPUTextures> textures,
    rynx::shared_ptr<rynx::graphics::shaders> shaders,
    rynx::graphics::renderer& renderer, rynx::observer_ptr<rynx::camera> camera)
    : gpu_textures(textures), m_rynx_renderer(renderer), camera(camera) {
    rynx::graphics::screenspace_draws(); // initialize gpu buffers for screenspace
    // ops.

    shader_copy_color =
        shaders->load_shader("fbo_color_to_bb", "/shaders/screenspace.vs.glsl",
            "/shaders/screenspace_color_passthrough.fs.glsl");

    shader_copy_color->activate();
    shader_copy_color->uniform("tex_color", 0);

    shader_blur_horizontal = shaders->load_shader(
        "screenspace_blur_horizontal", "/shaders/screenspace.vs.glsl",
        "/shaders/screenspace_blur_horizontal.fs.glsl");
    shader_blur_vertical = shaders->load_shader(
        "screenspace_blur_vertical", "/shaders/screenspace.vs.glsl",
        "/shaders/screenspace_blur_vertical.fs.glsl");
    shader_composite = shaders->load_shader(
        "screenspace_composite", "/shaders/screenspace.vs.glsl",
        "/shaders/screenspace_composite.fs.glsl");

    shader_blur_horizontal->activate();
    shader_blur_horizontal->uniform("tex_color", 0);

    shader_blur_vertical->activate();
    shader_blur_vertical->uniform("tex_color", 0);

    shader_composite->activate();
    shader_composite->uniform("tex_color", 0);
    shader_composite->uniform("tex_lightmap", 1);

    auto omnilights_handler =
        rynx::make_unique<rynx::application::visualisation::omnilights_effect>(
            shaders);
    auto directed_lights_handler = rynx::make_unique<
        rynx::application::visualisation::directed_lights_effect>(shaders);
    auto ambient_lights_handler =
        rynx::make_unique<rynx::application::visualisation::ambient_light_effect>(
            shaders);

    m_omnilights = omnilights_handler.get();
    m_directedlights = directed_lights_handler.get();
    m_ambients = ambient_lights_handler.get();

    m_omnilights->set_camera(camera);
    m_directedlights->set_camera(camera);
    m_ambients->set_camera(camera);

    {
        lighting_pass = rynx::make_unique<graphics_step>();
        lighting_pass->add_graphics_step(std::move(omnilights_handler));
        lighting_pass->add_graphics_step(std::move(directed_lights_handler));
        lighting_pass->add_graphics_step(std::move(ambient_lights_handler));
    }

    {
        m_debug_draw_config = rynx::make_shared<rynx::binary_config::id>();

        rynx::graphics::mesh_id tube_mesh_id =
            renderer.meshes()->create_transient(rynx::Shape::makeBox(1.0f));
        auto* tube_mesh = renderer.meshes()->get(tube_mesh_id);
        tube_mesh->scale(sqrt(2.0f));
        tube_mesh->normals.clear();
        tube_mesh->putNormal(0, +1, 0);
        tube_mesh->putNormal(0, -1, 0);
        tube_mesh->putNormal(0, -1, 0);
        tube_mesh->putNormal(0, +1, 0);
        tube_mesh->bind();
        tube_mesh->rebuildNormalBuffer();

        auto boundary_rendering =
            rynx::make_unique<rynx::application::visualisation::boundary_renderer>(
                tube_mesh_id, &renderer);
        boundary_rendering->m_enabled = m_debug_draw_config;

        geometry_pass = rynx::make_unique<graphics_step>();
        geometry_pass->add_graphics_step(
            rynx::make_unique<
            rynx::application::visualisation::model_matrix_updates>());
        geometry_pass->add_graphics_step(
            rynx::make_unique<rynx::application::visualisation::mesh_renderer>(
                &renderer));
        geometry_pass->add_graphics_step(std::move(boundary_rendering));

        rynx::graphics::mesh_id circle_mesh_id =
            renderer.meshes()->create_transient(rynx::Shape::makeCircle(0.5f, 64));

        auto ball_rendering =
            rynx::make_unique<rynx::application::visualisation::ball_renderer>(
                renderer.meshes()->get(circle_mesh_id), &renderer);
        ball_rendering->m_enabled = m_debug_draw_config;
        geometry_pass->add_graphics_step(std::move(ball_rendering));
    }

    current_internal_resolution_geometry = { 1.0f, 1.0f };
    current_internal_resolution_lighting = { 1.0f, 1.0f };
}

void rynx::application::renderer::set_geometry_resolution(float percent_x,
    float percent_y) {
    current_internal_resolution_geometry = { percent_x, percent_y };
    fbo_world_geometry.reset();
    fbo_world_geometry =
        rynx::graphics::framebuffer::config()
        .set_default_resolution(
            static_cast<int>(current_resolution.first *
                current_internal_resolution_geometry.first),
            static_cast<int>(current_resolution.second *
                current_internal_resolution_geometry.second))
        .add_rgba8_target("color")
        .add_rgba8_target("normal")
        .add_float_target("position")
        .add_rgba8_target("material")
        .add_depth32_target()
        .construct(gpu_textures, "world_geometry");
}

void rynx::application::renderer::set_lights_resolution(float percent_x,
    float percent_y) {
    current_internal_resolution_lighting = { percent_x, percent_y };
    fbo_lights.reset();
    fbo_lights =
        rynx::graphics::framebuffer::config()
        .set_default_resolution(
            static_cast<int>(current_resolution.first *
                current_internal_resolution_lighting.first),
            static_cast<int>(current_resolution.second *
                current_internal_resolution_lighting.second))
        .add_float_target("color")
        .construct(gpu_textures, "world_colorized");

    fbo_lights_blur.reset();
    fbo_lights_blur =
        rynx::graphics::framebuffer::config()
        .set_default_resolution(
            static_cast<int>(current_resolution.first *
                current_internal_resolution_lighting.first),
            static_cast<int>(current_resolution.second *
                current_internal_resolution_lighting.second))
        .add_float_target("color")
        .construct(gpu_textures, "world_colorized_blur");

    if (shader_blur_horizontal) {
        shader_blur_horizontal->activate();
        float res_x =
            current_resolution.first * current_internal_resolution_lighting.first;
        if (res_x > 0)
            shader_blur_horizontal->uniform("blurSize", 1.5f / res_x);
    }
    if (shader_blur_vertical) {
        shader_blur_vertical->activate();
        float res_y =
            current_resolution.second * current_internal_resolution_lighting.second;
        if (res_y > 0)
            shader_blur_vertical->uniform("blurSize", 1.5f / res_y);
    }
}

void rynx::application::renderer::on_resolution_change(size_t new_size_x,
    size_t new_size_y) {
    current_resolution = { new_size_x, new_size_y };
    set_geometry_resolution(current_internal_resolution_geometry.first,
        current_internal_resolution_geometry.second);
    set_lights_resolution(current_internal_resolution_lighting.first,
        current_internal_resolution_lighting.second);
}

void rynx::application::renderer::execute() {
    {
        rynx_profile("Main", "draw");
        m_rynx_renderer.setDepthTest(false);

        m_rynx_renderer.setCamera(camera);
        m_rynx_renderer.cameraToGPU();

        fbo_world_geometry->bind_as_output();
        fbo_world_geometry->clear();

        rynx::graphics::gl::enable_blend_for(0);
        rynx::graphics::gl::blend_func_default_for(0);
        rynx::graphics::gl::disable_blend_for(1);
        rynx::graphics::gl::disable_blend_for(2);

        geometry_pass->execute();
    }
    {
        fbo_world_geometry->bind_as_input();
        // The lightmap should accumulate light, but clearing to 0 means pitch black
        // shadows. We should clear it to an ambient darkness level or black
        // depending on how ambient_lights_effect behaves. The ambient_light_effect
        // adds to the buffer using additive blending, so clearing to 0 is correct!
        fbo_lights->bind_as_output();
        rynx::graphics::screenspace_draws::clear_screen();

        rynx::graphics::gl::enable_blend_for(0);
        rynx::graphics::gl::blend_func_cumulative();

        lighting_pass->execute();
    }

    {
        // Horizontal blur
        rynx::graphics::gl::disable_blend_for(
            0); // Disable blending for blur passes
        gpu_textures->bindTexture(0,
            fbo_lights->texture(0)); // tex_color is at unit 0
        fbo_lights_blur->bind_as_output();
        rynx::graphics::screenspace_draws::clear_screen();
        shader_blur_horizontal->activate();
        rynx::graphics::screenspace_draws::draw_fullscreen();
    }

    {
        // Vertical blur
        gpu_textures->bindTexture(
            0, fbo_lights_blur->texture(0)); // tex_color is at unit 0
        fbo_lights->bind_as_output();
        rynx::graphics::screenspace_draws::clear_screen();
        shader_blur_vertical->activate();
        rynx::graphics::screenspace_draws::draw_fullscreen();
    }

    {
        // Composite pass
        rynx::graphics::gl::disable_blend_for(0);
        gpu_textures->bindTexture(
            0, fbo_world_geometry->texture(0)); // tex_color is at unit 0
        gpu_textures->bindTexture(
            1, fbo_lights->texture(0)); // tex_lightmap is at unit 1

        shader_composite->activate();
        rynx::graphics::framebuffer::bind_backbuffer();
        rynx::graphics::screenspace_draws::clear_screen();
        rynx::graphics::screenspace_draws::draw_fullscreen();
    }
}

void rynx::application::renderer::prepare(rynx::scheduler::context* ctx) {

    {
        auto& ecs = ctx->get_resource<rynx::ecs>();

        // add missing texture components
        {
            std::vector<rynx::ecs::id> ids =
                ecs.query()
                .in<rynx::components::graphics::mesh>()
                .notIn<rynx::components::graphics::texture>()
                .ids();
            for (auto&& id : ids)
                ecs[id].add(rynx::components::graphics::texture{ "" });
        }

        // add missing matrix4 components
        {
            std::vector<rynx::ecs::id> ids =
                ecs.query()
                .in<rynx::components::transform::position>()
                .notIn<rynx::components::transform::matrix>()
                .ids();
            for (auto&& id : ids)
                ecs[id].add(rynx::components::transform::matrix());
        }

        // convert human readable and storable texture info to runtime tex info.
        {
            struct unhandled_entity_tex {
                rynx::ecs::id entity_id;
                rynx::graphics::texture_id tex_id;
            };

            std::vector<unhandled_entity_tex> found;
            ecs.query().notIn<rynx::graphics::texture_id>().for_each(
                [this, &found](rynx::ecs::id id,
                    rynx::components::graphics::texture tex) {
                        found.emplace_back(
                            id, gpu_textures->findTextureByName(tex.textureName));
                });
            for (auto&& missingEntityTex : found) {
                ecs[missingEntityTex.entity_id].add(
                    rynx::graphics::texture_id(missingEntityTex.tex_id));
            }
        }
    }

    geometry_pass->prepare(ctx);
    lighting_pass->prepare(ctx);
}

void rynx::application::renderer::geometry_step_insert_front(
    rynx::unique_ptr<igraphics_step> step) {
    geometry_pass->add_graphics_step(std::move(step), true);
}

void rynx::application::renderer::light_global_ambient(rynx::floats4 color) {
    static_cast<rynx::application::visualisation::ambient_light_effect*>(
        m_ambients)
        ->set_global_ambient(color);
}

void rynx::application::renderer::light_global_directed(rynx::floats4 color,
    rynx::vec3f direction) {
    m_ambients->set_global_directed(color, direction);
}

void rynx::application::renderer::enable_shadows(bool enable) {
    if (m_omnilights)
        m_omnilights->enable_shadows(enable);
    if (m_directedlights)
        m_directedlights->enable_shadows(enable);
    if (m_ambients)
        m_ambients->enable_shadows(enable);
}

void rynx::application::renderer::set_shadow_density(float density) {
    m_shadow_density = density;
    if (m_omnilights)
        m_omnilights->set_shadow_density(density);
    if (m_directedlights)
        m_directedlights->set_shadow_density(density);
    if (m_ambients)
        m_ambients->set_shadow_density(density);
}