
#include <rynx/graphics/opengl.hpp>
#include "framebuffer.hpp"
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/system/assert.hpp>

#include <sstream>
#include <stdexcept>
#include <iomanip>

enum { MAX_TARGETS = 4 };

rynx::unordered_map<std::string, Graphics::Framebuffer> Graphics::Framebuffer::fbos;

Graphics::Framebuffer::Framebuffer(std::shared_ptr<GPUTextures> textures):
	resolution_x(0),
	resolution_y(0),
	location(0),
	m_textures(textures)
{
}

Graphics::Framebuffer::Framebuffer(std::shared_ptr<GPUTextures> textures, const std::string& prefixx, size_t screen_width, size_t screen_height, bool depth_texture, size_t target_count):
	prefix(prefixx),
	resolution_x(screen_width),
	resolution_y(screen_height),
	location(0),
	m_textures(textures)
{
	rynx_assert(target_count <= MAX_TARGETS, "too many targets");
	targets.reserve(target_count);

	glGenFramebuffers(1, &location);
	glBindFramebuffer(GL_FRAMEBUFFER, location);

	for(size_t i = 0; i < target_count; ++i)
	{
		targets.push_back(target_name(i));
		textures->createTexture(targets.back(), int(resolution_x), int(resolution_y));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + int(i), GL_TEXTURE_2D, textures->getTextureID(targets.back()), 0);
	}
	if(!target_count)
	{
		glDrawBuffer(GL_NONE);
	}

	if(depth_texture)
	{
		depthtexture = prefix + "_depthtexture";
		textures->createDepthTexture(depthtexture, int(resolution_x), int(resolution_y));
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, textures->getTextureID(depthtexture), 0);
	}

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::stringstream ss;
		ss << "Failed to create FBO: '" << prefix << "', error: 0x" << std::hex << status;
		throw std::runtime_error(ss.str());
	}
}


void Graphics::Framebuffer::create(const std::string& name, const Framebuffer& fb)
{
	fbos[name] = fb;
}

void Graphics::Framebuffer::create(std::shared_ptr<GPUTextures> textures, const std::string& prefix, size_t screen_width, size_t screen_height, bool depth_texture, size_t target_count )
{
	Framebuffer fb(textures, prefix, screen_width, screen_height, depth_texture, target_count);
	fbos[prefix] = fb;
}

Graphics::Framebuffer& Graphics::Framebuffer::get(const std::string& s)
{
	return fbos[s];
}


void Graphics::Framebuffer::destroy()
{
	glDeleteFramebuffers(1, &location);
	for(size_t i = 0; i < targets.size(); ++i)
	{
		m_textures->deleteTexture(targets[i]);
	}
	if(!depthtexture.empty())
	{
		m_textures->deleteTexture(depthtexture);
	}
}

void Graphics::Framebuffer::bind() const
{
	bind(targets.size());
}

void Graphics::Framebuffer::bind(size_t target_count) const
{
	rynx_assert(target_count <= targets.size(), "error");
	glBindFramebuffer(GL_FRAMEBUFFER, location);
	bind_helper(target_count);
}

void Graphics::Framebuffer::bind_for_reading() const
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, location);
}

void Graphics::Framebuffer::bind_for_writing() const
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, location);
	bind_helper(targets.size());
}

void Graphics::Framebuffer::bind_helper(size_t target_count) const
{
	if(targets.empty())
	{
		glDrawBuffer(GL_NONE);
	}
	else
	{
		GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(GLsizei(target_count), buffers);
	}
	glViewport(0, 0, int(resolution_x), int(resolution_y));
}

void Graphics::Framebuffer::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::string Graphics::Framebuffer::depth_texture() const
{
	rynx_assert(!depthtexture.empty(), "??");
	return depthtexture;
}

std::string Graphics::Framebuffer::texture(size_t target) const
{
	rynx_assert(target < targets.size(), "??");
	return targets[target];
}

std::string Graphics::Framebuffer::target_name(size_t target) const
{
	std::stringstream ss;
	ss << prefix << "_texture" << target;
	return ss.str();
}

size_t Graphics::Framebuffer::width() const
{
	return resolution_x;
}

size_t Graphics::Framebuffer::height() const
{
	return resolution_y;
}

// this should never be used for graphics things.
// for general computing it might be good.
void Graphics::Framebuffer::add_float_target()
{
	rynx_assert(targets.size() < MAX_TARGETS, "??");
	size_t target = targets.size();
	targets.push_back(target_name(target));
	m_textures->createFloatTexture(targets.back(), int(resolution_x), int(resolution_y));
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + int(target), GL_TEXTURE_2D, m_textures->getTextureID(targets.back()), 0);
}

size_t Graphics::Framebuffer::target_count() const
{
	return targets.size();
}

