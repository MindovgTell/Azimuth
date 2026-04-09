#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window/IWindow.hpp"

namespace azm
{
    class GLFWWindow final : public IWindow
    {
    private:
        GLFWWindow* _window = nullptr;
    public:
        GLFWWindow(const WindowSettings& settings);
        ~GLFWWindow() override;

        void setTitle(const std::string& title) override;
        bool isValid() const override;
        bool shouldClose() const override;
    };
}