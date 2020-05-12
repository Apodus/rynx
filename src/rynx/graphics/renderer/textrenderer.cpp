
#include "textrenderer.hpp"
#include <rynx/graphics/camera/camera.hpp>
#include <rynx/graphics/shader/shaders.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/text/font.hpp>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

rynx::floats4 getColorByCode(char c) {
	switch (c) {
	case 'w': return Color::WHITE;
	case 's': return Color::BLACK;
	case 'g': return Color::GREEN;
	case 'r': return Color::RED;
	case 'b': return Color::BLUE;
	case 'c': return Color::CYAN;
	case 'y': return Color::YELLOW;
	case 'o': return Color::ORANGE;
	case 'R': return Color::DARK_RED;
	case 'x': return Color::GREY;
	default:
		return Color::WHITE;
	}
}

void getColorByCode(char c, rynx::floats4& color) {
	color = getColorByCode(c);
}

rynx::TextRenderer::TextRenderer(std::shared_ptr<GPUTextures> textures, std::shared_ptr<rynx::graphics::shaders> shaders) :
	m_textures(textures),
	m_shaders(shaders)
{
	auto fontShader = m_shaders->load_shader("font", "../shaders/font_130.vs.glsl", "../shaders/font_130.fs.glsl");
	m_shaders->activate_shader("font");

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	int vposIndex = fontShader->attribute("vertexPosition");
	int vcolIndex = fontShader->attribute("vertexColor");
	int vtexIndex = fontShader->attribute("textureCoordinate");

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 6 * 2 * MAX_TEXT_LENGTH * sizeof(float), NULL, GL_STREAM_DRAW);
	glVertexAttribPointer(vposIndex, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vposIndex);

	glGenBuffers(1, &cbo);
	glBindBuffer(GL_ARRAY_BUFFER, cbo);
	glBufferData(GL_ARRAY_BUFFER, 6 * 4 * MAX_TEXT_LENGTH * sizeof(float), NULL, GL_STREAM_DRAW); // 4 floats per element.
	glVertexAttribPointer(vcolIndex, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vcolIndex);

	glGenBuffers(1, &tbo);
	glBindBuffer(GL_ARRAY_BUFFER, tbo);
	glBufferData(GL_ARRAY_BUFFER, 6 * 2 * MAX_TEXT_LENGTH * sizeof(float), NULL, GL_STREAM_DRAW);
	glVertexAttribPointer(vtexIndex, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(vtexIndex);

	glBindVertexArray(0);
}

void rynx::TextRenderer::drawText(const std::string& text, float x, float y, float lineHeight, const Font& font, std::function<void(const std::string&, int, float&, float&, float&, float&, floats4&)> custom) {
	drawText(text, x, y, lineHeight, Color::WHITE, font, custom);
}

void rynx::TextRenderer::drawText(const std::string& text, float x, float y, float lineHeight, floats4 color, const Font& font, std::function<void(const std::string&, int, float&, float&, float&, float&, floats4&)> custom) {
	drawText(text, x, y, lineHeight, color, Align::Center, font, custom);
}

void rynx::TextRenderer::drawText(const std::string& text, float x, float y, float lineHeight, floats4 color, Align align, const Font& font, std::function<void(const std::string&, int, float&, float&, float&, float&, floats4&)> custom) {
	activeColor = color;
	rynx_assert(text.length() < MAX_TEXT_LENGTH, "too long text to render!");
	if (text.length() >= MAX_TEXT_LENGTH) {
		return;
	}

	m_textures->bindTexture(0, font.getTextureID());
	int length = fillTextBuffers(text, font, x, y, lineHeight, lineHeight, align, custom);
	drawTextBuffers(length);
}

void rynx::TextRenderer::drawTextBuffers(int textLength) {
	if (textLength == 0)
		return;

	std::shared_ptr<rynx::graphics::shader> textShader = m_shaders->activate_shader("font");
	const matrix4& projectionMatrix = m_pCamera->getProjection();
	const matrix4& viewMatrix = m_pCamera->getView();

	projectionViewMatrix.storeMultiply(projectionMatrix, viewMatrix);
	textShader->uniform("MVPMatrix", projectionViewMatrix);
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * m_vertexBuffer.size(), m_vertexBuffer.data());

	glBindBuffer(GL_ARRAY_BUFFER, cbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * m_colorBuffer.size(), m_colorBuffer.data());

	glBindBuffer(GL_ARRAY_BUFFER, tbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * m_texCoordBuffer.size(), m_texCoordBuffer.data());

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 2 * 3 * textLength);
	glBindVertexArray(0);

	m_vertexBuffer.clear();
	m_texCoordBuffer.clear();
	m_colorBuffer.clear();
}

int rynx::TextRenderer::fillTextBuffers(const std::string& text, const Font& font, float x, float y, float scaleX, float scaleY, Align align, std::function<void(const std::string&, int, float&, float&, float&, float&, floats4&)> custom) {
	int skippedCharacters = 0;
	int length = int(text.length());
	float textHeight = scaleY;
	float textWidth = font.getLength(text, scaleY);

	switch (align) {
	case Align::Center:
		x -= textWidth * 0.5f;
		break;
	case Align::Left:
		break;
	case Align::Right:
		x -= textWidth;
		break;
	}
	y -= textHeight * 0.5f;

	for (int i = 0; i < length; ++i) {
		char c = text[i];
		if (c == '^') {
			++i;
			skippedCharacters += 2;
			c = text[i];
			getColorByCode(c, activeColor);
			continue;
		}

		custom(text, i, x, y, scaleX, scaleY, activeColor);

		float width = font.width(c, scaleX);
		float height = font.height(c, scaleY);
		float advance = font.advance(c, scaleX);
		float offsetX = font.offsetX(c, scaleX);
		float offsetY = font.offsetY(c, scaleY);

		fillCoordinates(x + offsetX, y + offsetY, width, height);
		fillColorBuffer(activeColor);
		fillTextureCoordinates(font, c);

		// TODO: Add kerning to advance here.

		x += advance;
	}

	return length - skippedCharacters;
}


void rynx::TextRenderer::fillTextureCoordinates(const Font& font, char c) {
	floats4 textureCoordinates = font.getTextureCoordinates(c);

	m_texCoordBuffer.push_back(textureCoordinates.left); m_texCoordBuffer.push_back(textureCoordinates.top);
	m_texCoordBuffer.push_back(textureCoordinates.right); m_texCoordBuffer.push_back(textureCoordinates.top);
	m_texCoordBuffer.push_back(textureCoordinates.right); m_texCoordBuffer.push_back(textureCoordinates.bottom);

	m_texCoordBuffer.push_back(textureCoordinates.right); m_texCoordBuffer.push_back(textureCoordinates.bottom);
	m_texCoordBuffer.push_back(textureCoordinates.left); m_texCoordBuffer.push_back(textureCoordinates.bottom);
	m_texCoordBuffer.push_back(textureCoordinates.left); m_texCoordBuffer.push_back(textureCoordinates.top);
}