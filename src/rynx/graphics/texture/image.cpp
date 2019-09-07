
#include "image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fstream>

void Image::loadImage(const std::string& filename)
{
	std::ifstream in(filename, std::ios::binary);
	in.seekg(0, std::ios::end);
	auto size = in.tellg();
	in.seekg(0, std::ios::beg);
	std::unique_ptr<uint8_t[]> sourcedata(new uint8_t[size]);
	in.read(reinterpret_cast<char*>(sourcedata.get()), size);

	int x = 0;
	int y = 0;
	int comp = 0;
	data = (char*)stbi_load_from_memory(sourcedata.get(), int(size), &x, &y, &comp, 0);
	sizeX = x;
	sizeY = y;
	hasAlpha = (comp == 4);
}

void Image::unload()
{
	stbi_image_free(data);
	data = nullptr;
}

