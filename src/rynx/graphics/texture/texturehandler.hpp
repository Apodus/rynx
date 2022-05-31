
#pragma once

#include <rynx/tech/serialization.hpp>
#include <rynx/graphics/texture/image.hpp>
#include <rynx/graphics/texture/id.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/math/vector.hpp>

#include <string>
#include <vector>

namespace rynx {
	namespace graphics {
		class GPUTextures {
		public:
			struct tex_conf {
				bool vertex_positions_as_uv = false;
				float vertex_position_scale = 1.0f;
			};

		private:
			struct tex_info {
				rynx::vec4<int32_t> textureSize;
				unsigned textureGL_ID = ~0u;
				unsigned bufferGL_ID = ~0u;
				unsigned buffer_gl_format = 0;
				std::string name;
				std::string filepath;
				bool in_atlas = false;
				bool has_buffer() const { return bufferGL_ID != ~0u; }
			};

			class dynamic_texture_atlas {

				int32_t m_atlas_size = 0; // physical texture is nxn
				int32_t m_slots_per_row = 0; // ..
				int32_t atlasBlockSize = 512; // smallest supported nxn reservation

				struct atlas_tex_info {
					int32_t slot = 0;
					int32_t slotSize = 1;
					int32_t textureMode = 0; // 0=use uv attributes, 1=vertex position as uv
					int32_t textureScale = 100000; // vertex position -> uv  scaling factor in 1/100000.
				};

				rynx::graphics::texture_id m_atlas_tex_id;
				rynx::graphics::texture_id m_indirection_buffer_tex_id;

				std::vector<atlas_tex_info> m_slots;
				rynx::dynamic_bitset m_tex_presence;
				
				rynx::graphics::GPUTextures* m_textures = nullptr;

				bool is_slot_free(int x, int y) {
					return !m_tex_presence.test(x + y * m_slots_per_row);
				}
				
				std::pair<int32_t, int32_t> findFreeSlot(int32_t slotSize);
				void reserveSlot(rynx::graphics::texture_id id, int x, int y, int slotSize);
				void evictTexture(rynx::graphics::texture_id id);

				int32_t blockSize() const {
					return atlasBlockSize;
				}

				void updateIndirectionBuffer(rynx::graphics::texture_id id);

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
				
				void texture_config_changed(rynx::graphics::texture_id id, tex_conf conf) {
					m_slots[id.value].textureMode = conf.vertex_positions_as_uv;
					m_slots[id.value].textureScale = int32_t(100000.0f * conf.vertex_position_scale);
					updateIndirectionBuffer(id);
				}
				
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

			rynx::serialization::vector_writer serialize() {
				rynx::serialization::vector_writer result;
				rynx::serialize(m_tex_configs, result);
				return result;
			}

			void deserialize(rynx::serialization::vector_reader& reader) {
				m_tex_configs.clear();
				rynx::deserialize(m_tex_configs, reader);
			}

			void bindAtlas() {
				bindTexture(0, m_texture_atlas.getAtlasTexture());
				bindTexture(1, m_texture_atlas.getUVIndirectionTexture());
			}

			struct texture_entry {
				tex_info info;
				rynx::graphics::texture_id tex_id;
			};
			
			std::vector<texture_entry> getListOfTextures() const;

			texture_id createTexture(const std::string&, Image& img);
			texture_id createTexture(const std::string&, int width, int height);
			texture_id createDepthTexture(const std::string& name, int width, int height, int bits_per_pixel = 16);
			texture_id createFloatTexture(const std::string& name, int width, int height);
			
			texture_id createBuffer(const std::string& name, size_t bufferSizeBytes);
			void bufferData(rynx::graphics::texture_id id, size_t bytes, void* data, size_t offset = 0);

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

			tex_conf get_tex_config(texture_id id) {
				if (id == texture_id())
					return tex_conf();
				
				auto tex_info_it = m_textures.find(id);
				auto tex_config_it = m_tex_configs.find(tex_info_it->second.name);
				if (tex_config_it == m_tex_configs.end())
					return m_tex_configs[tex_info_it->second.name];
				return tex_config_it->second;
			}

			void set_tex_config(texture_id id, tex_conf conf) {
				if (id == texture_id())
					return;

				auto tex_info_it = m_textures.find(id);
				m_tex_configs[tex_info_it->second.name] = conf;

				// update atlas also.
				m_texture_atlas.texture_config_changed(id, conf);
			}

		private:
			GPUTextures(const GPUTextures&) = delete;
			GPUTextures& operator=(const GPUTextures&) = delete;

			dynamic_texture_atlas m_texture_atlas;
			rynx::unordered_map<texture_id, tex_info> m_textures;
			rynx::unordered_map<std::string, tex_conf> m_tex_configs;
			std::vector<texture_id> current_textures;
			std::vector<texture_id> m_free_ids;
			int32_t m_next_id = 0;
		};
	}

	namespace serialization {
		template<>
		struct Serialize<rynx::graphics::GPUTextures::tex_conf> {
			template<typename IOStream>
			void serialize(const rynx::graphics::GPUTextures::tex_conf& t, IOStream& writer) {
				rynx::serialize(t.vertex_positions_as_uv, writer);
				rynx::serialize(t.vertex_position_scale, writer);
			}

			template<typename IOStream>
			void deserialize(rynx::graphics::GPUTextures::tex_conf& t, IOStream& reader) {
				rynx::deserialize(t.vertex_positions_as_uv, reader);
				rynx::deserialize(t.vertex_position_scale, reader);
			}
		};
	}
}