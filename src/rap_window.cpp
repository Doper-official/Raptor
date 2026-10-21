#include "rap_window.hpp"

// GLFW native Win32 access must be enabled before including glfw3native.h.
#define GLFW_EXPOSE_NATIVE_WIN32

// Windows
#include <windows.h>
#include <shobjidl.h>
#include <propkey.h>
#include <propvarutil.h>

// GLFW
#include <GLFW/glfw3native.h>

// Standard library
#include <cassert>
#include <iostream>

namespace rap {

    namespace {

        constexpr wchar_t RAPTOR_APP_ID[] = L"Doper.Raptor";

        bool setTaskbarIdentity(GLFWwindow* window) {
            HWND hwnd = glfwGetWin32Window(window);

            if (hwnd == nullptr) {
                return false;
            }

            IPropertyStore* propertyStore = nullptr;

            HRESULT hr = SHGetPropertyStoreForWindow(
                hwnd,
                IID_PPV_ARGS(&propertyStore)
            );

            if (FAILED(hr)) {
                return false;
            }

            PROPVARIANT appId;
            PropVariantInit(&appId);

            hr = InitPropVariantFromString(
                RAPTOR_APP_ID,
                &appId
            );

            if (SUCCEEDED(hr)) {
                hr = propertyStore->SetValue(
                    PKEY_AppUserModel_ID,
                    appId
                );
            }

            if (SUCCEEDED(hr)) {
                hr = propertyStore->Commit();
            }

            PropVariantClear(&appId);
            propertyStore->Release();

            return SUCCEEDED(hr);
        }

    } // namespace

    RaptorWindow::RaptorWindow(
        int w,
        int h,
        std::string name
    )
        : width{ w },
        height{ h },
        windowName{ name } {
        initWindow();
    }

    RaptorWindow::~RaptorWindow() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void RaptorWindow::initWindow() {
        glfwInit();

        glfwWindowHint(
            GLFW_CLIENT_API,
            GLFW_NO_API
        );

        glfwWindowHint(
            GLFW_RESIZABLE,
            GLFW_TRUE
        );

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        assert(monitor != nullptr && "Failed to get primary monitor");

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        assert(mode != nullptr && "Failed to get primary monitor video mode");

        std::cout
            << "Monitor resolution: "
            << mode->width << "x" << mode->height
            << '\n';

        window = glfwCreateWindow(
            mode->width,
            mode->height,
            windowName.c_str(),
            monitor,
            nullptr
        );

        assert(window != nullptr && "Failed to create GLFW window");

        width = mode->width;
        height = mode->height;

        // Give the game window the same Windows taskbar identity as the launcher.
        const bool taskbarIdentitySet = setTaskbarIdentity(window);

        assert(
            taskbarIdentitySet &&
            "Failed to set Raptor taskbar identity"
        );

        glfwSetInputMode(
            window,
            GLFW_CURSOR,
            GLFW_CURSOR_DISABLED
        );

        if (glfwRawMouseMotionSupported()) {
            glfwSetInputMode(
                window,
                GLFW_RAW_MOUSE_MOTION,
                GLFW_TRUE
            );
        }

        glfwSetWindowUserPointer(
            window,
            this
        );

        glfwSetFramebufferSizeCallback(
            window,
            framebufferResizeCallback
        );

        std::cout
            << "Window created: "
            << width << "x" << height
            << '\n';
    }

    VkResult RaptorWindow::createWindowSurface(
        VkInstance instance,
        VkSurfaceKHR* surface
    ) {
        return glfwCreateWindowSurface(
            instance,
            window,
            nullptr,
            surface
        );
    }

    VkExtent2D RaptorWindow::getExtent() {
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

    bool RaptorWindow::wasWindowResized() {
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
        int height
    ) {
        auto raptorWindow = static_cast<RaptorWindow*>(
            glfwGetWindowUserPointer(window)
            );

        raptorWindow->framebufferResized = true;
        raptorWindow->width = width;
        raptorWindow->height = height;
    }

} // namespace rap