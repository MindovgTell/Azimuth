#include "core/Application.hpp"
#include "window/GLFWWindow.hpp"

#include "Mesh.hpp"

#include <GLFW/glfw3.h>
#include <stdexcept>
#include <utility>
#include <vector>

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


        const std::vector<Vertex> vertices = {
            // Front face (+Z)
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 0
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 1
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 2
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 3

            // Back face (-Z)
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 4
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 5
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 6
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 7

            // Right face (+X)
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 8
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 9
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 10
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 11

            // Left face (-X)
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 12
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 13
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 14
            {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 15

            // Top face (+Y)
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 16
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 17
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 18
            {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 19

            // Bottom face (-Y)
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 20
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 21
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 22
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}  // 23
        };

        const std::vector<uint16_t> indices = {
            0,  1,  2,   2,  3,  0,   // Front
            4,  5,  6,   6,  7,  4,   // Back
            8,  9, 10,  10, 11,  8,   // Right
            12, 13, 14,  14, 15, 12,   // Left
            16, 17, 18,  18, 19, 16,   // Top
            20, 21, 22,  22, 23, 20    // Bottom
        };

        // In the VulkanApplication class:
        // Array of game objects to render
        std::vector<Object> gameObjects(6);

        // Object 1 - Center
        gameObjects[0].position = {0.0f, 0.0f, 0.0f};
        gameObjects[0].rotation = {0.0f, 0.0f, 0.0f};
        gameObjects[0].scale = {1.0f, 1.0f, 1.0f};

        // Object 2 - Left
        gameObjects[1].position = {-2.0f, 0.0f, -1.0f};
        gameObjects[1].rotation = {0.0f, glm::radians(45.0f), 0.0f};
        gameObjects[1].scale = {0.75f, 0.75f, 0.75f};

        // Object 3 - Right
        gameObjects[2].position = {2.0f, 0.0f, -1.0f};
        gameObjects[2].rotation = {0.0f, glm::radians(-45.0f), 0.0f};
        gameObjects[2].scale = {0.75f, 0.75f, 0.75f};

        gameObjects[3].position = {0.0f, 2.0f, -1.0f};
        gameObjects[3].rotation = {0.0f, glm::radians(-45.0f), 0.0f};
        gameObjects[3].scale = {0.75f, 0.75f, 0.75f};

        gameObjects[4].position = {0.0f, -1.0f, 2.0f};
        gameObjects[4].rotation = {0.0f, glm::radians(-45.0f), 0.0f};
        gameObjects[4].scale = {0.75f, 0.75f, 0.75f};

        gameObjects[5].position = {3.0f, -2.0f, -1.0f};
        gameObjects[5].rotation = {0.0f, glm::radians(-45.0f), 0.0f};
        gameObjects[5].scale = {0.75f, 0.75f, 0.75f};

        _core.init(pAppName, nativeWindow, vertices, indices, std::move(gameObjects));
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
