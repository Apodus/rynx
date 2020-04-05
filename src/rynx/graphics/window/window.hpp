
#pragma once

#include <string>
#include <functional>

struct GLFWwindow;

class Window {
public:
	Window();
	~Window();

	size_t width() const;
	size_t height() const;

	void toggle_fullscreen();
	void swap_buffers() const;

	void createWindow(int, int, std::string name = "Game");

	void enable_grab() const;
	void disable_grab() const;

	void requestExit();
	bool shouldClose() const;
	void pollEvents() const;

	bool active() const;
	void hide() const;

	void on_resize(std::function<void(size_t, size_t)> onResize);

	inline float getAspectRatio() const {
		return m_aspectRatio;
	}

	void set_gl_viewport_to_window_dimensions() const;

	GLFWwindow* getGLFWwindow() const;

private:
	Window& operator=(const Window&); // Disabled.
	Window(const Window&); // Disabled.

	GLFWwindow* createWindow(std::string name = "Game");
	void platformResizeEvent(int width, int height);

	std::function<void(size_t, size_t)> m_onResize;

	size_t m_width;
	size_t m_height;

	bool m_fullscreen;
	bool m_userRequestedExit;

	float m_aspectRatio;

	GLFWwindow* m_pWindow;
};