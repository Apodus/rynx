
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/internal/includes.hpp>
#include <rynx/system/assert.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>


void rynx::graphics::GPUTextures::dynamic_texture_atlas::init(rynx::graphics::GPUTextures* textures) {
	m_textures = textures;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_atlas_size);
	logmsg("Max texture size: %d", m_atlas_size);

	m_atlas_size >>= 1;
	m_slots_per_row = m_atlas_size / blockSize();
	m_atlas_tex_id = textures->createTexture("dynamic-atlas", m_atlas_size, m_atlas_size);
	m_indirection_buffer_tex_id = textures->createBuffer("uv-indirection");
	
	// first index is reserved for blank white.
	Image blankWhite(1, 1);
	blankWhite.fill(rynx::floats4{ 1, 1, 1, 1 });
	reserveSlot(rynx::graphics::texture_id(), 0, 0, 1);
	loadImage(rynx::graphics::texture_id(), std::move(blankWhite));
}

std::pair<int32_t, int32_t> rynx::graphics::GPUTextures::dynamic_texture_atlas::findFreeSlot(int32_t slotSize) {
	for (int y = 0; y <= m_slots_per_row - slotSize; y+= slotSize) {
		for (int x = 0; x <= m_slots_per_row - slotSize; x+= slotSize) {
			bool accepted = true;
			for (int xmod = 0; xmod < slotSize; ++xmod) {
				for (int ymod = 0; ymod < slotSize; ++ymod) {
					accepted &= is_slot_free(x+xmod, y + ymod);
				}
			}
			if (accepted)
				return {x, y};
		}
	}

	return {-1, -1};
}

void rynx::graphics::GPUTextures::dynamic_texture_atlas::reserveSlot(rynx::graphics::texture_id id, int x, int y, int slotSize) {
	for (int xmod = 0; xmod < slotSize; ++xmod) {
		for (int ymod = 0; ymod < slotSize; ++ymod) {
			m_tex_presence.set((x + xmod) + (y + ymod) * m_slots_per_row);
		}
	}

	if (id.value >= m_slots.size()) {
		m_slots.resize((id.value + 1) * 2);
	}

	m_slots[id.value].slotSize = slotSize;
	m_slots[id.value].slot = x + y * m_slots_per_row;
}

void rynx::graphics::GPUTextures::dynamic_texture_atlas::evictTexture(rynx::graphics::texture_id id) {
	int32_t slotSize = m_slots[id.value].slotSize;
	int x = m_slots[id.value].slot % m_slots_per_row;
	int y = m_slots[id.value].slot / m_slots_per_row;
	for (int32_t i = x; i < x + slotSize; ++i) {
		for (int32_t k = y; k < y + slotSize; ++k) {
			m_tex_presence.reset((x+i) + (y+k) * m_slots_per_row);
		}
	}

	m_slots[id.value] = atlas_tex_info();
}

void rynx::graphics::GPUTextures::dynamic_texture_atlas::loadImage(rynx::graphics::texture_id id, Image&& img, int slot_x, int slot_y, int slotSize) {
	this->reserveSlot(id, slot_x, slot_y, slotSize);
	glTextureSubImage2D(
		m_textures->getGLID(m_atlas_tex_id),
		0, /*mip level 0*/
		slot_x * atlasBlockSize, // x pos
		slot_y * atlasBlockSize, // y pos
		atlasBlockSize * slotSize, // width
		atlasBlockSize * slotSize, // height
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		img.data
	);

	m_textures->bufferData(m_indirection_buffer_tex_id, m_slots.size() * 8, m_slots.data());

	auto verifyNoGlErrors = []() {
		auto error = glGetError();
		rynx_assert(error == GL_NO_ERROR, "oh shit");
		return error == GL_NO_ERROR;
	};

	verifyNoGlErrors();
	img.unload();
}

void rynx::graphics::GPUTextures::dynamic_texture_atlas::loadImage(rynx::graphics::texture_id id, Image&& img) {
	int32_t slotSize = 1; // TODO: select based on source image size
	img.rescale(atlasBlockSize * slotSize, atlasBlockSize * slotSize);

	auto [x, y] = this->findFreeSlot(slotSize);
	if (x == -1) {
		// TODO: Block not found - need to evict something
	}

	loadImage(id, std::move(img), x, y, slotSize);
}

void rynx::graphics::GPUTextures::dynamic_texture_atlas::loadImage(rynx::graphics::texture_id id, const tex_info& info) {
	
	if (id.value >= m_slots.size()) {
		m_slots.resize((id.value + 1) * 2);
	}

	// if already loaded in atlas, don't do anything.
	if (m_slots[id.value].slot > 0) {
		return;
	}
	
	Image img;
	logmsg("Loading texture '%s' from file '%s'", info.name.c_str(), info.filepath.c_str());
	img.loadImage(info.filepath);
	loadImage(id, std::move(img));
}






rynx::graphics::GPUTextures::GPUTextures() {
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
	m_texture_atlas.init(this);

	for (;;) {
		auto errorValue = glGetError();
		if (errorValue == GL_NO_ERROR)
			break;
		rynx_assert(false, "gl error value: %d", errorValue);
	}
}

rynx::graphics::GPUTextures::~GPUTextures() {
	deleteAllTextures();
}

rynx::graphics::texture_id rynx::graphics::GPUTextures::getCurrentTexture(size_t texture_unit) const {
	rynx_assert(texture_unit < current_textures.size(), "Texture unit out of bounds! Requested: %u, size %u", static_cast<unsigned>(texture_unit), static_cast<unsigned>(current_textures.size()));
	return current_textures[texture_unit];
}

void rynx::graphics::GPUTextures::loadTexturesFromPath(const std::string& path) {
	for (auto& p : std::filesystem::recursive_directory_iterator(path)) {
		if (p.is_regular_file()) {
			std::string path_string = p.path().string();
			if (path_string.ends_with(".png")) {
				std::string filename = path_string;
				filename.pop_back();
				filename.pop_back();
				filename.pop_back();
				filename.pop_back();
				auto pos1 = filename.find_last_of("/");
				auto pos2 = filename.find_last_of("\\");
				auto pos = std::min(pos1, pos2);
				if (pos1 != std::string::npos && pos2 != std::string::npos) {
					pos = std::max(pos1, pos2);
				}

				if (pos != std::string::npos)
					filename = filename.substr(pos + 1);
				loadToTextureAtlas(filename, path_string);
			}
		}
	}
}

rynx::graphics::texture_id rynx::graphics::GPUTextures::loadToTextureAtlas(const std::string& name, const std::string& filename) {
	rynx_assert(!name.empty(), "create texture called with empty name");

	rynx::graphics::texture_id id = generate_tex_id();
	m_textures[id].name = name;
	m_textures[id].filepath = filename;
	m_textures[id].in_atlas = true;
	m_texture_atlas.loadImage(id, m_textures[id]);
	return id;
}

unsigned rynx::graphics::GPUTextures::getGLID(texture_id id) const {
	// rynx::graphics::texture_id real_id = m_atlasHandler.getRealTextureID(id);
	auto it = m_textures.find(id);
	return it->second.textureGL_ID;
}

void buildDebugMipmaps(size_t x, size_t y) {
	rynx_assert(x == y, "debug mipmaps don't work with non-square textures");

	struct Color {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	};

	int lod = 0;
	do {
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

rynx::graphics::texture_id rynx::graphics::GPUTextures::generate_tex_id() {
	rynx::graphics::texture_id id = {++m_next_id};
	m_textures.emplace(id, tex_info());
	return id;
}


rynx::graphics::texture_id rynx::graphics::GPUTextures::createFloatTexture(const std::string& name, int width, int height) {
	rynx_assert(!name.empty(), "create float texture called with empty name");

	rynx::graphics::texture_id id = generate_tex_id();
	m_textures[id].name = name;

	glGenTextures(1, &(m_textures[id].textureGL_ID));
	glBindTexture(GL_TEXTURE_2D, m_textures[id].textureGL_ID);
	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR); // scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR); // scale linearly when image smalled than texture
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	int value;
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &value);
	return id;
}

rynx::graphics::texture_id rynx::graphics::GPUTextures::createTexture(const std::string& name, int width, int height) {
	rynx_assert(!name.empty(), "create texture called with empty name");
	rynx::graphics::texture_id id = generate_tex_id();
	m_textures[id].name = name;

	glGenTextures(1, &(m_textures[id].textureGL_ID));
	glBindTexture(GL_TEXTURE_2D, m_textures[id].textureGL_ID);
	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	m_textures[id].textureSize = rynx::vec4<int32_t>(width, height, 0, 0);
	return id;
}

rynx::graphics::texture_id rynx::graphics::GPUTextures::createBuffer(const std::string& name) {
	rynx_assert(!name.empty(), "create texture called with empty name");
	rynx::graphics::texture_id id = generate_tex_id();
	m_textures[id].name = name;

	glGenTextures(1, &(m_textures[id].textureGL_ID));
	glGenBuffers(1, &(m_textures[id].bufferGL_ID));
	glBindBuffer(GL_TEXTURE_BUFFER, m_textures[id].bufferGL_ID);
	glBufferData(GL_TEXTURE_BUFFER, 1024 * 1024, nullptr, GL_STATIC_DRAW);
	return id;
}

void rynx::graphics::GPUTextures::bufferData(rynx::graphics::texture_id id, size_t bytes, void* data) {
	rynx_assert(m_textures[id].has_buffer(), "bufferData can only be called for buffer textures");
	glBindBuffer(GL_TEXTURE_BUFFER, m_textures[id].bufferGL_ID);
	glBufferSubData(GL_TEXTURE_BUFFER, 0, bytes, data);

	m_textures[id].buffer_gl_format = GL_RG32I; // TODO: Some control over this would be great.
}

rynx::graphics::texture_id rynx::graphics::GPUTextures::createDepthTexture(const std::string& name, int width, int height, int bits_per_pixel) {
	rynx_assert(!name.empty(), "create depth texture called with empty name");

	rynx::graphics::texture_id id = generate_tex_id();
	m_textures[id].name = name;

	glGenTextures(1, &(m_textures[id].textureGL_ID));
	glBindTexture(GL_TEXTURE_2D, m_textures[id].textureGL_ID);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); // scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); // scale linearly when image smalled than texture
	
	rynx_assert(bits_per_pixel == 16 || bits_per_pixel == 32, "pixel format not supported by this code.");

	int depth_tex_internal_format = GL_DEPTH_COMPONENT16;
	int depth_tex_storage_format = GL_UNSIGNED_SHORT;
	if (bits_per_pixel == 16) {
		depth_tex_internal_format = GL_DEPTH_COMPONENT16;
		depth_tex_storage_format = GL_UNSIGNED_SHORT;
	}
	if (bits_per_pixel == 32) {
		depth_tex_internal_format = GL_DEPTH_COMPONENT32;
		depth_tex_storage_format = GL_UNSIGNED_INT;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, depth_tex_internal_format, width, height, 0, GL_DEPTH_COMPONENT, depth_tex_storage_format, nullptr);
	return id;
}

rynx::graphics::texture_id rynx::graphics::GPUTextures::createTexture(const std::string& name, Image& img) {
	rynx_assert(!name.empty(), "create texture called with an empty name.");
	rynx_assert(img.data, "createTexture called with nullptr img data.");

	if(textureExists(name)) {
		logmsg("Texture for \"%s\" is already loaded!", name.c_str());
	}

	rynx::graphics::texture_id id = generate_tex_id();
	m_textures[id].name = name;

	glGenTextures(1, &(m_textures[id].textureGL_ID));
	glBindTexture(GL_TEXTURE_2D, m_textures[id].textureGL_ID);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // scale linearly when image bigger than texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// 2d texture, level of detail 0 (normal), 3 components (red, green, blue), x size from image, y size from image, 
	// border 0 (normal), rgb color data, unsigned byte data, and finally the data itself.
	if(img.hasAlpha) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.sizeX, img.sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data);
	}
	else {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.sizeX, img.sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, img.data);
	}
	
	glGenerateMipmap(GL_TEXTURE_2D);

	m_textures[id].textureSize = rynx::vec4<int32_t>(img.sizeX, img.sizeY, 0, 0);
	img.unload();
	return id;
}


void rynx::graphics::GPUTextures::deleteTexture(rynx::graphics::texture_id id) {
	rynx_assert(id.value, "deleting texture with an empty name?");
	auto it = m_textures.find(id);
	if(it != m_textures.end()) {
		for (size_t i = 0; i < current_textures.size(); ++i) {
			auto tex = current_textures[i];
			if (tex == it->first) {
				unbindTexture(i);
			}
		}

		glDeleteTextures(1, &it->second.textureGL_ID);
		m_textures.erase(it);
	}
	else {
		logmsg("WARNING: tried to delete a texture that doesnt exist");
	}
}

void rynx::graphics::GPUTextures::unbindTexture(size_t texture_unit) {
	current_textures[texture_unit].value = 0;
}

int rynx::graphics::GPUTextures::bindTexture(size_t texture_unit, texture_id id) {
	rynx_assert(texture_unit < current_textures.size(), "texture unit out of bounds: %d >= %d", static_cast<int>(texture_unit), static_cast<int>(current_textures.size()));
	if(current_textures[texture_unit] == id)
		return 1;

	// zero texture id is special. it is always the empty texture in atlas.
	if (id.value == 0) {
		return bindTexture(texture_unit, m_texture_atlas.getAtlasTexture());
	}

	if(textureExists(id)) {
		glActiveTexture(GL_TEXTURE0 + int(texture_unit));
		auto it = m_textures.find(id);
		if (it->second.in_atlas) {
			return bindTexture(texture_unit, m_texture_atlas.getAtlasTexture());
		}
		else {
			current_textures[texture_unit] = id;
			if (!it->second.has_buffer()) {
				glBindTexture(GL_TEXTURE_2D, it->second.textureGL_ID);
				rynx_assert(glGetError() == GL_NO_ERROR, "gl error :(");
			}
			else {
				glBindTexture(GL_TEXTURE_BUFFER, it->second.textureGL_ID);
				glTexBuffer(GL_TEXTURE_BUFFER, it->second.buffer_gl_format, it->second.bufferGL_ID);
				rynx_assert(glGetError() == GL_NO_ERROR, "gl error :(");
			}
			return 1;
		}
	}
	else {
		logmsg("Trying to bind to a texture that does not exist");
		rynx_assert(!m_textures.empty(), "please load at least one texture...");

		// just bind a random texture, sometimes this is better than crash.
		bindTexture(texture_unit, m_textures.begin()->first);
		return 0;
	}
}

void rynx::graphics::GPUTextures::deleteAllTextures() {
	for(auto&& entry : m_textures) {
		glDeleteTextures(1, &entry.second.textureGL_ID);
	}
	m_textures.clear();
}

rynx::graphics::texture_id rynx::graphics::GPUTextures::findTextureByName(std::string_view name) const {
	for (auto&& entry : m_textures)
		if (entry.second.name == name)
			return entry.first;
	logmsg("WARNING: finding texture by name '%s' by no such texture exists", name.data());
	return {};
}

bool rynx::graphics::GPUTextures::textureExists(std::string name) const {
	for (auto&& entry : m_textures)
		if (entry.second.name == name)
			return true;
	return false;
}

bool rynx::graphics::GPUTextures::textureExists(texture_id id) const {
	return m_textures.find(id) != m_textures.end();
}

