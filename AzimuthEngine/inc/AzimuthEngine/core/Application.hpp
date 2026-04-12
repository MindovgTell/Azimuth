#pragma once

#include "window/IWindow.hpp"
#include "AzmVkCore.hpp"


namespace azm
{
    class Applicaton
    {
    private: 
        azm::backend::VkCore _core;

    public:
        Applicaton();
        ~Applicaton();

        void init(const char* pAppName, IWindow* window);

        void renderScene();
    };

}