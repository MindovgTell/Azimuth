#include "GLFWWindow.hpp"
#include "log/log.hpp"
#include <GLFW/glfw3.h>

using namespace azm;

DEFINE_LOG_CATEGORY_STATIC(LogGLFWWindow);

GLFWWindow::GLFWWindow(WindowId id, const WindowSettings& settings) : _id(id)
{
    // window initialization
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    _window = glfwCreateWindow(settings.width, settings.height, settings.title.c_str(), nullptr, nullptr);
    if (!_window)
    {
        AZM_LOG(LogGLFWWindow, Error, "Failed to create GLFW window!");
        return;
    }
    glfwSetWindowPos(_window, settings.x, settings.y);

    // user pointer
    glfwSetWindowUserPointer(_window, this);

    // events handling
    glfwSetWindowCloseCallback(_window, [](GLFWwindow* window)
        {
            auto *thisWindow = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            AZM_LOG(LogGLFWWindow, Display, "Window with id={} closed", thisWindow->_id.value);

            InputEvent event;
            event.type = EventType::WindowClose;
            // event.data = ...
            thisWindow->_windowEvent.invoke(event);
        }
    );

    glfwSetFramebufferSizeCallback(_window, [](GLFWwindow* window, int width, int height)
        {
            auto *thisWindow = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            AZM_LOG(LogGLFWWindow, Display, "Resize: width={}, height={}", width, height);

            InputEvent event;
            event.type = EventType::WindowResize;
            // event.data = ...
            thisWindow->_windowEvent.invoke(event);
        }
    );

    glfwSetKeyCallback(_window,[](GLFWwindow* window, int key, int scancode, int action, int mods)
        {   
            auto *thisWindow = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            AZM_LOG(LogGLFWWindow, Display, "Key={}, scancode={}", key, scancode);
            
            InputEvent event;
            event.type = EventType::KeyPress;
            // event.data = ...
            thisWindow->_windowEvent.invoke(event);
        }
    );

    glfwSetCursorPosCallback(_window, [](GLFWwindow* window, double xpos, double ypos) 
        {
            auto *thisWindow = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
                        
            InputEvent event;
            event.type = EventType::MouseMove;
            // event.data = ...
            thisWindow->_windowEvent.invoke(event);
        }   
    );

    glfwSetMouseButtonCallback(_window, [](GLFWwindow* window, int button, int action, int mods) 
        {
            auto *thisWindow = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
                        
            InputEvent event;
            event.type = EventType::MouseButton;
            // event.data = ...
            thisWindow->_windowEvent.invoke(event);
        }
    ); 

    glfwSetScrollCallback(_window, [](GLFWwindow* window, double xoffset, double yoffset)
        {
            auto *thisWindow = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
            
            InputEvent event;
            event.type = EventType::MouseScroll;
            // event.data = ...
            thisWindow->_windowEvent.invoke(event);
        }
    );
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

void* GLFWWindow::getNativeHandle() const
{
    return _window;
}
