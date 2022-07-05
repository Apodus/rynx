
#pragma once

#include <rynx/math/vector.hpp>
#include <rynx/math/matrix.hpp>
#include <rynx/graphics/color.hpp>
#include <rynx/graphics/text/font.hpp>
#include <rynx/tech/std/memory.hpp>

#include <vector>
#include <string_view>

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

			struct orientation_t {
				rynx::vec3f up = {0.0f, 1.0f, 0.0f};
				rynx::vec3f forward = {0.0f, 0.0f, -1.0f};
			};

			renderable_text& text(rynx::string s) { m_str = std::move(s); return *this; }
			rynx::string& text() { return m_str; }
			renderable_text& align_left() { m_align = align::Left; return *this; }
			renderable_text& align_right() { m_align = align::Right; return *this; }
			renderable_text& align_center() { m_align = align::Center; return *this; }

			renderable_text& pos(rynx::vec3f v) { m_pos = v; return *this; }
			rynx::vec3f& pos() { return m_pos; }

			renderable_text& color(rynx::floats4 color) { m_color = color; return *this; }
			rynx::floats4& color() { return m_color; }

			renderable_text& font(const Font* font) { m_font = font; return *this; }
			renderable_text& font_size(float v) { m_textHeight = v; return *this; }

			orientation_t& orientation() { return m_orientation; }
			const orientation_t& orientation() const { return m_orientation; }

			rynx::string_view text() const { return m_str; }
			rynx::vec3f pos() const { return m_pos; }
			rynx::floats4 color() const { return m_color; }
			float font_size() const { return m_textHeight; }
			const Font* font() const { return m_font; }

			float alignmentOffset(const Font& font) const;
			rynx::vec3f position(const Font& font, int32_t cursor_pos) const;
			rynx::vec3f position(int32_t cursor_pos) const { return position(*m_font, cursor_pos); }
			
			bool is_align_left() const { return m_align == align::Left; }
			bool is_align_right() const { return m_align == align::Right; }
			bool is_align_center() const { return m_align == align::Center; }

			operator bool() const { return !m_str.empty(); }

		private:
			rynx::string m_str;
			rynx::vec3f m_pos;
			rynx::floats4 m_color = {1,1,1,1};
			renderable_text::align m_align = align::Center;
			const Font* m_font = nullptr;
			float m_textHeight = 0.1f;
			orientation_t m_orientation;
		};

		class text_renderer {

			static constexpr int MAX_TEXT_LENGTH = 1024;

			std::vector<float> m_vertexBuffer;
			std::vector<float> m_texCoordBuffer;
			std::vector<float> m_colorBuffer;

			matrix4 projectionViewMatrix;

			floats4 activeColor;

			rynx::shared_ptr<rynx::graphics::GPUTextures> m_textures;
			rynx::shared_ptr<rynx::graphics::shaders> m_shaders;
			rynx::observer_ptr<camera> m_pCamera;
			GLuint vao;
			GLuint vbo, cbo, tbo;

			Font m_defaultFont;

			// private implementation.
			int fillTextBuffers(const renderable_text& text, const Font& font);
			void drawTextBuffers(int textLength, rynx::graphics::texture_id tex_id);
			void fillColorBuffer(const floats4& activeColor_);
			void fillCoordinates(rynx::vec3f pos, rynx::vec3f left, rynx::vec3f up, float charWidth, float charHeight);
			void fillTextureCoordinates(const Font& font, char c);

		public:
			text_renderer(rynx::shared_ptr<rynx::graphics::GPUTextures> textures, rynx::shared_ptr<rynx::graphics::shaders> shaders);

			void setDefaultFont(const Font& font) { m_defaultFont = font; }
			const Font& getDefaultFont() const { return m_defaultFont; }

			void setCamera(rynx::observer_ptr<camera> pCamera) { m_pCamera = pCamera; }
			void drawText(const renderable_text& text);
		};
	}
}