
#pragma once

typedef int GLint;
typedef unsigned GLuint;

#include <rynx/tech/unordered_map.hpp>
#include <string>
#include <vector>
#include <memory>

namespace rynx {
	namespace graphics {
		class GPUTextures;
		class framebuffer {
		public:
			framebuffer(std::shared_ptr<rynx::graphics::GPUTextures> textures, const std::string& name);
			~framebuffer();

			class config {
			public:
				std::shared_ptr<framebuffer> construct(std::shared_ptr<rynx::graphics::GPUTextures> textures, std::string name);

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
			void clear() const;

			void bind_as_output() const;
			void bind_as_input(int32_t starting_at_texture_unit = 0) const;
			void bind_for_readback() const;
			
			std::string depth_texture() const;
			std::string texture(size_t target) const;

			size_t width() const;
			size_t height() const;

			size_t target_count() const;
			static void bind_backbuffer();

		private:
			std::string fbo_name;

			rynx::unordered_map<std::string, std::string> m_tex_map; // map from render target name to texture name.

			int32_t resolution_x = 0;
			int32_t resolution_y = 0;

			GLuint location;
			std::shared_ptr<rynx::graphics::GPUTextures> m_textures;
			std::string depthtexture;
			std::vector<std::string> targets;
		};
	}
}
