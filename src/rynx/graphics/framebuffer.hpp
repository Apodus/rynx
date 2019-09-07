
#pragma once

typedef int GLint;
typedef unsigned GLuint;

#include <rynx/tech/unordered_map.hpp>
#include <string>
#include <vector>
#include <memory>

class GPUTextures;

namespace Graphics
{
	class Framebuffer
	{
	public:
		Framebuffer() {}
		Framebuffer(std::shared_ptr<GPUTextures> textures);
		Framebuffer(std::shared_ptr<GPUTextures> textures, const std::string& prefix, size_t screen_width, size_t screen_height, bool depth_texture, size_t target_count);
		
		void add_float_target();
		void destroy();
		
		void bind() const;
		void bind_for_reading() const;
		void bind_for_writing() const;
		void bind(size_t target_count) const;
		
		std::string depth_texture() const;
		std::string texture(size_t target) const;
		
		size_t width() const;
		size_t height() const;
		
		size_t target_count() const;
		
		static void unbind();
		static void create(const std::string& name, const Framebuffer& );
		static void create(std::shared_ptr<GPUTextures> textures, const std::string& prefix, size_t screen_width, size_t screen_height, bool depth_texture, size_t target_count );
		static Framebuffer& get(const std::string&);
		
	private:
		static rynx::unordered_map<std::string, Framebuffer> fbos;
		
		std::string target_name(size_t target) const;
		void bind_helper(size_t target_count) const;
		
		std::string prefix;
		size_t resolution_x;
		size_t resolution_y;
		
		GLuint location;
		std::shared_ptr<GPUTextures> m_textures;
		std::string depthtexture;
		std::vector<std::string> targets;
	};
}
