#pragma once

#include "window/IWindow.hpp"
#include "window/IWindowManager.hpp"
// TODO: Add concepts for windows such as noncopyable
#include <memory>
#include <unordered_map>
#include <expected>

namespace azm
{

class GLFWWindowManager final : public IWindowManager
{
public:
    GLFWWindowManager();
    ~GLFWWindowManager() override;

    void update() override;
    bool areAllWindowsClosed() const override;

    std::expected<WindowId, WindowCreationError> createWindow(const WindowSettings& settings) override;
    std::shared_ptr<IWindow> getWindowById(WindowId id) const override;

private:
    bool _initialized{false};
    std::unordered_map<WindowId, std::shared_ptr<IWindow>> _windows;
    WindowId _windowIdCounter{1};

    void cleanupClosedWindows();
};

} // namespace azm
