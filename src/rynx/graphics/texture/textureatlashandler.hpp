
#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/graphics/texture/textureatlas.hpp>

#include <string>

class TextureAtlasHandler {

	struct SubTexture {
		vec4<float> subTextureCoordinates;
		std::string realTextureID;

		SubTexture() {
		}

		SubTexture(const vec4<float>& subTextureCoordinates, const std::string& realTextureID) {
			this->subTextureCoordinates = subTextureCoordinates;
			this->realTextureID = realTextureID;
		}
	};

	rynx::unordered_map<std::string, SubTexture> subTextures;
	vec4<float> DEFAULT_TEXTURE_COORDINATES;

public:
	TextureAtlasHandler() {
		DEFAULT_TEXTURE_COORDINATES = vec4<float>(0.5f, 0.5f, 1.0f, 1.0f);
	}

	void addTextureAtlas(const TextureAtlas& textureAtlas) {
		const std::string& realTextureID = textureAtlas.getRealTextureID();
		for(const std::string& subTextureID : textureAtlas.getSubTextureTextureIDs()) {
			vec4<float> subTextureCoordinates = textureAtlas.getSubTextureCoordinates(subTextureID);
			SubTexture subTexture(subTextureCoordinates, realTextureID);
			subTextures.emplace(subTextureID, subTexture);
			logmsg("Added subtexture: %s", subTextureID.c_str());
		}
	}

	const std::string& getRealTextureID(const std::string& subTextureID) const {
		auto it = subTextures.find(subTextureID);
		if(it != subTextures.end())
			return it->second.realTextureID;
		return subTextureID;
	}

	const vec4<float>& getTextureCoordinateLimits(const std::string& subTextureID) const {
		auto it = subTextures.find(subTextureID);
		if(it == subTextures.end()) {
			logmsg("Could not find %s", subTextureID.c_str());
			return DEFAULT_TEXTURE_COORDINATES;
		}
		return it->second.subTextureCoordinates;
	}

	vec4<float> getTextureCoordinateLimits(const std::string& textureID, vec4<float> uvLimits) const {
		vec4<float> subtex_limits = getTextureCoordinateLimits(textureID);
		float width = subtex_limits[2] - subtex_limits[0];
		float height = subtex_limits[3] - subtex_limits[1];
		uvLimits[0] *= width;
		uvLimits[2] *= width;
		uvLimits[1] *= height;
		uvLimits[3] *= height;
		return subtex_limits + uvLimits;
	}

};