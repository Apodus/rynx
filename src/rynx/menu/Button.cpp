
#include <rynx/menu/Button.hpp>
#include <rynx/input/mapped_input.hpp>

#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>

#include <rynx/graphics/mesh/mesh.hpp>
#include <rynx/graphics/mesh/shape.hpp>

#include <rynx/graphics/text/font.hpp>

void rynx::menu::Button::initialize() {
	m_text = "";
	m_color = Color::WHITE;
	m_color.tick(1.0f);
	m_textColor = Color::WHITE;
	m_textColor.tick(1.0f);
	m_align = TextRenderer::Align::Center;

	on_hover([this](rynx::vec3f mousePos, bool inRect) {
		if (inRect) {
			m_color.target_value().a = 1.0f;
			m_textColor = vec4<float>(1.0f, 1.0f, 1.0f, 1.0f);
		}
		else {
			m_color.target_value().a = 0.3f;
			m_textColor = vec4<float>(0.7f, 0.7f, 0.7f, 1.0f);
		}

		return inRect;
	});

	on_click([this]() {
		m_color.setCurrent({ 0, 1, 0, 1 });
	});
}

void rynx::menu::Button::onInput(rynx::mapped_input& input) {}

void rynx::menu::Button::draw(MeshRenderer& meshRenderer, TextRenderer& textRenderer) const {
	m_frame.draw(meshRenderer, textRenderer);
	if (!m_text.empty()) {
		const vec3<float>& m_pos = position_world();
		textRenderer.drawText(m_text, m_pos.x, m_pos.y, scale_world().y * 0.75f, m_textColor, m_align, *m_font);
	}
}

void rynx::menu::Button::update(float dt) {
	m_frame.tick(dt, aspectRatio());
	m_color.tick(dt * 5);
	m_frame.color(m_color);
	m_textColor.tick(dt * 5);
}