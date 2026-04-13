#include "core/Application.hpp"
#include "window/GLFWWindowManager.hpp"

#include <cstdlib>

int main()
{
    azm::GLFWWindowManager windowManager;

    azm::WindowSettings windowSettings{
        .title = "Azimuth",
        .width = 800,
        .height = 600,
        .x = 100,
        .y = 100
    };

    auto windowIdResult = windowManager.createWindow(windowSettings);
    if (!windowIdResult.has_value())
    {
        return EXIT_FAILURE;
    }

    auto window = windowManager.getWindowById(windowIdResult.value());
    if (!window)
    {
        return EXIT_FAILURE;
    }

    azm::Applicaton app;
    app.init("Azimuth", window);
    app.run(windowManager);

    return EXIT_SUCCESS;
}
