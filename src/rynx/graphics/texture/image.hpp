
#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/tech/std/string.hpp>

struct Image {
	Image() {}
	~Image() {
		if(data)
			unload();
	}
	
	Image(int width, int height) { createEmptyRGBA(width, height); }
	Image(const Image& other);
	Image& operator = (const Image& other);

	void loadImage(const rynx::string& filename);
	void unload();
	void rescale(int newX, int newY);

	void rgb_to_rgba();
	void loadByteBufferRGBA(unsigned char* in_data, int width, int height);
	void loadFloatBufferRGBA(rynx::floats4* data, int width, int height);
	void createEmptyRGBA(int width, int height);
	void fill(rynx::floats4 color);

	unsigned sizeX = 0;
	unsigned sizeY = 0;
	unsigned char* data = nullptr;
	bool hasAlpha = false;
};

