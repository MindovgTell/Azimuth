#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window/IWindow.hpp"
#include "event/Event.hpp"
#include "event/IntupEvent.hpp"

namespace azm
{
    class GLFWWindow final : public IWindow
    {
    private:
        const WindowId _id;
        GLFWwindow* _window = nullptr;
        Event<const InputEvent&> _windowEvent;
    public:
        GLFWWindow(WindowId id, const WindowSettings& settings);
        ~GLFWWindow() override;

        void setTitle(const std::string& title) override;
        bool isValid() const override;
        bool shouldClose() const override;
    };
}