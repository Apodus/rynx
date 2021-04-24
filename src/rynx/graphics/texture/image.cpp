

#include "image.hpp"

#include <rynx/math/vector.hpp>
#include <rynx/tech/filesystem/filesystem.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <algorithm>
#include <cstdint>

Image::Image(const Image& other)
{
	this->operator=(other);
}

Image& Image::operator = (const Image& other)
{
	loadByteBufferRGBA(data, other.sizeX, other.sizeY);
	return *this;
}

void Image::loadImage(const std::string& filename)
{
	auto file_data = rynx::filesystem::read_file(filename);
	
	int x = 0;
	int y = 0;
	int comp = 0;
	data = (unsigned char*)stbi_load_from_memory(
		reinterpret_cast<stbi_uc const*>(file_data.data()),
		static_cast<int>(file_data.size()),
		&x, &y, &comp, 0);
	sizeX = x;
	sizeY = y;
	hasAlpha = (comp == 4);
}

void Image::unload()
{
	stbi_image_free(data);
	data = nullptr;
}

void Image::rescale(int newX, int newY) {
	rynx_assert(hasAlpha, "rescale currently only supports rgba, sorry.");
	
	struct color {
		unsigned char r, g, b, a;
	};

	color* color_view = reinterpret_cast<color*>(data);
	auto pixel = [this, color_view](int x, int y, int max_x, int max_y) {
		int actual_x = (x * sizeX) / max_x;
		int actual_y = (y * sizeY) / max_y;
		return color_view[int32_t(actual_x + actual_y * sizeX)];
	};

	color* color_new = static_cast<color*>(malloc(sizeof(color) * newX * newY));
	for (int i = 0; i < newX; ++i) {
		for (int k = 0; k < newY; ++k) {
			color_new[i + k * newX] = pixel(i, k, newX, newY);
		}
	}

	unload();

	sizeX = newX;
	sizeY = newY;
	hasAlpha = true;
	data = reinterpret_cast<unsigned char*>(color_new);
}

void Image::loadByteBufferRGBA(unsigned char* in_data, int width, int height) {
	unload();
	createEmptyRGBA(width, height);

	for (uint32_t pixel = 0; pixel < sizeX * sizeY; ++pixel) {
		data[4 * pixel + 0] = in_data[4 * pixel + 0];
		data[4 * pixel + 1] = in_data[4 * pixel + 1];
		data[4 * pixel + 2] = in_data[4 * pixel + 2];
		data[4 * pixel + 3] = in_data[4 * pixel + 3];
	}
}

void Image::loadFloatBufferRGBA(rynx::floats4* in_data, int width, int height) {
	unload();
	createEmptyRGBA(width, height);
	
	for (uint32_t pixel = 0; pixel < sizeX * sizeY; ++pixel) {
		rynx::floats4 color = in_data[pixel];
		data[4 * pixel + 0] = static_cast<unsigned char>(255 * std::clamp(color.x, 0.0f, 1.0f));
		data[4 * pixel + 1] = static_cast<unsigned char>(255 * std::clamp(color.y, 0.0f, 1.0f));
		data[4 * pixel + 2] = static_cast<unsigned char>(255 * std::clamp(color.z, 0.0f, 1.0f));
		data[4 * pixel + 3] = static_cast<unsigned char>(255 * std::clamp(color.w, 0.0f, 1.0f));
	}
}

void Image::createEmptyRGBA(int width, int height) {
	data = static_cast<unsigned char*>(malloc(width * height * 4));
	rynx_assert(data != nullptr, "memory alloc failed");
	memset(data, 0, width * height * 4);
	sizeX = width;
	sizeY = height;
	hasAlpha = true;
}

void Image::fill(rynx::floats4 color) {
	for (uint32_t pixel = 0; pixel < sizeX * sizeY; ++pixel) {
		data[4 * pixel + 0] = static_cast<unsigned char>(255 * std::clamp(color.x, 0.0f, 1.0f));
		data[4 * pixel + 1] = static_cast<unsigned char>(255 * std::clamp(color.y, 0.0f, 1.0f));
		data[4 * pixel + 2] = static_cast<unsigned char>(255 * std::clamp(color.z, 0.0f, 1.0f));
		data[4 * pixel + 3] = static_cast<unsigned char>(255 * std::clamp(color.w, 0.0f, 1.0f));
	}
}
