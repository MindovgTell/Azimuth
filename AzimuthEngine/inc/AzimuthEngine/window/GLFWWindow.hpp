#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "window/IWindow.hpp"
#include "event/Event.hpp"
#include "event/IntupEvent.hpp"

namespace azm
{
    class Applicaton;

    class GLFWWindow final : public IWindow
    {
    private:
        struct CallbackContext
        {
            GLFWWindow* window = nullptr;
            Applicaton* application = nullptr;
        };

        const WindowId _id;
        GLFWwindow* _window = nullptr;
        Event<const InputEvent&> _windowEvent;
        CallbackContext _callbackContext{};
    public:
        GLFWWindow(WindowId id, const WindowSettings& settings);
        ~GLFWWindow() override;

        void setApplication(Applicaton* application);
        void setTitle(const std::string& title) override;
        bool isValid() const override;
        bool shouldClose() const override;
        void* getNativeHandle() const override;
    };
}
