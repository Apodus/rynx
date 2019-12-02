
#pragma once

typedef int GLint;
typedef unsigned GLuint;

#include <rynx/tech/unordered_map.hpp>
#include <string>
#include <vector>
#include <memory>

class GPUTextures;

namespace rynx
{
	namespace graphics {
		class framebuffer
		{
		public:
			framebuffer(std::shared_ptr<GPUTextures> textures, const std::string& name);
			~framebuffer();

			class config {
			public:
				std::shared_ptr<framebuffer> construct(std::shared_ptr<GPUTextures> textures, std::string name);

				config& set_default_resolution(int width, int height);
				config& add_rgba8_target(std::string name, int res_x, int res_y);
				config& add_rgba8_target(std::string name);

				config& add_float_target(std::string name, int res_x, int res_y);
				config& add_float_target(std::string name);

				config& add_depth16_target(int res_x, int res_y);
				config& add_depth16_target();

				config& add_depth32_target(int res_x, int res_y);
				config& add_depth32_target();

			private:
				enum class render_target_type {
					none,
					rgba8,
					float32,
					depth16,
					depth32
				};

				struct render_target {
					int resolution_x = 0;
					int resolution_y = 0;
					render_target_type type = render_target_type::none;
					std::string name;
				};

				int m_default_resolution_x = 0;
				int m_default_resolution_y = 0;
				std::vector<render_target> m_targets;
				render_target m_depth_target;
			};

			const std::string& operator [](const std::string& name);
			const std::string& get_texture_name_of_render_target(const std::string& name) { return this->operator[](name); }
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

		private:
			void bind_helper(size_t target_count) const;

			std::string fbo_name;

			rynx::unordered_map<std::string, std::string> m_tex_map; // map from render target name to texture name.

			size_t resolution_x;
			size_t resolution_y;

			GLuint location;
			std::shared_ptr<GPUTextures> m_textures;
			std::string depthtexture;
			std::vector<std::string> targets;
		};
	}
}
