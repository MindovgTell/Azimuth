#pragma once

#include "window/IWindow.hpp"
#include "AzmVkCore.hpp"


namespace azm
{
    class AzmApplicaton
    {
    private: 
        azm::backend::VkCore _core;

    public:
        AzmApplicaton();
        ~AzmApplicaton();

        void init(const char* pAppName, IWindow* window);

        void renderScene();
    };

}