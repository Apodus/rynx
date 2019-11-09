
#pragma once

#include <rynx/tech/math/vector.hpp>
#include <rynx/tech/math/matrix.hpp>
#include <rynx/graphics/color.hpp>
#include <rynx/graphics/text/font.hpp>

#include <memory>
#include <functional>

typedef int GLint;
typedef unsigned GLuint;

class GPUTextures;
class Camera;
class Shaders;
class Font;


class TextRenderer {

	static constexpr int MAX_TEXT_LENGTH = 1024;

	std::vector<float> m_vertexBuffer;
	std::vector<float> m_texCoordBuffer;
	std::vector<float> m_colorBuffer;

	matrix4 projectionViewMatrix;

	floats4 activeColor;

	std::shared_ptr<GPUTextures> m_textures;
	std::shared_ptr<Shaders> m_shaders;
	std::shared_ptr<Camera> m_pCamera;
	GLuint vao;
	GLuint vbo, cbo, tbo;

public:

	enum class Align {
		Left,
		Center,
		Right
	};

	TextRenderer(std::shared_ptr<GPUTextures> textures, std::shared_ptr<Shaders> shaders);

	void setCamera(std::shared_ptr<Camera> pCamera) { m_pCamera = std::move(pCamera); }
  void drawText(const std::string& text, float x, float y, float lineHeight, const Font& font, std::function<void(const std::string&, int, float&, float&, float&, float&, floats4&)> custom = [](const std::string&, int, float&, float&, float&, float&, floats4&){});
  void drawText(const std::string& text, float x, float y, float lineHeight, floats4 color, const Font& font, std::function<void(const std::string&, int, float&, float&, float&, float&, floats4&)> custom = [](const std::string&, int, float&, float&, float&, float&, floats4&){});
  void drawText(const std::string& text, float x, float y, float lineHeight, floats4 color, Align align, const Font& font, std::function<void(const std::string&, int, float&, float&, float&, float&, floats4&)> custom = [](const std::string&, int, float&, float&, float&, float&, floats4&){});

	void drawTextBuffers(int textLength);
  int fillTextBuffers(const std::string& text, const Font& font, float x, float y, float scaleX, float scaleY, Align align, std::function<void(const std::string&, int, float&, float&, float&, float&, floats4&)> custom = [](const std::string&, int, float&, float&, float&, float&, floats4&){});

	void fillColorBuffer(const floats4& activeColor_) {
		for (int i = 0; i < 6; ++i) {
			m_colorBuffer.push_back(activeColor_[0]);
			m_colorBuffer.push_back(activeColor_[1]);
			m_colorBuffer.push_back(activeColor_[2]);
			m_colorBuffer.push_back(activeColor_[3]);
		}
	}

	void fillCoordinates(float x, float y, float charWidth, float charHeight) {
		m_vertexBuffer.push_back(x); m_vertexBuffer.push_back(y);
		m_vertexBuffer.push_back(x + charWidth); m_vertexBuffer.push_back(y);
		m_vertexBuffer.push_back(x + charWidth); m_vertexBuffer.push_back(y + charHeight);

		m_vertexBuffer.push_back(x + charWidth); m_vertexBuffer.push_back(y + charHeight);
		m_vertexBuffer.push_back(x); m_vertexBuffer.push_back(y + charHeight);
		m_vertexBuffer.push_back(x); m_vertexBuffer.push_back(y);
	}

	void fillTextureCoordinates(const Font& font, char c);
};
