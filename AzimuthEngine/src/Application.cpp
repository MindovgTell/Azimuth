#include "core/Application.hpp"
#include "window/GLFWWindow.hpp"

#include <GLFW/glfw3.h>
#include <stdexcept>

namespace azm
{
    Applicaton::Applicaton() = default;
    Applicaton::~Applicaton()
    {
        shutdown();
    }

    void Applicaton::init(const char* pAppName, std::shared_ptr<IWindow> window)
    {
        if (window == nullptr)
        {
            throw std::runtime_error("Cannot initialize application: window is null");
        }

        auto* nativeWindow = static_cast<GLFWwindow*>(window->getNativeHandle());
        if (nativeWindow == nullptr)
        {
            throw std::runtime_error("Cannot initialize application: native window handle is null");
        }

        _window = std::move(window);
        if (auto glfwWindow = std::dynamic_pointer_cast<GLFWWindow>(_window))
        {
            glfwWindow->setApplication(this);
        }
        _core.init(pAppName, nativeWindow);
        _initialized = true;
    }

    void Applicaton::notifyFramebufferResized()
    {
        _core.notifyFramebufferResized();
    }

    void Applicaton::run(IWindowManager& windowManager)
    {
        if (!_initialized || _window == nullptr)
        {
            throw std::runtime_error("Cannot run application: application is not initialized");
        }

        while (!_window->shouldClose())
        {
            windowManager.update();
            if (_window->shouldClose())
            {
                break;
            }

            _core.drawFrame(static_cast<GLFWwindow*>(_window->getNativeHandle()));
        }
    }

    void Applicaton::shutdown()
    {
        if (!_initialized)
        {
            return;
        }

        _core.waitIdle();
        _window.reset();
        _initialized = false;
    }

}
