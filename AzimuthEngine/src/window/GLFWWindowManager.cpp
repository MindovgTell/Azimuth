#include "GLFWWindowManager.hpp"
#include "GLFWWindow.hpp"
#include "log/log.hpp"
#include <GLFW/glfw3.h>

using namespace azm;

DEFINE_LOG_CATEGORY_STATIC(LogGLFWWindowManager);

GLFWWindowManager::GLFWWindowManager()
{
    glfwSetErrorCallback([](int errorCode, const char* description)  //
        { AZM_LOG(LogGLFWWindowManager, Error, "GLFW error, code: {}, description: {}", errorCode, description); });

    if (!glfwInit())
    {
        AZM_LOG(LogGLFWWindowManager, Error, "Failed to initialize GLFW!");
        return;
    }

    _initialized = true;
    AZM_LOG(LogGLFWWindowManager, Display, "GLFW initialized successfully!");
}

GLFWWindowManager::~GLFWWindowManager()
{
    _windows.clear();
    if (_initialized)
    {
        glfwSetErrorCallback(nullptr);
        glfwTerminate();
    }

    _initialized = false;
    AZM_LOG(LogGLFWWindowManager, Display, "GLFW shutdown complete!");
}

void GLFWWindowManager::update()
{
    if (!_initialized) return;

    glfwPollEvents();
    cleanupClosedWindows();
}

void GLFWWindowManager::cleanupClosedWindows()
{
    auto it = _windows.begin();
    while (it != _windows.end())
    {
        if (it->second->shouldClose())
        {
            AZM_LOG(LogGLFWWindowManager, Display, "Remove closed window with id: {}", it->first.value);
            it = _windows.erase(it);
            continue;
        }
        ++it;
    }
}

bool GLFWWindowManager::areAllWindowsClosed() const
{
    return _windows.empty();
}

std::expected<WindowId, WindowCreationError> GLFWWindowManager::createWindow(const WindowSettings& settings)
{
    if (!_initialized)
    {
        AZM_LOG(LogGLFWWindowManager, Error, "Cannot create window. GLFW is not initialized.");
        return std::unexpected(WindowCreationError::ManagerIsNotInitialized);
    }

    auto window = std::make_shared<GLFWWindow>(settings);
    if (!window->isValid())
    {
        AZM_LOG(LogGLFWWindowManager, Error, "Failed to create GLFW window.");
        return std::unexpected(WindowCreationError::CreationFailed);
    }

    const WindowId id = _windowIdCounter++;
    _windows[id] = window;

    AZM_LOG(LogGLFWWindowManager, Display, "Added window with id: {}", id.value);

    return id;
}

std::shared_ptr<IWindow> GLFWWindowManager::getWindowById(WindowId id) const
{
    const auto it = _windows.find(id);
    return it != _windows.end() ? it->second : nullptr;
}