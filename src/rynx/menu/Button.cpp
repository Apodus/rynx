
#include <rynx/menu/Button.hpp>
#include <rynx/input/mapped_input.hpp>

#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>

#include <rynx/graphics/mesh/mesh.hpp>
#include <rynx/graphics/mesh/shape.hpp>

#include <rynx/graphics/text/font.hpp>

void rynx::menu::Button::initialize() {
	m_color = Color::WHITE;
	m_color.tick(1.0f);

	on_hover([this](rynx::vec3f mousePos, bool inRect) {
		if (inRect) {
			m_color.target_value().w = 1.0f;
		}
		else {
			m_color.target_value().w = 0.3f;
		}

		return inRect;
	});

	on_click([this]() {
		m_color.setCurrent({ 0, 1, 0, 1 });
	});
}

void rynx::menu::Button::onInput(rynx::mapped_input&) {}
void rynx::menu::Button::draw(rynx::graphics::renderer&, rynx::graphics::text_renderer&) const {}
void rynx::menu::Button::update(float dt) {
	m_background->color(m_color);
}