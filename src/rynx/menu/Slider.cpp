
#include <rynx/menu/Slider.hpp>

#include <rynx/graphics/mesh/mesh.hpp>
#include <rynx/graphics/mesh/polygonTesselator.hpp>
#include <rynx/graphics/texture/texturehandler.hpp>
#include <rynx/graphics/mesh/shape.hpp>
#include <rynx/graphics/renderer/meshrenderer.hpp>
#include <rynx/graphics/renderer/textrenderer.hpp>
#include <rynx/input/mapped_input.hpp>
#include <rynx/menu/Div.hpp>
#include <rynx/menu/Frame.hpp>


rynx::menu::SlideBarVertical::SlideBarVertical(GPUTextures& textures, std::string knobTexture, std::string baseTexture, Component* parent, vec3<float> scale, float minValue, float maxValue, float initialValue)
	: Component(parent, scale) {
	rynx_assert(minValue < maxValue, "wut");

	{
		auto slideBarDiv = std::make_shared<Div>(this, vec3<float>(1.0f, 0.25f, 0));
		slideBarDiv->addChild(std::make_shared<Frame>(slideBarDiv.get(), textures, std::move(baseTexture)));
		addChild(slideBarDiv);
	}

	{
		// todo: better formulation for slider knob dimensions
		auto knobDiv = std::make_shared<Div>(this, vec3<float>(0.1f, 1.0f, 0));
		knobDiv->addChild(std::make_shared<Frame>(knobDiv.get(), textures, std::move(baseTexture)));
		addChild(knobDiv);
		m_knobDiv = knobDiv.get();
	}

	m_minValue = minValue;
	m_maxValue = maxValue;
	m_mouseDragActive = false;

	setValue(initialValue);
}

rynx::menu::SlideBarVertical& rynx::menu::SlideBarVertical::onValueChanged(std::function<void(float)> t) {
	m_callback = std::move(t);
	return *this;
}

void rynx::menu::SlideBarVertical::onInput(rynx::input::mapped_input& input) {
	auto mousePos = input.mouseMenuPosition(aspectRatio());
	if (inRectComponent(mousePos)) {
		int mouseKey1 = input.applicationKeyByName("menuCursorActivation");
		if (input.isKeyPressed(mouseKey1))
			m_mouseDragActive = true;
		if (input.isKeyReleased(mouseKey1))
			m_mouseDragActive = false;

		if (m_mouseDragActive) {
			onActivate(mousePos.x, mousePos.y);
		}
	}
	else {
		m_mouseDragActive = false;
	}
}

void rynx::menu::SlideBarVertical::update(float dt) {
	m_knobDiv->position_local(scale_world().x * (m_currentValue - 0.5f));
	m_knobDiv->position_approach(dt * 10);
}

void rynx::menu::SlideBarVertical::onActivate(float x, float /* y */) {
	float length = scale_world().x;
	if (length > 0.0f) {
		x -= position_world().x;
		m_currentValue = (x / length + 0.5f);
		if (m_callback) {
			m_callback(m_currentValue * (m_maxValue - m_minValue) + m_minValue);
		}
	}
}

void rynx::menu::SlideBarVertical::setValue(float value) {
	m_currentValue = (value - m_minValue) / (m_maxValue - m_minValue);
	if (m_callback) {
		m_callback(m_currentValue * (m_maxValue - m_minValue) + m_minValue);
	}
}