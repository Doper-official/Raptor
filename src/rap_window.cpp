
#include "rap_window.hpp"

// Standard
#include <cassert>
#include <iostream>

namespace rap {

    RaptorWindow::RaptorWindow(int w, int h, std::string name)
        : width{ w },
        height{ h },
        windowName{ std::move(name) } {
        initWindow();
    }

    RaptorWindow::~RaptorWindow() {
        if (window != nullptr) {
            glfwDestroyWindow(window);
            window = nullptr;
        }

        glfwTerminate();
    }

    void RaptorWindow::initWindow() {
        const int glfwInitResult = glfwInit();

        assert(glfwInitResult == GLFW_TRUE &&
            "Failed to initialize GLFW");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        assert(monitor != nullptr &&
            "Failed to get primary monitor");

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        assert(mode != nullptr &&
            "Failed to get primary monitor video mode");

        std::cout << "Monitor resolution: "
            << mode->width << "x" << mode->height << '\n';

        window = glfwCreateWindow(
            mode->width,
            mode->height,
            windowName.c_str(),
            monitor,
            nullptr
        );

        assert(window != nullptr &&
            "Failed to create GLFW window");

        width = mode->width;
        height = mode->height;

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if (glfwRawMouseMotionSupported()) {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }

        glfwSetWindowUserPointer(window, this);

        glfwSetFramebufferSizeCallback(
            window,
            framebufferResizeCallback
        );

        std::cout << "Window created: "
            << width << "x" << height << '\n';
    }

    VkResult RaptorWindow::createWindowSurface(
        VkInstance instance,
        VkSurfaceKHR* surface) {

        return glfwCreateWindowSurface(
            instance,
            window,
            nullptr,
            surface
        );
    }

    VkExtent2D RaptorWindow::getExtent() const {
        return {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
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

        auto* raptorWindow = static_cast<RaptorWindow*>(
            glfwGetWindowUserPointer(window)
            );

        if (raptorWindow == nullptr) {
            return;
        }

        raptorWindow->framebufferResized = true;
        raptorWindow->width = width;
        raptorWindow->height = height;
    }

    void RaptorWindow::keyCallback(
        GLFWwindow* window,
        int key,
        int scancode,
        int action,
        int mods) {

        // Add keyboard callback handling here when needed.
        (void)window;
        (void)key;
        (void)scancode;
        (void)action;
        (void)mods;
    }

} // namespace rap