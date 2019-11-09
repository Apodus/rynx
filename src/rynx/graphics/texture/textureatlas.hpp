
#pragma once

#include <rynx/tech/unordered_map.hpp>
#include <rynx/tech/math/vector.hpp>
#include <rynx/system/assert.hpp>

#include <string>
#include <vector>

class TextureAtlas {

	std::string textureID;
	rynx::unordered_map<std::string, vec4<float>> textureCoordinates;
	std::vector<std::string> textureIDs;

	float halfPixelWidth;
	float halfPixelHeight;

public:
	TextureAtlas(const std::string& realTexture, int pixelCountX, int pixelCountY) {
		textureID = realTexture;
		rynx_assert(pixelCountX * pixelCountY > 0, "TextureAtlas pointing to bad texture name: %s", realTexture.c_str());
		halfPixelWidth = 1.0f / pixelCountX;
		halfPixelHeight = 1.0f / pixelCountY;
	}

	void insertTexture(const std::string& subTextureID, int slotX, int slotY, int slotCountX, int slotCountY) {
		vec4<float> texCoord(
			(slotX + 0.0f) / slotCountX + halfPixelWidth,
			((slotY + 1.0f) / slotCountY - halfPixelHeight),
			(slotX + 1.0f) / slotCountX - halfPixelWidth,
			((slotY + 0.0f) / slotCountY + halfPixelHeight)
		);

		rynx_assert(textureCoordinates.find(subTextureID) == textureCoordinates.end(), "inserting subtexture that already exists: %s", subTextureID.c_str());

		textureCoordinates.emplace(subTextureID, texCoord);
		textureIDs.push_back(subTextureID);
	}

	const std::string& getRealTextureID() const {
		return textureID;
	}

	const vec4<float>& getSubTextureCoordinates(const std::string& subTexture) const {
		auto it = textureCoordinates.find(subTexture);
		rynx_assert(it != textureCoordinates.end(), "subtexture not found: %s", subTexture.c_str());
		return it->second;
	}


	const std::vector<std::string>& getSubTextureTextureIDs() const {
		return textureIDs;
	}
};