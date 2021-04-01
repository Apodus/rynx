
#include <rynx/menu/Slider.hpp>

#include <rynx/graphics/mesh/mesh.hpp>
#include <rynx/math/geometry/polygon_triangulation.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/input/mapped_input.hpp>
#include <rynx/menu/Div.hpp>
#include <rynx/menu/Frame.hpp>

#include <algorithm>

rynx::menu::SlideBarVertical::SlideBarVertical(rynx::graphics::GPUTextures& textures, std::string baseTexture, std::string knobTexture, vec3<float> scale, float minValue, float maxValue, float initialValue)
	: Component(scale) {
	rynx_assert(minValue < maxValue, "wut");

	{
		auto slideBarDiv = std::make_shared<Div>(vec3<float>(1.0f, 0.25f, 0));
		auto slideBarFrame = std::make_shared<Frame>(textures, std::move(baseTexture), 0.15f);
		slideBarDiv->addChild(std::move(slideBarFrame));
		addChild(slideBarDiv);
	}

	{
		// todo: better formulation for slider knob dimensions
		auto knobDiv = std::make_shared<Div>(vec3<float>(0.2f, 0.5f, 0));
		auto knobFrame = std::make_shared<Frame>(textures, std::move(knobTexture));
		knobDiv->addChild(std::move(knobFrame));
		addChild(knobDiv);
		m_knobDiv = knobDiv.get();
	}

	m_minValue = minValue;
	m_maxValue = maxValue;
	m_mouseDragActive = false;

	setValue(initialValue);
}

rynx::menu::SlideBarVertical& rynx::menu::SlideBarVertical::on_value_changed(std::function<void(float)> t) {
	m_callback = std::move(t);
	return *this;
}

rynx::menu::SlideBarVertical& rynx::menu::SlideBarVertical::on_drag_end(std::function<void(float)> t) {
	m_on_drag_end = std::move(t);
	return *this;
}

rynx::menu::SlideBarVertical& rynx::menu::SlideBarVertical::on_active_tick(std::function<void(float, float)> t) {
	m_on_active_tick = std::move(t);
	return *this;
}

void rynx::menu::SlideBarVertical::onInput(rynx::mapped_input& input) {
	auto mousePos = input.mouseMenuPosition(aspectRatio());
	if (inRectComponent(mousePos)) {
		rynx::key::physical mouseKey1 = input.getMouseKeyPhysical(0);
		if (input.isKeyPressed(mouseKey1)) {
			input.consume(mouseKey1);
			m_mouseDragActive = true;
			m_menuSystem->capture_mouse_input(this);
		}
	}
}

void rynx::menu::SlideBarVertical::onDedicatedInput(rynx::mapped_input& input) {
	rynx::key::physical mouseKey1 = input.getMouseKeyPhysical(0);
	if (input.isKeyReleased(mouseKey1)) {
		m_mouseDragActive = false;
		if (m_on_drag_end) {
			m_on_drag_end(getValue());
		}
		m_menuSystem->release_mouse_input();
	}
	else {
		auto mousePos = input.mouseMenuPosition(aspectRatio());
		input.consume(mouseKey1);
		onActivate(mousePos.x, mousePos.y);
	}
}

void rynx::menu::SlideBarVertical::update(float dt) {
	m_knobDiv->color().w = m_color->w;
	if(m_background)
		m_background->color().w = m_color->w;
	set_input_enabled(m_color->w > 0.01f);

	m_knobDiv->position_local(scale_world().x * (m_currentValue - 0.5f));
	m_knobDiv->position_approach(dt * 10);

	if (m_mouseDragActive) {
		if (m_on_active_tick) {
			m_on_active_tick(getValue(), dt);
		}
	}
}

void rynx::menu::SlideBarVertical::onActivate(float x, float /* y */) {
	float length = scale_world().x;
	if (length > 0.0f) {
		x -= position_world().x;
		m_currentValue = std::clamp((x / length + 0.5f), 0.0f, 1.0f);
		
		if (m_callback) {
			m_callback(getValue());
		}
	}
}

void rynx::menu::SlideBarVertical::setValue(float value) {
	m_currentValue = (value - m_minValue) / (m_maxValue - m_minValue);
	if (m_callback) {
		m_callback(getValue());
	}
}

float rynx::menu::SlideBarVertical::getValue() const {
	return m_currentValue * (m_maxValue - m_minValue) + m_minValue;
}

float rynx::menu::SlideBarVertical::getValueQuadratic() const {
	return m_currentValue * std::fabs(m_currentValue) * (m_maxValue - m_minValue) + m_minValue;
}

float rynx::menu::SlideBarVertical::getValueQuadratic_AroundCenter() const {
	float v = (m_currentValue - 0.5f) * 2.0f;
	return (v * std::fabs(v) + 1.0f) * 0.5f * (m_maxValue - m_minValue) + m_minValue;
}

float rynx::menu::SlideBarVertical::getValueCubic() const {
	return m_currentValue * m_currentValue * m_currentValue * (m_maxValue - m_minValue) + m_minValue;
}

float rynx::menu::SlideBarVertical::getValueCubic_AroundCenter() const {
	float v = (m_currentValue - 0.5f) * 2.0f;
	return  (v * v * v + 1.0f) * 0.5f * (m_maxValue - m_minValue) + m_minValue;
}