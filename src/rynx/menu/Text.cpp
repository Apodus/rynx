
#include <rynx/menu/Text.hpp>
#include <rynx/input/mapped_input.hpp>

#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>

#include <rynx/graphics/mesh/mesh.hpp>
#include <rynx/graphics/mesh/shape.hpp>

#include <rynx/graphics/text/font.hpp>

rynx::menu::Text::Text(
	vec3<float> scale,
	vec3<float> position
) : Component(scale, position)
{
	m_defaultScale = scale;
	m_text = "";
	m_text_color = Color::WHITE;
	m_align = TextRenderer::Align::Center;

	on_hover([this](rynx::vec3f mousePos, bool inRect) {
		m_color = m_text_color - m_text_color * m_text_dimming_when_not_hovering * inRect * config.dim_when_not_hover;
		return inRect;
	});

	on_click([this]() {
		if (config.allow_input) {
			m_menuSystem->capture_keyboard_input(this);
		}
	});
}

void rynx::menu::Text::onInput(rynx::mapped_input& input) {
}

void rynx::menu::Text::onDedicatedInput(rynx::mapped_input& input) {
	auto key = input.getAnyClicked_PhysicalKey();
	while (key) {
		const bool alhabetic_accept = config.input_type_alhabet && (key.id >= 'A' && key.id <= 'Z');
		const bool numeric_accept = config.input_type_numeric && (key.id >= '0' && key.id <= '9');
		if (alhabetic_accept || numeric_accept) {
			m_text.insert(m_text.begin() + m_cursor_pos, static_cast<char8_t>(key.id));
			++m_cursor_pos;
		}
		else if (key == rynx::key::codes::backspace()) {
			if (m_cursor_pos > 0) {
				--m_cursor_pos;
				m_text.erase(m_text.begin() + m_cursor_pos);
			}
		}
		else if (key == rynx::key::codes::delete_key()) {
			if (m_cursor_pos < m_text.size()) {
				m_text.erase(m_text.begin() + m_cursor_pos);
			}
		}
		else if (key == rynx::key::codes::arrow_left()) {
			if (m_cursor_pos > 0) {
				--m_cursor_pos;
			}
		}
		else if (key == rynx::key::codes::arrow_right()) {
			if (m_cursor_pos < m_text.size()) {
				++m_cursor_pos;
			}
		}
		else if (key == rynx::key::codes::enter()) {
			if (m_commit) {
				m_commit(m_text);
			}
			m_menuSystem->release_keyboard_input();
		}

		input.consume(key);
		key = input.getAnyClicked_PhysicalKey();
	}
}

void rynx::menu::Text::onDedicatedInputGained() {
	m_hasDedicatedInput = true;
}

void rynx::menu::Text::onDedicatedInputLost() {
	m_hasDedicatedInput = false;
}

void rynx::menu::Text::draw(MeshRenderer& meshRenderer, TextRenderer& textRenderer) const {
	if (!m_text.empty()) {
		const vec3<float>& m_pos = position_world();
		float x_offset = 0;
		if (m_align == TextRenderer::Align::Left)
			x_offset = -scale_world().x * 0.47f;
		if (m_align == TextRenderer::Align::Right)
			x_offset = +scale_world().x * 0.47f;

		textRenderer.drawText(m_text, m_pos.x + x_offset, m_pos.y, scale_world().y * 0.75f, m_color, m_align, *m_font);
		
		if (m_hasDedicatedInput) {
			vec3f cursorPos = textRenderer.position(m_text, m_cursor_pos, m_pos.x + x_offset, m_pos.y, scale_world().y * 0.75f, m_align, *m_font);
			textRenderer.drawText("I", cursorPos.x, cursorPos.y, scale_world().y * 0.5f, m_color, rynx::TextRenderer::Align::Center, *m_font);
		}
	}
}

void rynx::menu::Text::update(float dt) {
}