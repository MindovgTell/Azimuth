#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

#include <vector>

namespace azm::backend
{
    class VulkanPhysicalDevice;
    class VulkanDevice;

    class VulkanSwapChain
    {
    private:
        vk::raii::SwapchainKHR _swapChain = nullptr;
        std::vector<vk::Image> _images;
        std::vector<vk::raii::ImageView> _imageViews;
        vk::SurfaceFormatKHR _surfaceFormat{};
        vk::Extent2D _extent{};
    public:
        VulkanSwapChain() = default;
        ~VulkanSwapChain() = default;

        void create(VulkanPhysicalDevice const& physicalDevice,
                    VulkanDevice const& logicalDevice,
                    vk::raii::SurfaceKHR const& surface,
                    GLFWwindow* window);

        // In future better to return const
        vk::raii::SwapchainKHR& handle()
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

        std::vector<vk::raii::ImageView> const& imageViews() {return _imageViews;}

        void clear() {
            _imageViews.clear();
		    _swapChain = nullptr;
        }

    private:
        static uint32_t chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const& surfaceCapabilities);
        static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats);
        static vk::PresentModeKHR chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes);
        static vk::Extent2D chooseSwapExtent(vk::SurfaceCapabilitiesKHR const& capabilities, GLFWwindow* window);
        void createImageViews(VulkanDevice const& device);
    };
}