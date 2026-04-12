#include "AzmVkSwapChain.hpp"
#include "AzmVkPhysDevice.hpp"
#include "AzmVkLogicalDevice.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <stdexcept>

namespace azm::backend
{

    void VulkanSwapChain::create(VulkanPhysicalDevice const& physicalDevice,
                    VulkanLogicalDevice const& logicalDevice,
                    vk::raii::SurfaceKHR const& surface,
                    GLFWwindow* window)
    {

        if (physicalDevice.handle() == nullptr)
        {
            throw std::runtime_error("Cannot create swapchain: invalid physical device");
        }

        if (logicalDevice.handle() == nullptr)
        {
            throw std::runtime_error("Cannot create swapchain: invalid logical device");
        }

		vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.handle().getSurfaceCapabilitiesKHR(*surface);
		_extent 							   = chooseSwapExtent(surfaceCapabilities, window);
		uint32_t minImageCount 						   = chooseSwapMinImageCount(surfaceCapabilities);

		std::vector<vk::SurfaceFormatKHR> availableFormats = physicalDevice.handle().getSurfaceFormatsKHR(*surface);
		if (availableFormats.empty())
        {
            throw std::runtime_error("Cannot create swapchain: no surface formats available");
        }
        
        _surfaceFormat 							   = chooseSwapSurfaceFormat(availableFormats);
		
		std::vector<vk::PresentModeKHR> availablePresentModes = physicalDevice.handle().getSurfacePresentModesKHR(*surface);
		vk::PresentModeKHR 				presentMode 		  = chooseSwapPresentMode(availablePresentModes);

		vk::SwapchainCreateInfoKHR swapChainCreateInfo{
			.surface = surface,
			.minImageCount = minImageCount,
			.imageFormat  = _surfaceFormat.format,
			.imageColorSpace = _surfaceFormat.colorSpace,
			.imageExtent = _extent,
			.imageArrayLayers = 1,
			.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
			.imageSharingMode = vk::SharingMode::eExclusive,
			.preTransform = surfaceCapabilities.currentTransform,
			.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode = presentMode,
			.clipped = true
		};

		_swapChain 		= vk::raii::SwapchainKHR(logicalDevice.handle(), swapChainCreateInfo);
		_images = _swapChain.getImages(); 
	}

	uint32_t VulkanSwapChain::chooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const &surfaceCapabilities) {
		auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
		if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount)) {
			minImageCount = surfaceCapabilities.maxImageCount;
		}
		return minImageCount;
	}

	vk::SurfaceFormatKHR VulkanSwapChain::chooseSwapSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const &availableFormats)
	{
		const auto formatIt = std::ranges::find_if(
			availableFormats,
			[](const auto &format){return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;}
		);
		return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
	}

	vk::PresentModeKHR VulkanSwapChain::chooseSwapPresentMode(std::vector<vk::PresentModeKHR> const &availablePresentModes)
	{
		assert(std::ranges::any_of(availablePresentModes, [](auto presentMode){return presentMode == vk::PresentModeKHR::eFifo;}));
		return std::ranges::any_of(
				availablePresentModes,
				[](const vk::PresentModeKHR value) {return vk::PresentModeKHR::eMailbox == value;}) ?
			vk::PresentModeKHR::eMailbox :
			vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D VulkanSwapChain::chooseSwapExtent(vk::SurfaceCapabilitiesKHR const &capabilities, GLFWwindow* window) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		int width, height;

		glfwGetFramebufferSize(window, &width, &height);

		return {
			std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}

}