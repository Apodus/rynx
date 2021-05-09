
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
	
	m_text_color = Color::WHITE;
	m_textline.align_center();

	on_hover([this]([[maybe_unused]] rynx::vec3f mousePos, bool inRect) {
		m_color = m_text_color - m_text_color * m_text_dimming_when_not_hovering * inRect * config.dim_when_not_hover;
		return inRect && config.allow_input;
	});

	on_click([this]() {
		if (config.allow_input) {
			m_menuSystem->capture_keyboard_input(this);
		}
	});
}

void rynx::menu::Text::onInput([[maybe_unused]] rynx::mapped_input& input) {
}

void rynx::menu::Text::onDedicatedInput(rynx::mapped_input& input) {
	auto key = input.getAnyClicked_PhysicalKey();
	while (key) {
		const bool alhabetic_accept = config.input_type_alhabet && (key.id >= 'A' && key.id <= 'Z');
		const bool numeric_accept = config.input_type_numeric && (key.id >= '0' && key.id <= '9');
		if (alhabetic_accept || numeric_accept) {
			char8_t character = static_cast<char8_t>(key.id);
			if (alhabetic_accept) {
				bool left_shift = input.isKeyDown(rynx::key::codes::shift_left());
				bool right_shift = input.isKeyDown(rynx::key::codes::shift_right()); 
				if (left_shift || right_shift) {

				}
				else {
					character -= char8_t('A' - 'a');
				}
			}

			m_textline.text().insert(m_textline.text().begin() + m_cursor_pos, character);
			++m_cursor_pos;
		}
		else if (key == rynx::key::codes::backspace()) {
			if (m_cursor_pos > 0) {
				--m_cursor_pos;
				m_textline.text().erase(m_textline.text().begin() + m_cursor_pos);
			}
		}
		else if (key == rynx::key::codes::delete_key()) {
			if (m_cursor_pos < m_textline.text().size()) {
				m_textline.text().erase(m_textline.text().begin() + m_cursor_pos);
			}
		}
		else if (key == rynx::key::codes::arrow_left()) {
			if (m_cursor_pos > 0) {
				--m_cursor_pos;
			}
		}
		else if (key == rynx::key::codes::arrow_right()) {
			if (m_cursor_pos < m_textline.text().size()) {
				++m_cursor_pos;
			}
		}
		else if (key == rynx::key::codes::enter()) {
			if (m_commit) {
				m_commit(m_textline.text());
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

void rynx::menu::Text::draw(rynx::graphics::renderer& renderer) const {
	if (m_textline) {
		renderer.drawText(m_textline);
		
		if (m_hasDedicatedInput) {
			vec3f cursorPos = m_textline.position(renderer.getDefaultFont(), m_cursor_pos);
			
			rynx::graphics::renderable_text blob;
			blob.text() = "|";
			blob.font(m_textline.font());
			blob.font_size(m_textline.font_size());
			blob.align_center();
			blob.pos() = rynx::vec3f(cursorPos.x, cursorPos.y, 0);
			blob.color() = m_color;
			blob.color().w *= 0.75f;

			renderer.drawText(blob);
		}
	}
}

void rynx::menu::Text::update(float /* dt */) {
	m_textline.pos() = position_world();
	if (m_textline.is_align_left())
		m_textline.pos() += rynx::vec3f(-scale_world().x * 0.47f, 0, 0);
	else if (m_textline.is_align_right())
		m_textline.pos() += rynx::vec3f(+scale_world().x * 0.47f, 0, 0);
	
	m_textline.font_size(scale_world().y * 0.75f);
	m_textline.color() = m_color;
}