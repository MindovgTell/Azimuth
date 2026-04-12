#include "core/Application.hpp"

#include <GLFW/glfw3.h>
#include <stdexcept>

namespace azm
{
    Applicaton::Applicaton() = default;
    Applicaton::~Applicaton() = default;

    void Applicaton::init(const char* pAppName, IWindow* window)
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

        _core.init(pAppName, nativeWindow);
    }

    void Applicaton::renderScene() {
        _core.drawFrame();
    }
}
