
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/opengl.hpp>
#include <rynx/system/assert.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

GPUTextures::GPUTextures()
{
	int max_texture_units = 0;
#ifdef _WIN32
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &max_texture_units);
	logmsg("Max texture units: %d", max_texture_units);
	if(max_texture_units == 0)
		max_texture_units = 4; // pray
#else
	max_texture_units = 4; // make a wild guess :DD
#endif
	current_textures.resize(max_texture_units);
}

GPUTextures::~GPUTextures()
{
	deleteAllTextures();
}

const std::string& GPUTextures::getCurrentTexture(size_t texture_unit) const
{
	rynx_assert(texture_unit < current_textures.size(), "Texture unit out of bounds! Requested: %u, size %u", static_cast<unsigned>(texture_unit), static_cast<unsigned>(current_textures.size()));
	return current_textures[texture_unit];
}

void GPUTextures::createTextures(const std::string& filename)
{
	std::ifstream in(filename.c_str());

	std::string line;
	while(getline(in, line))
	{
		if(line.empty() || line[0] == '#')
		{
			continue;
		}
		std::stringstream ss(line);
		std::string name;
		std::string file;
		ss >> name >> file;
		if(name == "atlas") {
			std::string atlasFileName;
			ss >> atlasFileName;
			TextureAtlas atlas(file, textureSizes[file].x, textureSizes[file].y);
			std::ifstream atlasIn(atlasFileName);
			
			int maxx = 1;
			int maxy = 1;
			while(getline(atlasIn, line))
			{
				if(line.empty() || line[0] == '#')
				{
					continue;
				}

				std::stringstream atlasSS(line);
				
				if (line[0] == '-') {
					atlasSS >> name >> maxx >> maxy;
				}
				else {
					int x, y;
					atlasSS >> x >> y >> name;
					atlas.insertTexture(name, x-1, y-1, maxx, maxy);
				}
			}

			insertAtlas(atlas);
		}
		else {
			createTexture(name, file);
		}
	}
}

unsigned GPUTextures::createTexture(const std::string& name, const std::string& filename)
{
	rynx_assert(!name.empty(), "create texture called with empty name");

	Image img;
	logmsg("Loading texture '%s' from file '%s'", name.c_str(), filename.c_str());
	img.loadImage(filename);
	
	if((img.sizeX == 0) || (img.sizeY == 0))
	{
		logmsg("Failed to load texture '%s' from file '%s'. File doesn't exist?", name.c_str(), filename.c_str());
		return 0;
	}
	
	logmsg("Creating %dx%d texture '%s'", static_cast<int>(img.sizeX), static_cast<int>(img.sizeY), name.c_str());
	return createTexture(name, img);
}


unsigned GPUTextures::getTextureID(const std::string& name) const
{
	rynx_assert(!name.empty(), "get texture called with an empty name!");
	std::string realName = m_atlasHandler.getRealTextureID(name);
	auto it = textures.find(realName);
	rynx_assert(it != textures.end(), "texture for '%s' not found! (real texture name: '%s')", name.c_str(), realName.c_str());
	return it->second;
}

namespace
{
	struct Color
	{
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	};
}

void buildDebugMipmaps(size_t x, size_t y)
{
	rynx_assert(x == y, "debug mipmaps don't work with non-square textures");

	int lod = 0;
	do
	{
		size_t bit = 1;
		while(bit < x)
			bit <<= 1;

		uint8_t r = static_cast<uint8_t>(255 - bit * 255 / 8);
		uint8_t g = static_cast<uint8_t>(bit * 255 / 8);
		uint8_t b = 0;
		Color color = { r, g, b, 255 };

		std::vector<Color> data(x*y, color);
		glTexImage2D(GL_TEXTURE_2D, lod++, GL_RGBA, int(x), int(y), 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

		x /= 2;
		y /= 2;
	}
	while(x != 0 && y != 0);
}


unsigned GPUTextures::createFloatTexture(const std::string& name, int width, int height)
{
	rynx_assert(!name.empty(), "create float texture called with empty name");

	glGenTextures(1, &(textures[name]));
	glBindTexture(GL_TEXTURE_2D, textures[name]);
	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR); // scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR); // scale linearly when image smalled than texture
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	int value;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &value);
	return textures[name];
}

unsigned GPUTextures::createTexture(const std::string& name, int width, int height)
{
	rynx_assert(!name.empty(), "create texture called with empty name");

	glGenTextures(1, &(textures[name]));
	glBindTexture(GL_TEXTURE_2D, textures[name]);
	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	textureSizes[name] = vec4<short>(short(width), short(height), 0, 0);
	return textures[name];
}

unsigned GPUTextures::createDepthTexture(const std::string& name, int width, int height)
{
	rynx_assert(!name.empty(), "create depth texture called with empty name");

	glGenTextures(1, &(textures[name]));
	glBindTexture(GL_TEXTURE_2D, textures[name]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture

#ifndef _WIN32
	// not really sure if this is 100% necessary anyway..
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE, GL_NONE);
#endif

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);

	int value;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &value);
	return textures[name];
}

unsigned GPUTextures::createTexture(const std::string& name, Image& img)
{
	rynx_assert(!name.empty(), "create texture called with an empty name.");
	rynx_assert(img.data, "createTexture called with nullptr img data.");
	
	if(textureExists(name))
	{
		logmsg("Texture for \"%s\" is already loaded!", name.c_str());
		return textures[name];
	}

	glGenTextures(1, &(textures[name]));
	glBindTexture(GL_TEXTURE_2D, textures[name]);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image, 
	// border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.
	if(img.hasAlpha)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.sizeX, img.sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.sizeX, img.sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
	}
	
	glGenerateMipmap(GL_TEXTURE_2D);

	textureSizes[name] = vec4<short>(static_cast<short>(img.sizeX), static_cast<short>(img.sizeY), 0, 0);
	img.unload();
	
	return textures[name];
}


void GPUTextures::deleteTexture(const std::string& name)
{
	rynx_assert(!name.empty(), "deleting texture with an empty name?");
	std::string realName = m_atlasHandler.getRealTextureID(name);

	if(textureExists(realName))
	{
		glDeleteTextures(1, &textures[realName]);
		textures.erase(realName);
	}
	else
	{
		logmsg("WARNING: tried to delete a texture that doesnt exist: \"%s\"", name.c_str());
	}
}


void GPUTextures::unbindTexture(size_t texture_unit)
{
	glActiveTexture(GL_TEXTURE0 + int(texture_unit));
	glDisable(GL_TEXTURE_2D);
	current_textures[texture_unit] = "";
}

int GPUTextures::bindTexture(size_t texture_unit, const std::string& name)
{
	rynx_assert(texture_unit < current_textures.size(), "texture unit out of bounds: %d >= %d", static_cast<int>(texture_unit), static_cast<int>(current_textures.size()));
	rynx_assert(!name.empty(), "empty name for bind texture");

	std::string realName = m_atlasHandler.getRealTextureID(name);

	if(current_textures[texture_unit] == realName)
		return 1;

	if(textureExists(realName))
	{
		glActiveTexture(GL_TEXTURE0 + int(texture_unit));
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textures[realName]);
		current_textures[texture_unit] = realName;
		return 1;
	}
	else
	{
		logmsg("Trying to bind to a texture that does not exist: \"%s\"", name.c_str());
		rynx_assert(!textures.empty(), "please load at least one texture...");
		std::string default_texture = textures.begin()->first;
		logmsg("Loading default texture '%s'", default_texture.c_str());
		rynx_assert(!default_texture.empty(), "failed to load default texture :(");
		textures[name] = textures[default_texture];
		bindTexture(texture_unit, default_texture);
		return 0;
	}
}

void GPUTextures::deleteAllTextures()
{
	for(auto iter = textures.begin(); iter != textures.end(); iter++)
	{
		glDeleteTextures(1, &iter->second);
	}
	textures.clear();
}

bool GPUTextures::textureExists(const std::string& name) const
{
	std::string realName = m_atlasHandler.getRealTextureID(name);
	return textures.find(name) != textures.end();
}

floats4 GPUTextures::textureLimits(const std::string& name) const {
	return m_atlasHandler.getTextureCoordinateLimits(name);
}

floats4 GPUTextures::textureLimits(const std::string& name, vec4<float> uvLimits) const {
	return m_atlasHandler.getTextureCoordinateLimits(name, uvLimits);
}

void GPUTextures::insertAtlas(const TextureAtlas& atlas) {
	m_atlasHandler.addTextureAtlas(atlas);
}

