
#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/tech/unordered_map.hpp>
#include <rynx/graphics/texture/textureatlas.hpp>

#include <string>

class TextureAtlasHandler {

	struct SubTexture {
		rynx::floats4 subTextureCoordinates;
		std::string realTextureID;

		SubTexture() {}
		SubTexture(rynx::floats4 subTextureCoordinates, const std::string& realTextureID) {
			this->subTextureCoordinates = subTextureCoordinates;
			this->realTextureID = realTextureID;
		}
	};

	rynx::unordered_map<std::string, SubTexture> subTextures;
	rynx::vec4<float> DEFAULT_TEXTURE_COORDINATES;

public:
	TextureAtlasHandler() {
		DEFAULT_TEXTURE_COORDINATES = rynx::vec4<float>(0.5f, 0.5f, 1.0f, 1.0f);
	}

	void addTextureAtlas(const TextureAtlas& textureAtlas) {
		const std::string& realTextureID = textureAtlas.getRealTextureID();
		for(const std::string& subTextureID : textureAtlas.getSubTextureTextureIDs()) {
			rynx::vec4<float> subTextureCoordinates = textureAtlas.getSubTextureCoordinates(subTextureID);
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

	rynx::floats4 getTextureCoordinateLimits(const std::string& subTextureID) const {
		auto it = subTextures.find(subTextureID);
		if(it == subTextures.end()) {
			logmsg("Could not find %s", subTextureID.c_str());
			return DEFAULT_TEXTURE_COORDINATES;
		}
		return it->second.subTextureCoordinates;
	}

	rynx::floats4 getTextureCoordinateLimits(const std::string& textureID, rynx::floats4 uvLimits) const {
		rynx::floats4 subtex_limits = getTextureCoordinateLimits(textureID);
		float width = subtex_limits.z - subtex_limits.x;
		float height = subtex_limits.w - subtex_limits.y;
		
		uvLimits.x *= width;
		uvLimits.z *= width;
		uvLimits.y *= height;
		uvLimits.w *= height;
		return subtex_limits + uvLimits;
	}

};