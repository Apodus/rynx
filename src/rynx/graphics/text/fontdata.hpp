
#pragma once

#include <rynx/graphics/text/glyph.hpp>
#include <rynx/tech/math/vector.hpp>
#include <string>


class FontData {
public:
	std::string textureID;
	int totalWidth;
	int totalHeight;
	int lineHeight;
	int baseLine;

	Glyph glyphData[256];
	vec4<float> textureCoordinates[256];

	FontData() {}

	void fillTextureCoordinates() {
		for (Glyph glyph : glyphData) {
			if (glyph.id == -1) {
				continue;
			}

			int c = glyph.id;

			float x = glyph.x;
			float y = glyph.y;
			float width = glyph.width;
			float height = glyph.height;

			float left = x * 1.0f / totalWidth;
			float right = (x + width) * 1.0f / totalWidth;
			float bottom = y * 1.0f / totalHeight;
			float top = (y + height) * 1.0f / totalHeight;

			textureCoordinates[c] = vec4<float>(left, right, top, bottom);
		}
	}

	const Glyph& operator [](int index) const {
		return glyphData[index];
	}

	Glyph& operator [](int index) {
		return glyphData[index];
	}
};

