
#include <rynx/graphics/renderer/textrenderer.hpp>
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

float rynx::graphics::renderable_text::alignmentOffset() const {
	float textWidth = m_font->getLength(m_str, m_textHeight);
	switch (m_align) {
	case align::Center: return -textWidth * 0.5f;
	case align::Left: return 0;
	case align::Right: return -textWidth;
	}
	rynx_assert(false, "unreachable code");
}

rynx::vec3f rynx::graphics::renderable_text::position(int32_t cursor_pos) const {
	float dx = m_font->getLength(std::string_view(m_str.data(), cursor_pos), m_textHeight);
	return m_pos + rynx::vec3f{ alignmentOffset() + dx, 0, 0 };
}


rynx::graphics::text_renderer::text_renderer(std::shared_ptr<rynx::graphics::GPUTextures> textures, std::shared_ptr<rynx::graphics::shaders> shaders) :
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

void rynx::graphics::text_renderer::drawText(const rynx::graphics::renderable_text& text_line) {
	activeColor = text_line.color();
	std::string_view text = text_line.text();
	rynx_assert(text.length() < MAX_TEXT_LENGTH, "too long text to render!");
	if (text.length() >= MAX_TEXT_LENGTH) {
		return;
	}

	m_textures->bindTexture(0, text_line.font().getTextureID());
	int length = fillTextBuffers(text_line);
	drawTextBuffers(length);
}

void rynx::graphics::text_renderer::drawTextBuffers(int textLength) {
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

int rynx::graphics::text_renderer::fillTextBuffers(const rynx::graphics::renderable_text& line) {
	std::string_view text = line.text();
	float scaleY = line.font_size();
	float scaleX = line.font_size();

	const Font& font = line.font();
	
	int skippedCharacters = 0;
	int length = int(text.length());
	float textHeight = scaleY;
	
	float x = line.pos().x;
	float y = line.pos().y;

	x += line.alignmentOffset();
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


void rynx::graphics::text_renderer::fillTextureCoordinates(const Font& font, char c) {
	floats4 textureCoordinates = font.getTextureCoordinates(c);

	m_texCoordBuffer.push_back(textureCoordinates.x); m_texCoordBuffer.push_back(textureCoordinates.z);
	m_texCoordBuffer.push_back(textureCoordinates.y); m_texCoordBuffer.push_back(textureCoordinates.z);
	m_texCoordBuffer.push_back(textureCoordinates.y); m_texCoordBuffer.push_back(textureCoordinates.w);

	m_texCoordBuffer.push_back(textureCoordinates.y); m_texCoordBuffer.push_back(textureCoordinates.w);
	m_texCoordBuffer.push_back(textureCoordinates.x); m_texCoordBuffer.push_back(textureCoordinates.w);
	m_texCoordBuffer.push_back(textureCoordinates.x); m_texCoordBuffer.push_back(textureCoordinates.z);
}

void rynx::graphics::text_renderer::fillColorBuffer(const rynx::floats4& activeColor_) {
	for (int i = 0; i < 6; ++i) {
		m_colorBuffer.push_back(activeColor_.x);
		m_colorBuffer.push_back(activeColor_.y);
		m_colorBuffer.push_back(activeColor_.z);
		m_colorBuffer.push_back(activeColor_.w);
	}
}

void rynx::graphics::text_renderer::fillCoordinates(float x, float y, float charWidth, float charHeight) {
	m_vertexBuffer.push_back(x); m_vertexBuffer.push_back(y);
	m_vertexBuffer.push_back(x + charWidth); m_vertexBuffer.push_back(y);
	m_vertexBuffer.push_back(x + charWidth); m_vertexBuffer.push_back(y + charHeight);

	m_vertexBuffer.push_back(x + charWidth); m_vertexBuffer.push_back(y + charHeight);
	m_vertexBuffer.push_back(x); m_vertexBuffer.push_back(y + charHeight);
	m_vertexBuffer.push_back(x); m_vertexBuffer.push_back(y);
}