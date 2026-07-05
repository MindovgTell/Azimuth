#include "core/Application.hpp"
#include "window/GLFWWindow.hpp"

#include <GLFW/glfw3.h>
#include <chrono>
#include <stdexcept>
#include <utility>
#include <vector>

#include <random>

#include "scene/scene.hpp"

namespace azm
{
    namespace
    {
        glm::vec3 randomAxis(std::mt19937& rng)
        {
            std::uniform_real_distribution<float> axisDist(-1.0f, 1.0f);
            glm::vec3 axis{};

            do {
                axis = {axisDist(rng), axisDist(rng), axisDist(rng)};
            } while (glm::length(axis) < 0.001f);

            return glm::normalize(axis);
        }

        void updateSpinSystem(ecs::Registry& scene, float dt)
        {
            scene.each<Spin>([&](ecs::Entity entity, Spin& spin) {
                if (!scene.has<Transform>(entity))
                    return;

                auto& transform = scene.get<Transform>(entity);
                transform.rotation += spin.axis * spin.speed * dt;
            });
        }
    }

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

        _renderEntities.clear();

        auto createCube = [&](glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
            {
                static std::random_device spinDevice;
                static std::mt19937 spinRng(spinDevice());
                static std::uniform_real_distribution<float> speedDist(20.0f, 110.0f);

                ecs::Entity entity = _scene.create();
                _scene.emplace<Transform>(
                    entity,
                    Transform{
                        .position=position,
                        .rotation=rotation,
                        .scale=scale
                    }
                );

                _scene.emplace<Spin>(
                    entity,
                    Spin{
                        .axis = randomAxis(spinRng),
                        .speed = glm::radians(speedDist(spinRng))
                    }
                );
                _renderEntities.push_back(entity);
                return entity;
            };



        // createCube({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        // createCube({-2.0f, 0.0f, -1.0f}, {0.0f, glm::radians(45.0f), 0.0f}, {0.75f, 0.75f, 0.75f});
        // createCube({2.0f, 0.0f, -1.0f}, {0.0f, glm::radians(-45.0f), 0.0f}, {0.75f, 0.75f, 0.75f});
        // createCube({0.0f, 2.0f, -1.0f}, {0.0f, glm::radians(-45.0f), 0.0f}, {0.75f, 0.75f, 0.75f});
        // createCube({0.0f, -1.0f, 2.0f}, {0.0f, glm::radians(-45.0f), 0.0f}, {0.75f, 0.75f, 0.75f});
        // createCube({3.0f, -2.0f, -1.0f}, {0.0f, glm::radians(-45.0f), 0.0f}, {0.75f, 0.75f, 0.75f});

        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_real_distribution<float> dis(-3.0f, 3.0f);

        for (int i = 0; i < 30; ++i)
        {
            createCube({dis(rng), dis(rng), dis(rng)}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
        }

        std::vector<Object> gameObjects;

        for (ecs::Entity entity : _renderEntities)
        {
            auto& transform = _scene.get<Transform>(entity);
            Object object;
            object.position = transform.position;
            object.rotation = transform.rotation;
            object.scale = transform.scale;

            gameObjects.emplace_back(std::move(object));
        }

        auto mesh = createCubeMesh();
        _core.init(pAppName, nativeWindow, mesh.vertices, mesh.indices, std::move(gameObjects));
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

        auto previousTime = std::chrono::high_resolution_clock::now();

        while (!_window->shouldClose())
        {
            const auto currentTime = std::chrono::high_resolution_clock::now();
            const float dt = std::chrono::duration<float>(currentTime - previousTime).count();
            previousTime = currentTime;

            windowManager.update();
            if (_window->shouldClose())
            {
                break;
            }

            updateSpinSystem(_scene, dt);
            for (std::size_t i = 0; i < _renderEntities.size(); ++i)
            {
                const auto& transform = _scene.get<Transform>(_renderEntities[i]);
                _core.setObjectTransform(i, transform.position, transform.rotation, transform.scale);
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
