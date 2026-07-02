#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>


namespace azm::backend
{

class VulkanPhysicalDevice;

// TODO: in future logical device class should work with bigger number of queues
class VulkanDevice
{
private:
    vk::raii::Device _device = nullptr;
    vk::raii::Queue  _queue  = nullptr;
public:
    VulkanDevice() = default;
    ~VulkanDevice() = default;

    void create(VulkanPhysicalDevice const& physicalDevice);

    vk::raii::Device const& handle() const
    {
        return _device;
    }

    vk::raii::Queue const& queue() const
    {
        return _queue;
    }

    
    vk::raii::ImageView createImageView(vk::Image const &image, vk::Format format, vk::ImageAspectFlags aspectFlags) const
	{
		vk::ImageViewCreateInfo viewInfo{
			.image            = image,
			.viewType         = vk::ImageViewType::e2D,
			.format           = format,
			.subresourceRange = {aspectFlags, 0, 1, 0, 1}};
		return vk::raii::ImageView(_device, viewInfo);
	}

};

}