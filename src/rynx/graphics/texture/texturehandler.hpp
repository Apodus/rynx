
#pragma once

#include <rynx/graphics/texture/image.hpp>
#include <rynx/graphics/texture/textureatlashandler.hpp>
#include <rynx/tech/unordered_map.hpp>

#include <string>
#include <vector>

class GPUTextures
{
	public:
		GPUTextures();
		~GPUTextures();

		// static GPUTextures& getSingleton();

		void createTextures(const std::string& filename);
		unsigned createTexture(const std::string&, const std::string&);
		unsigned createTexture(const std::string&, Image& img);
		unsigned createTexture(const std::string&, int width, int height);
		unsigned createDepthTexture(const std::string& name, int width, int height, int bits_per_pixel = 16);
		unsigned createFloatTexture(const std::string& name, int width, int height);
		
		int bindTexture(size_t texture_unit, const std::string&);
		void unbindTexture(size_t texture_unit);

		bool textureExists(const std::string& name) const;
		const std::string& getCurrentTexture(size_t texture_unit) const;
		unsigned getTextureID(const std::string&) const;

		floats4 textureLimits(const std::string&) const;
		floats4 textureLimits(const std::string& name, vec4<float> uvLimits) const;

		void insertAtlas(const TextureAtlas& atlas);
		
		void deleteTexture(const std::string& name);
		void deleteAllTextures();

	private:
		TextureAtlasHandler m_atlasHandler;

		GPUTextures(const GPUTextures&) = delete;
		GPUTextures& operator=(const GPUTextures&) = delete;
		
		rynx::unordered_map<std::string, vec4<short>> textureSizes;
		rynx::unordered_map<std::string, unsigned> textures;
		std::vector<std::string> current_textures;

};
