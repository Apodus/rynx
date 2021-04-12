
#pragma once

#include <rynx/graphics/texture/image.hpp>
#include <rynx/graphics/texture/id.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/math/vector.hpp>


#include <string>
#include <vector>

namespace rynx {
	namespace graphics {
		class GPUTextures {
			struct tex_info {
				rynx::vec4<int32_t> textureSize;
				unsigned textureGL_ID = ~0u;
				unsigned bufferGL_ID = ~0u;
				unsigned buffer_gl_format;
				std::string name;
				std::string filepath;
				bool in_atlas = false;

				bool has_buffer() const { return bufferGL_ID != ~0u; }
			};

			class dynamic_texture_atlas {

				int32_t m_atlas_size = 0;
				rynx::graphics::texture_id m_atlas_tex_id;
				rynx::graphics::texture_id m_indirection_buffer_tex_id;

				struct atlas_tex_info {
					int32_t slot = 0;
					int32_t slotSize = 1;
				};

				std::vector<atlas_tex_info> m_slots;
				rynx::dynamic_bitset m_tex_presence;
				
				rynx::graphics::GPUTextures* m_textures;

				int32_t m_slots_per_row = 0;
				int32_t atlasBlockSize = 512;

				bool is_slot_free(int x, int y) {
					return !m_tex_presence.test(x + y * m_slots_per_row);
				}
				
				std::pair<int32_t, int32_t> findFreeSlot(int32_t slotSize);
				void reserveSlot(rynx::graphics::texture_id id, int x, int y, int slotSize);
				void evictTexture(rynx::graphics::texture_id id);

				int32_t blockSize() const {
					return atlasBlockSize;
				}

				void loadImage(
					rynx::graphics::texture_id id,
					Image&& img,
					int slot_x,
					int slot_y,
					int slotSize
				);

			public:
				dynamic_texture_atlas() = default;
				void init(GPUTextures* textures);
				void loadImage(rynx::graphics::texture_id id, Image&& img);
				void loadImage(rynx::graphics::texture_id id, const tex_info& info);

				texture_id getAtlasTexture() const {
					return m_atlas_tex_id;
				}

				texture_id getUVIndirectionTexture() const {
					return m_indirection_buffer_tex_id;
				}

				int getAtlasBlocksPerRow() const {
					return m_slots_per_row;
				}
			};

		public:
			GPUTextures();
			~GPUTextures();

			void loadTexturesFromPath(const std::string& filename);
			texture_id loadToTextureAtlas(const std::string&, const std::string&);
			int getAtlasBlocksPerRow() const {
				return m_texture_atlas.getAtlasBlocksPerRow();;
			}

			void bindAtlas() {
				bindTexture(0, m_texture_atlas.getAtlasTexture());
				bindTexture(1, m_texture_atlas.getUVIndirectionTexture());
			}
			
			texture_id createTexture(const std::string&, Image& img);
			texture_id createTexture(const std::string&, int width, int height);
			texture_id createDepthTexture(const std::string& name, int width, int height, int bits_per_pixel = 16);
			texture_id createFloatTexture(const std::string& name, int width, int height);
			
			texture_id createBuffer(const std::string& name);
			void bufferData(rynx::graphics::texture_id id, size_t bytes, void* data);

			int bindTexture(size_t texture_unit, texture_id);
			void unbindTexture(size_t texture_unit);

			texture_id findTextureByName(std::string_view name) const;

			bool textureExists(std::string name) const;
			bool textureExists(texture_id id) const;

			rynx::graphics::texture_id getCurrentTexture(size_t texture_unit) const;
			unsigned getGLID(texture_id) const;

			void deleteTexture(texture_id id);
			void deleteAllTextures();

			texture_id generate_tex_id();

		private:
			GPUTextures(const GPUTextures&) = delete;
			GPUTextures& operator=(const GPUTextures&) = delete;

			dynamic_texture_atlas m_texture_atlas;
			rynx::unordered_map<texture_id, tex_info> m_textures;
			std::vector<texture_id> current_textures;
			std::vector<texture_id> m_free_ids;
			int32_t m_next_id = 0;
		};
	}
}