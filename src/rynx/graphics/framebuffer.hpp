
#pragma once

typedef int GLint;
typedef unsigned GLuint;

#include <rynx/tech/unordered_map.hpp>
#include <rynx/graphics/texture/id.hpp>
#include <rynx/tech/std/string.hpp>
#include <vector>

namespace rynx {
	namespace graphics {
		class GPUTextures;
		class framebuffer {
		public:
			framebuffer(rynx::shared_ptr<rynx::graphics::GPUTextures> textures, const rynx::string& name);
			~framebuffer();

			class config {
			public:
				rynx::shared_ptr<framebuffer> construct(rynx::shared_ptr<rynx::graphics::GPUTextures> textures, rynx::string name);

				config& set_default_resolution(int width, int height);
				config& add_rgba8_target(rynx::string name, int res_x, int res_y);
				config& add_rgba8_target(rynx::string name);

				config& add_float_target(rynx::string name, int res_x, int res_y);
				config& add_float_target(rynx::string name);

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
					rynx::string name;
				};

				int m_default_resolution_x = 0;
				int m_default_resolution_y = 0;
				std::vector<render_target> m_targets;
				render_target m_depth_target;
			};

			rynx::graphics::texture_id operator [](const rynx::string& name);
			rynx::graphics::texture_id get_texture_name_of_render_target(const rynx::string& name) { return this->operator[](name); }
			void destroy();
			void clear() const;

			void bind_as_output() const;
			void bind_as_input(int32_t starting_at_texture_unit = 0) const;
			void bind_for_readback() const;
			
			rynx::graphics::texture_id depth_texture() const;
			rynx::graphics::texture_id texture(size_t target) const;

			size_t width() const;
			size_t height() const;

			size_t target_count() const;
			static void bind_backbuffer();

		private:
			rynx::string fbo_name;

			rynx::unordered_map<rynx::string, rynx::graphics::texture_id> m_tex_map; // map from render target name to texture id.

			int32_t resolution_x = 0;
			int32_t resolution_y = 0;

			struct fbo_tex {
				rynx::string name;
				rynx::graphics::texture_id id;
			};

			GLuint location;
			rynx::shared_ptr<rynx::graphics::GPUTextures> m_textures;
			rynx::string depthtexture;
			rynx::graphics::texture_id depthtexture_id;
			std::vector<fbo_tex> targets;
		};
	}
}
