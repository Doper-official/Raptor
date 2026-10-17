#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace rap {

	class RaptorWindow {

	public:

		RaptorWindow(
			int w,
			int h,
			std::string name);

		~RaptorWindow();

		RaptorWindow(
			const RaptorWindow&) = delete;

		RaptorWindow& operator=(
			const RaptorWindow&) = delete;

		RaptorWindow(
			RaptorWindow&&) = delete;

		RaptorWindow& operator=(
			RaptorWindow&&) = delete;

		bool shouldClose();

		VkExtent2D getExtent() const;

		bool wasWindowResized() const;

		void resetWindowResizedFlag();

		GLFWwindow* getGLFWwindow() const;

		void createWindowSurface(
			VkInstance instance,
			VkSurfaceKHR* surface);

		void pollEvents();

	private:

		static void framebufferResizeCallback(
			GLFWwindow* window,
			int width,
			int height);

		static void keyCallback(
			GLFWwindow* window,
			int key,
			int scancode,
			int action,
			int mods);

		void initWindow();

		int width;
		int height;

		bool framebufferResized = false;

		std::string windowName;

		GLFWwindow* window = nullptr;
	};

} // namespace rap