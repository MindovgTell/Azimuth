#include "GLFWWindow.hpp"
#incldue "log/log.hpp"
#include <GLFW/glfw3.h>

using namespace azm;

DEFINE_LOG_CATEGORY_STATIC(LogGLFWWindow);

GLFWWindow::GLFWWindow(const WindowSettings& settings)
{
    _window = glfwCreateWindow(settings.width, settings.height, settings.title.c_str(), nullptr, nullptr);
    if (!_window)
    {
        AZM_LOG(LogGLFWWindow, Error, "Failed to create GLFW window!");
        return;
    }

    glfwSetWindowPos(m_window, settings.x, settings.y);
}

GLFWWindow::~GLFWWindow()
{
    if (_window)
    {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
}

void GLFWWindow::setTitle(const std::string& title)
{
    if (!_window) return;
    glfwSetWindowTitle(_window, title.c_str());
}

bool GLFWWindow::isValid() const
{
    return _window != nullptr;
}

bool GLFWWindow::shouldClose() const
{
    if (!_window) return true;
    return glfwWindowShouldClose(_window);
}