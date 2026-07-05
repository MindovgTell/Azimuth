#pragma once

#include "window/IWindow.hpp"
#include "window/IWindowManager.hpp"
#include "VkCore.hpp"
#include "ecs/ecs.hpp"

#include <memory>
#include <vector>


namespace azm
{
    class Applicaton
    {
    private:
        azm::backend::VkCore _core;
        ecs::Registry _scene;
        std::vector<ecs::Entity> _renderEntities;
        std::shared_ptr<IWindow> _window;
        bool _initialized{false};

    public:
        Applicaton();
        ~Applicaton();

        void init(const char* pAppName, std::shared_ptr<IWindow> window);
        void notifyFramebufferResized();
        void run(IWindowManager& windowManager);
        void shutdown();
    };

}
