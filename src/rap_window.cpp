#include "rap_window.hpp"

#include <stdexcept>
#include <utility>
#include <iostream>

namespace rap {

	RaptorWindow::RaptorWindow(
		int w,
		int h,
		std::string name)
		: width{ w },
		height{ h },
		windowName{ std::move(name) } {

		initWindow();
	}

	RaptorWindow::~RaptorWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void RaptorWindow::initWindow() {

		if (!glfwInit()) {
			throw std::runtime_error(
				"OUHHH NOOOO I CANT FIND ME WINDOW (Scottish accent)");
		}

		// -------------------------
		// GLFW window settings
		// -------------------------

		glfwWindowHint(
			GLFW_CLIENT_API,
			GLFW_NO_API);

		glfwWindowHint(
			GLFW_RESIZABLE,
			GLFW_TRUE);

		// -------------------------
		// Fullscreen
		// -------------------------

		GLFWmonitor* monitor =
			glfwGetPrimaryMonitor();

		if (monitor == nullptr) {
			throw std::runtime_error(
				"failed to find primary monitor!");
		}

		const GLFWvidmode* mode =
			glfwGetVideoMode(monitor);

		if (mode == nullptr) {
			throw std::runtime_error(
				"failed to get monitor video mode!");
		}

		// Create fullscreen window at the monitor's
		// current resolution and refresh rate.
		window =
			glfwCreateWindow(
				mode->width,
				mode->height,
				windowName.c_str(),
				monitor,
				nullptr);

		if (window == nullptr) {
			throw std::runtime_error("failed to create GLFW window!");
		}

		std::cout << "Fullscreen monitor: "
			<< glfwGetWindowMonitor(window) << '\n';

		std::cout << "Window size: "
			<< width << " x " << height << '\n';

		if (window == nullptr) {
			throw std::runtime_error(
				"failed to create GLFW window!");
		}

		// Update our stored dimensions.
		width = mode->width;
		height = mode->height;

		// -------------------------
		// Mouse
		// -------------------------

		// Hide and capture the mouse cursor.
		glfwSetInputMode(
			window,
			GLFW_CURSOR,
			GLFW_CURSOR_DISABLED);

		// Use raw mouse input if supported.
		if (glfwRawMouseMotionSupported()) {
			glfwSetInputMode(
				window,
				GLFW_RAW_MOUSE_MOTION,
				GLFW_TRUE);
		}

		// -------------------------
		// Callbacks
		// -------------------------

		glfwSetWindowUserPointer(
			window,
			this);

		glfwSetFramebufferSizeCallback(
			window,
			framebufferResizeCallback);

		glfwSetKeyCallback(
			window,
			keyCallback);
	}

	void RaptorWindow::createWindowSurface(
		VkInstance instance,
		VkSurfaceKHR* surface) {

		if (glfwCreateWindowSurface(
			instance,
			window,
			nullptr,
			surface) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to create window surface!");
		}
	}

	VkExtent2D RaptorWindow::getExtent() const {

		int framebufferWidth;
		int framebufferHeight;

		glfwGetFramebufferSize(
			window,
			&framebufferWidth,
			&framebufferHeight);

		return {
			static_cast<uint32_t>(framebufferWidth),
			static_cast<uint32_t>(framebufferHeight)
		};
	}

	bool RaptorWindow::shouldClose() {
		return glfwWindowShouldClose(window);
	}

	void RaptorWindow::pollEvents() {
		glfwPollEvents();
	}

	bool RaptorWindow::wasWindowResized() const {
		return framebufferResized;
	}

	void RaptorWindow::resetWindowResizedFlag() {
		framebufferResized = false;
	}

	GLFWwindow* RaptorWindow::getGLFWwindow() const {
		return window;
	}

	void RaptorWindow::framebufferResizeCallback(
		GLFWwindow* window,
		int width,
		int height) {

		auto windowObject =
			reinterpret_cast<RaptorWindow*>(
				glfwGetWindowUserPointer(window));

		windowObject->framebufferResized = true;

		windowObject->width = width;
		windowObject->height = height;
	}

	void RaptorWindow::keyCallback(
		GLFWwindow* window,
		int key,
		int scancode,
		int action,
		int mods) {

		if (key == GLFW_KEY_ESCAPE &&
			action == GLFW_PRESS) {

			glfwSetWindowShouldClose(
				window,
				GLFW_TRUE);
		}
	}

} // namespace rap