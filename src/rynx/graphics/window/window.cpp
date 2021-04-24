
#include "window.hpp"
#include <rynx/tech/unordered_map.hpp>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <functional>
#include <iostream>
#include <memory>

#include <rynx/system/assert.hpp>

using ResizeEventMapper = rynx::unordered_map<GLFWwindow*, std::function<void(int, int)>>;

static std::unique_ptr<ResizeEventMapper>  g_resizeEventMapper;

static ResizeEventMapper& getResizeEventMapper()
{
	if (!g_resizeEventMapper)
	{
		g_resizeEventMapper = std::make_unique<ResizeEventMapper>();
	}
	return *g_resizeEventMapper;
}

void global_scope_windowResizeCallback(GLFWwindow* window, int width, int height) {
	getResizeEventMapper()[window](width, height);
}

Window::Window() {
	m_fullscreen = false;
	m_userRequestedExit = false;
	m_pWindow = nullptr;
	m_width = 0;
	m_height = 0;
}

Window::~Window() {
	glfwDestroyWindow(m_pWindow);
	m_pWindow = nullptr;
	g_resizeEventMapper.reset();
}

void Window::platformResizeEvent(int width, int height) {
	if (width == 0 || height == 0)
		return;

	m_width = width;
	m_height = height;
	m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	if (m_onResize)
		m_onResize(m_width, m_height);
	set_gl_viewport_to_window_dimensions();
}

void Window::on_resize(std::function<void(size_t, size_t)> onResize) {
	m_onResize = std::move(onResize);
}

void Window::set_gl_viewport_to_window_dimensions() const {
	glViewport(0, 0, int(m_width), int(m_height));
}

GLFWwindow* Window::createWindow(std::string name) {
	GLFWmonitor* monitor = (m_fullscreen) ? glfwGetPrimaryMonitor() : nullptr;
	GLFWwindow* window = glfwCreateWindow(static_cast<int>(m_width), static_cast<int>(m_height), name.c_str(), monitor, nullptr);

	if (!window) {
		std::cout << "Could not create window. Ensure OpenGL 4.3 is supported by your drivers." << std::endl;
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}

	m_pWindow = window;
	getResizeEventMapper()[m_pWindow] = [this](int x, int y) { platformResizeEvent(x, y); };
	glfwSetWindowSizeCallback(window, global_scope_windowResizeCallback);

	int width, height;
	glfwGetWindowSize(window, &width, &height);

	platformResizeEvent(width, height);
	return window;
}

void Window::createWindow(int width, int height, std::string name) {
	m_width = width;
	m_height = height;

	// Initialize GLFW
	if (!glfwInit()) {
		std::cout << "Failed to init GLFW" << std::endl;
		throw std::string("Failed to init GLFW");
	}

	// Window hints
	glfwWindowHint(GLFW_SAMPLES, 4); // FSAA
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // Min OpenGL 4.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	// glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	m_pWindow = createWindow(name);

	// Make the window's context current
	glfwMakeContextCurrent(m_pWindow);
	glfwSwapInterval(0); // No vsync?

	// Initialize GLEW
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		std::cout << err << std::endl;
		std::cout << "Renderer: " << std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER))) << std::endl;
		std::cout << "Version:  " << std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION))) << std::endl;
		throw std::string("Failed to init GLEW");
	}
	else {
		logmsg("Glew init OK");
	}
}

size_t Window::width() const {
	return m_width;
}

size_t Window::height() const {
	return m_height;
}

void Window::toggle_fullscreen() {
	m_fullscreen = !m_fullscreen;
	m_pWindow = createWindow();
}

void Window::swap_buffers() const {
	glfwSwapBuffers(m_pWindow);
}

void Window::enable_grab() const {
	// When using this, must draw your own in-game cursor. And remember to release mouse eventually.
	glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::disable_grab() const {
	glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

bool Window::shouldClose() const {
	return (glfwWindowShouldClose(m_pWindow) == 1) || m_userRequestedExit;
}

void Window::pollEvents() const {
	glfwPollEvents();
}

bool Window::active() const {
	return (glfwGetWindowAttrib(m_pWindow, GLFW_FOCUSED) != 0);
}

void Window::hide() const {
	disable_grab();
	glfwIconifyWindow(m_pWindow);
}

void Window::requestExit() {
	m_userRequestedExit = true;
}

GLFWwindow* Window::getGLFWwindow() const {
	return m_pWindow;
}

