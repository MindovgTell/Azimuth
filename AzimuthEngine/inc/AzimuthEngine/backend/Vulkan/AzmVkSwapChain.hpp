#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

#include <vector>

namespace azm::backend
{
    class VulkanPhysicalDevice;
    class VulkanLogicalDevice;

    class VulkanSwapChain
    {
    public:
        VulkanSwapChain() = default;
        ~VulkanSwapChain() = default;

        void create(VulkanPhysicalDevice const& physicalDevice,
                    VulkanLogicalDevice const& logicalDevice,
                    vk::raii::SurfaceKHR const& surface,
                    GLFWwindow* window);

        vk::raii::SwapchainKHR const& handle() const
        {
            return _swapChain;
        }

        std::vector<vk::Image> const& images() const
        {
            return _images;
        }

        vk::SurfaceFormatKHR const& surfaceFormat() const
        {
            return _surfaceFormat;
        }

        vk::Extent2D extent() const
        {
            return _extent;
        }

    private:
        static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);
        static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
        static vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes);
        static vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities, GLFWwindow* window);

    private:
        vk::raii::SwapchainKHR _swapChain = nullptr;
        std::vector<vk::Image> _images;
        vk::SurfaceFormatKHR _surfaceFormat{};
        vk::Extent2D _extent{};
    };
}