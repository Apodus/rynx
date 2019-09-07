
#pragma once

#include <array>
#include <string>
#include <cstdlib>
#include <memory>

#include <rynx/tech/math/vector.hpp>

struct GLFWwindow;

class Window;

class UserIO
{
	void onKeyEvent(int key, int scancode, int action, int mods);
	void onMouseButtonEvent(int key, int action, int mods);
	void onMouseScrollEvent(double xoffset, double yoffset);
	void onMouseMoveEvent(double xpos, double ypos);
	void onMouseEnterEvent(int entered);

	std::array<uint8_t, 512> m_buttonStates;
	vec3<float> m_mousePosition;
	vec3<float> m_mousePosition_clickBegin;
	std::shared_ptr<Window> m_window;
	float m_mouseScroll = 0;
	bool m_mouseInScreen = true;

	enum {
		KEY_DOWN = 1,
		KEY_PRESSED = 2,
		KEY_REPEAT = 4,
		KEY_RELEASED = 8,
		KEY_CLICK = 16,
		KEY_CONSUMED = 32
	};

public:
	UserIO(std::shared_ptr<Window> window);
	~UserIO();

	inline int getMouseKeyCode(int mouseButton) const {
		return mouseButton + 256;
	}

	template<class T>
	inline void getCursorPosition(T& t) const {
		t.x = m_mousePosition.x;
		t.y = m_mousePosition.y;
	}

	vec3<float> getCursorPosition() const
	{
		return m_mousePosition;
	}

	bool isKeyClicked(int key) const;
	bool isKeyPressed(int key) const;
	bool isKeyDown(int key) const;
	bool isKeyRepeat(int key) const;
	bool isKeyReleased(int key) const;
	bool isKeyConsumed(int key) const;
	void consume(int key);

	float getMouseScroll() const;

	int getAnyClicked();
	int getAnyReleased();

	void update();

	static void keyCallbackDummy(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseButtonCallbackDummy(GLFWwindow* window, int button, int action, int mods);
	static void scrollCallbackDummy(GLFWwindow* window, double xoffset, double yoffset);
	static void cursorPosCallbackDummy(GLFWwindow* window, double xpos, double ypos);
	static void cursorEnterCallbackDummy(GLFWwindow* window, int entered);
};