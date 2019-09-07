
#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/graphics/texture/textureatlas.hpp>

#include <string>
#include <unordered_map>

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

	std::unordered_map<std::string, SubTexture> subTextures;
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
			subTextures[subTextureID] = subTexture;
			// logmsg("Added subtexture: %s", subTextureID.c_str());
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

	vec4<float> combineUVLimits(const vec4<float>& uvLimits, const vec4<float>& atlasLimits) const {
		vec4<float> limits;
		limits.data[0] = uvLimits.data[0] * atlasLimits.data[2] + atlasLimits.data[0];
		limits.data[1] = uvLimits.data[1] * atlasLimits.data[3] + atlasLimits.data[1];
		limits.data[2] = uvLimits.data[2] * atlasLimits.data[2];
		limits.data[3] = uvLimits.data[3] * atlasLimits.data[3];
		return limits;
	}

	vec4<float> getTextureCoordinateLimits(const std::string& textureID, const vec4<float>& uvLimits) const {
		vec4<float> atlasLimits = getTextureCoordinateLimits(textureID);
		return combineUVLimits(uvLimits, atlasLimits);
	}

};