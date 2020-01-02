
#include <rynx/graphics/opengl.hpp>
#include "framebuffer.hpp"
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/system/assert.hpp>

#include <string>
#include <stdexcept>

namespace {
	constexpr int FboMaxTextures = 16;
	constexpr int default_resolution = -1;
}

rynx::graphics::framebuffer::framebuffer(std::shared_ptr<GPUTextures> textures, const std::string& name):
	fbo_name(name),
	location(0),
	m_textures(textures)
{
	logmsg("creating FBO '%s'", name.c_str());
	glGenFramebuffers(1, &location);
	glBindFramebuffer(GL_FRAMEBUFFER, location);

	rynx_assert(glGetError() == GL_NO_ERROR, "opengl error :(");
}

rynx::graphics::framebuffer::~framebuffer() {
	logmsg("framebuffer destroyed");
	destroy();
}

std::shared_ptr<rynx::graphics::framebuffer> rynx::graphics::framebuffer::config::construct(std::shared_ptr<GPUTextures> textures, std::string name) {
	auto fbo = std::make_shared<rynx::graphics::framebuffer>(textures, name);
	fbo->resolution_x = m_default_resolution_x;
	fbo->resolution_y = m_default_resolution_y;

	auto add_one = [this, &textures, &name, &fbo](render_target& render_target_instance) {
		auto add_depth_buffer = [&](int bits_per_pixel) {
			fbo->depthtexture = name + std::string("_depth") + std::to_string(bits_per_pixel);
			if (render_target_instance.resolution_x == default_resolution) {
				textures->createDepthTexture(fbo->depthtexture, int(m_default_resolution_x), int(m_default_resolution_y), bits_per_pixel);
			}
			else {
				textures->createDepthTexture(fbo->depthtexture, render_target_instance.resolution_x, render_target_instance.resolution_y, bits_per_pixel);
			}
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textures->getTextureID(fbo->depthtexture), 0);
		};

		switch (render_target_instance.type) {
		case render_target_type::rgba8:
			fbo->targets.emplace_back(name + std::string("_rgba8_slot") + std::to_string(fbo->targets.size()));
			fbo->m_tex_map.emplace(render_target_instance.name, fbo->targets.back());
			if (render_target_instance.resolution_x == default_resolution) {
				textures->createTexture(fbo->targets.back(), int(m_default_resolution_x), int(m_default_resolution_y));
			}
			else {
				textures->createTexture(fbo->targets.back(), render_target_instance.resolution_x, render_target_instance.resolution_y);
			}
			glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0 + int(fbo->targets.size() - 1),
				GL_TEXTURE_2D,
				textures->getTextureID(fbo->targets.back()),
				0
			);
			break;

		case render_target_type::float32:
			fbo->targets.emplace_back(name + std::string("_float32_slot") + std::to_string(fbo->targets.size()));
			fbo->m_tex_map.emplace(render_target_instance.name, fbo->targets.back());
			if (render_target_instance.resolution_x == default_resolution) {
				textures->createFloatTexture(fbo->targets.back(), int(m_default_resolution_x), int(m_default_resolution_y));
			}
			else {
				textures->createFloatTexture(fbo->targets.back(), render_target_instance.resolution_x, render_target_instance.resolution_y);
			}
			glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				GL_COLOR_ATTACHMENT0 + int(fbo->targets.size() - 1),
				GL_TEXTURE_2D,
				textures->getTextureID(fbo->targets.back()),
				0
			);
			break;

		case render_target_type::depth16:
			add_depth_buffer(16);
			break;

		case render_target_type::depth32:
			add_depth_buffer(32);
			break;
		}
	};

	for (auto&& render_target_instance : m_targets) {
		add_one(render_target_instance);
	}
	if (m_depth_target.type != render_target_type::none) {
		add_one(m_depth_target);
	}

	rynx_assert(glGetError() == GL_NO_ERROR, "gl error :(");

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		throw std::runtime_error(std::string("Failed to create FBO: '") + name + std::string("', error: ") + std::to_string(status));
	}

	return fbo;
}

rynx::graphics::framebuffer::config& rynx::graphics::framebuffer::config::set_default_resolution(int x, int y) {
	m_default_resolution_x = x;
	m_default_resolution_y = y;
	return *this;
}

rynx::graphics::framebuffer::config& rynx::graphics::framebuffer::config::add_rgba8_target(std::string name, int res_x, int res_y) {
	m_targets.emplace_back(render_target{ res_x, res_y, render_target_type::rgba8, name });
	return *this;
}

rynx::graphics::framebuffer::config& rynx::graphics::framebuffer::config::add_rgba8_target(std::string name) {
	m_targets.emplace_back(render_target{default_resolution, default_resolution, render_target_type::rgba8, name});
	return *this;
}

rynx::graphics::framebuffer::config& rynx::graphics::framebuffer::config::add_float_target(std::string name, int res_x, int res_y) {
	m_targets.emplace_back(render_target{ res_x, res_y, render_target_type::float32, name });
	return *this;
}

rynx::graphics::framebuffer::config& rynx::graphics::framebuffer::config::add_float_target(std::string name) {
	m_targets.emplace_back(render_target{ default_resolution, default_resolution, render_target_type::float32, name });
	return *this;
}

rynx::graphics::framebuffer::config& rynx::graphics::framebuffer::config::add_depth16_target(int res_x, int res_y) {
	rynx_assert(m_depth_target.type == render_target_type::none, "depth target has already been added! can't have more than one depth target.");
	m_depth_target.type = render_target_type::depth16;
	m_depth_target.resolution_x = res_x;
	m_depth_target.resolution_y = res_y;
	return *this;
}

rynx::graphics::framebuffer::config& rynx::graphics::framebuffer::config::add_depth16_target() {
	rynx_assert(m_depth_target.type == render_target_type::none, "depth target has already been added! can't have more than one depth target.");
	m_depth_target.type = render_target_type::depth16;
	m_depth_target.resolution_x = default_resolution;
	m_depth_target.resolution_y = default_resolution;
	return *this;
}

rynx::graphics::framebuffer::config& rynx::graphics::framebuffer::config::add_depth32_target(int res_x, int res_y) {
	rynx_assert(m_depth_target.type == render_target_type::none, "depth target has already been added! can't have more than one depth target.");
	m_depth_target.type = render_target_type::depth32;
	m_depth_target.resolution_x = res_x;
	m_depth_target.resolution_y = res_y;
	return *this;
}

rynx::graphics::framebuffer::config& rynx::graphics::framebuffer::config::add_depth32_target() {
	rynx_assert(m_depth_target.type == render_target_type::none, "depth target has already been added! can't have more than one depth target.");
	m_depth_target.type = render_target_type::depth32;
	m_depth_target.resolution_x = default_resolution;
	m_depth_target.resolution_y = default_resolution;
	return *this;
}

const std::string& rynx::graphics::framebuffer::operator [](const std::string& name) {
	auto it = m_tex_map.find(name);
	rynx_assert(it != m_tex_map.end(), "requested render target does not exist in this FBO");
	return it->second;
}

void rynx::graphics::framebuffer::destroy() {
	glDeleteFramebuffers(1, &location);
	for(size_t i = 0; i < targets.size(); ++i)
	{
		m_textures->deleteTexture(targets[i]);
	}
	targets.clear();

	if(!depthtexture.empty())
	{
		m_textures->deleteTexture(depthtexture);
	}
}

void rynx::graphics::framebuffer::bind_as_output() const
{
	rynx_assert(targets.size() <= targets.size(), "error");
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, location);
	
	if (targets.empty())
	{
		glDrawBuffer(GL_NONE);
	}
	else
	{
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(GLsizei(targets.size()), buffers);
	}
	
	glViewport(0, 0, resolution_x, resolution_y);
	int32_t error_v = glGetError();
	rynx_assert(error_v == GL_NO_ERROR, "gl error: %d", error_v);
}

void rynx::graphics::framebuffer::bind_as_input(int32_t starting_at_texture_unit) const
{
	for(size_t i=0; i<targets.size(); ++i)
		m_textures->bindTexture(starting_at_texture_unit + i, this->texture(i));
}


void rynx::graphics::framebuffer::bind_for_readback() const
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, location);
}

void rynx::graphics::framebuffer::clear() const {
	if (!depthtexture.empty()) {
		float depth_clear_value = 1.0f;
		glClearBufferfv(GL_DEPTH, 0, &depth_clear_value);
	}

	for(size_t i=0; i<targets.size(); ++i) {
		float color_clear[] = { 0, 0, 0, 0 };
		glClearBufferfv(GL_COLOR, static_cast<int>(i), color_clear);
	}
}

void rynx::graphics::framebuffer::bind_backbuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::string rynx::graphics::framebuffer::depth_texture() const
{
	rynx_assert(!depthtexture.empty(), "??");
	return depthtexture;
}

std::string rynx::graphics::framebuffer::texture(size_t target) const
{
	rynx_assert(target < targets.size(), "??");
	return targets[target];
}

size_t rynx::graphics::framebuffer::width() const
{
	return resolution_x;
}

size_t rynx::graphics::framebuffer::height() const
{
	return resolution_y;
}

size_t rynx::graphics::framebuffer::target_count() const
{
	return targets.size();
}

