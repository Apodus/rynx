
#include "userio.hpp"

#include <rynx/system/assert.hpp>
#include <rynx/graphics/window/window.hpp>

#include <iostream>
#include <functional>
#include <memory>

#include <GLFW/glfw3.h>

namespace {
	std::function<void(int, int, int, int)> g_keyboardKeyEventHandler;
	std::function<void(double, double)> g_mouseScrollEventHandler;
	std::function<void(double, double)> g_mouseMoveEventHandler;
	std::function<void(int, int, int)> g_mouseKeyEventHandler;
	std::function<void(int)> g_mouseEnteredHandler;
}

rynx::input::input(std::shared_ptr<Window> window)
{
	rynx_assert(!g_keyboardKeyEventHandler, "Multiple UserIO initialisations not allowed :( Never fix");
	
	for(auto& buttonState : m_buttonStates)
		buttonState = 0;

	m_window = window;
	m_mouseInScreen = true;

	g_keyboardKeyEventHandler = std::bind(&input::onKeyEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	g_mouseScrollEventHandler = std::bind(&input::onMouseScrollEvent, this, std::placeholders::_1, std::placeholders::_2);
	g_mouseMoveEventHandler = std::bind(&input::onMouseMoveEvent, this, std::placeholders::_1, std::placeholders::_2);
	g_mouseKeyEventHandler = std::bind(&input::onMouseButtonEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	g_mouseEnteredHandler = std::bind(&input::onMouseEnterEvent, this, std::placeholders::_1);

	glfwSetKeyCallback(window->getGLFWwindow(), keyCallbackDummy);
	glfwSetScrollCallback(window->getGLFWwindow(), scrollCallbackDummy);
	glfwSetCursorPosCallback(window->getGLFWwindow(), cursorPosCallbackDummy);
	glfwSetMouseButtonCallback(window->getGLFWwindow(), mouseButtonCallbackDummy);
	glfwSetCursorEnterCallback(window->getGLFWwindow(), cursorEnterCallbackDummy);
}

rynx::input::~input() {
	g_keyboardKeyEventHandler = nullptr;
	g_mouseScrollEventHandler = nullptr;
	g_mouseMoveEventHandler = nullptr;
	g_mouseKeyEventHandler = nullptr;
	g_mouseEnteredHandler = nullptr;
}

#if PLATFORM_WIN
#pragma warning (disable : 4800) // forcing int value to boolean
#endif


bool rynx::input::isKeyValueInState(int8_t keyValue, int8_t requestedValue, bool ignoreConsumed) {
	return (keyValue & (requestedValue | (!ignoreConsumed * rynx::input::KEY_CONSUMED))) == requestedValue;
}

bool rynx::input::isKeyClicked(rynx::key::physical key, bool ignoreConsumed) const {
	return isKeyInState(key, KEY_CLICK, ignoreConsumed);
}

bool rynx::input::isKeyPressed(rynx::key::physical key, bool ignoreConsumed) const {
	return isKeyInState(key, KEY_PRESSED, ignoreConsumed);
}

bool rynx::input::isKeyDown(rynx::key::physical key, bool ignoreConsumed) const {
	return isKeyInState(key, KEY_DOWN, ignoreConsumed);
}

bool rynx::input::isKeyRepeat(rynx::key::physical key, bool ignoreConsumed) const {
	return isKeyInState(key, KEY_REPEAT, ignoreConsumed);
}

bool rynx::input::isKeyReleased(rynx::key::physical key, bool ignoreConsumed) const {
	return isKeyInState(key, KEY_RELEASED, ignoreConsumed);
}

bool rynx::input::isKeyConsumed(rynx::key::physical key) const {
	return m_buttonStates[key.id] & KEY_CONSUMED * isUnhinged(key);
}

void rynx::input::consume(rynx::key::physical key) {
	if(isUnhinged(key))
		m_buttonStates[key.id] |= KEY_CONSUMED;
}

rynx::key::physical rynx::input::getAnyClicked() {
	for(int32_t index = 0; index < m_buttonStates.size(); ++index) {
		if (
			(m_buttonStates[index] & KEY_PRESSED) &&
			isUnhinged(rynx::key::physical{index}) &&
			((m_buttonStates[index] & KEY_CONSUMED) == 0)
		) {
			return rynx::key::physical{ index };
		}
	}
	return { 0 };
}

rynx::key::physical rynx::input::getAnyReleased() {
	for(int32_t index = 0; index < m_buttonStates.size(); ++index) {
		if(
			(m_buttonStates[index] & KEY_RELEASED) &&
			isUnhinged(rynx::key::physical{ index }) &&
			((m_buttonStates[index] & KEY_CONSUMED) == 0)
		) {
			return rynx::key::physical{ index };
		}
	}
	return { 0 };
}

float rynx::input::getMouseScroll() const {
	return m_mouseScroll;
}

#if PLATFORM_WIN
#pragma warning (default : 4800) // forcing int value to boolean
#endif

void rynx::input::update() {
	for(uint8_t& keyState : m_buttonStates) {
		keyState &= ~(KEY_PRESSED | KEY_RELEASED | KEY_REPEAT | KEY_CLICK | KEY_CONSUMED);
	}
	m_mouseScroll = 0;
	m_mouseDelta = vec3f(0, 0, 0);
}

void rynx::input::onKeyEvent(int key, int /* scancode */, int action, int /* mods */) {
	if (key < 0)
		return;
	if (action == GLFW_PRESS) {
		m_buttonStates[key] |= KEY_PRESSED | KEY_DOWN;
	}
	else if (action == GLFW_RELEASE) {
		m_buttonStates[key] = KEY_RELEASED | KEY_CLICK;
	}
	else if (action == GLFW_REPEAT) {
		m_buttonStates[key] |= KEY_REPEAT;
	}
}

void rynx::input::onMouseButtonEvent(int key, int action, int /* mods */) {
	if (action == GLFW_PRESS) {
		m_buttonStates[getMouseKeyCode(key).id] |= KEY_PRESSED | KEY_DOWN;
		m_mousePosition_clickBegin = m_mousePosition;
	}

	if (action == GLFW_RELEASE) {
		uint8_t value = KEY_RELEASED;
		if ((m_mousePosition - m_mousePosition_clickBegin).length_squared() < 0.02f * 0.02f) {
			value |= KEY_CLICK;
		}

		m_buttonStates[getMouseKeyCode(key).id] = value;
	}
}

void rynx::input::onMouseScrollEvent(double /* xoffset */, double yoffset) {
	m_mouseScroll = static_cast<float>(yoffset);
}

void rynx::input::onMouseMoveEvent(double xpos, double ypos) {
	vec3f newPos(
		2.0f * static_cast<float>(xpos) / m_window->width() - 1.0f,
		1.0f - 2.0f * static_cast<float>(ypos) / m_window->height(),
		0
	);

	m_mouseDelta = newPos - m_mousePosition;
	m_mousePosition = newPos;
}

void rynx::input::onMouseEnterEvent(int entered) {
	if (entered == GL_TRUE) {
		m_mouseInScreen = true;
	}
	if (entered == GL_FALSE) {
		m_mouseInScreen = false;
	}
}



void rynx::input::keyCallbackDummy(GLFWwindow* /* window */, int key, int scancode, int action, int mods)
{
	if(g_keyboardKeyEventHandler)
		g_keyboardKeyEventHandler(key, scancode, action, mods);
}

void rynx::input::mouseButtonCallbackDummy(GLFWwindow* /* window */, int button, int action, int mods)
{
	if(g_mouseKeyEventHandler)
		g_mouseKeyEventHandler(button, action, mods);
}

void rynx::input::scrollCallbackDummy(GLFWwindow* /* window */, double xoffset, double yoffset)
{
	if(g_mouseScrollEventHandler)
		g_mouseScrollEventHandler(xoffset, yoffset);
}

void rynx::input::cursorPosCallbackDummy(GLFWwindow* /* window */, double xpos, double ypos)
{
	if(g_mouseMoveEventHandler)
		g_mouseMoveEventHandler(xpos, ypos);
}

void rynx::input::cursorEnterCallbackDummy(GLFWwindow* /* window */, int entered)
{
	if(g_mouseEnteredHandler)
		g_mouseEnteredHandler(entered);
}

rynx::scoped_input_inhibitor rynx::input::inhibit_mouse_scoped() { return scoped_input_inhibitor(&m_inhibited, false, true); }
rynx::scoped_input_inhibitor rynx::input::inhibit_keyboard_scoped() { return scoped_input_inhibitor(&m_inhibited, true, false); }
rynx::scoped_input_inhibitor rynx::input::inhibit_mouse_and_keyboard_scoped() { return scoped_input_inhibitor(&m_inhibited, true, true); }