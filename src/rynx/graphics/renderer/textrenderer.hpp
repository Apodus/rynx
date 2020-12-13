
#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/graphics/color.hpp>
#include <rynx/graphics/text/font.hpp>

#include <memory>
#include <functional>

typedef int GLint;
typedef unsigned GLuint;

class Font;

namespace rynx {
	class camera;
	namespace graphics {
		class GPUTextures;
		class shaders;
	}
}

namespace rynx {
	namespace graphics {
		class renderable_text {
		public:
			enum class align {
				Left,
				Right,
				Center
			};

			renderable_text& text(std::string s) { m_str = std::move(s); return *this; }
			std::string& text() { return m_str; }
			renderable_text& align_left() { m_align = align::Left; return *this; }
			renderable_text& align_right() { m_align = align::Right; return *this; }
			renderable_text& align_center() { m_align = align::Center; return *this; }

			renderable_text& pos(rynx::vec3f v) { m_pos = v; return *this; }
			rynx::vec3f& pos() { return m_pos; }

			renderable_text& color(rynx::floats4 color) { m_color = color; return *this; }
			rynx::floats4& color() { return m_color; }

			renderable_text& font(const Font& font) { m_font = &font; return *this; }
			renderable_text& font_size(float v) { m_textHeight = v; return *this; }

			std::string_view text() const { return m_str; }
			rynx::vec3f pos() const { return m_pos; }
			rynx::floats4 color() const { return m_color; }
			float font_size() const { return m_textHeight; }
			const Font& font() const { return *m_font; }

			float alignmentOffset() const;
			rynx::vec3f position(int32_t cursor_pos) const;

			bool is_align_left() const { return m_align == align::Left; }
			bool is_align_right() const { return m_align == align::Right; }
			bool is_align_center() const { return m_align == align::Center; }

			operator bool() const { return !m_str.empty(); }

		private:
			std::string m_str;
			rynx::vec3f m_pos;
			rynx::floats4 m_color;
			renderable_text::align m_align;
			const Font* m_font = nullptr;
			float m_textHeight = 0.1f;
		};

		class text_renderer {

			static constexpr int MAX_TEXT_LENGTH = 1024;

			std::vector<float> m_vertexBuffer;
			std::vector<float> m_texCoordBuffer;
			std::vector<float> m_colorBuffer;

			matrix4 projectionViewMatrix;

			floats4 activeColor;

			std::shared_ptr<rynx::graphics::GPUTextures> m_textures;
			std::shared_ptr<rynx::graphics::shaders> m_shaders;
			std::shared_ptr<camera> m_pCamera;
			GLuint vao;
			GLuint vbo, cbo, tbo;

			// private implementation.
			int fillTextBuffers(const renderable_text& text);
			void drawTextBuffers(int textLength);
			void fillColorBuffer(const floats4& activeColor_);
			void fillCoordinates(float x, float y, float charWidth, float charHeight);
			void fillTextureCoordinates(const Font& font, char c);

		public:
			text_renderer(std::shared_ptr<rynx::graphics::GPUTextures> textures, std::shared_ptr<rynx::graphics::shaders> shaders);

			void setCamera(std::shared_ptr<camera> pCamera) { m_pCamera = std::move(pCamera); }
			void drawText(const renderable_text& text);
		};
	}
}