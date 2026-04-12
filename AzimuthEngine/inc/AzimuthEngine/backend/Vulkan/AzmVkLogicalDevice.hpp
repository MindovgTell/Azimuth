#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>


namespace azm::backend
{

class VulkanPhysicalDevice;

// TODO: in future logical device class should work with bigger number of queues
class VulkanLogicalDevice
{
private:
    vk::raii::Device _device = nullptr;
    vk::raii::Queue  _queue  = nullptr;
public:
    VulkanLogicalDevice() = default;
    ~VulkanLogicalDevice() = default;


    void create(VulkanPhysicalDevice const& physicalDevice);

    vk::raii::Device const& handle() const
    {
        return _device;
    }

    vk::raii::Queue const& queue() const
    {
        return _queue;
    }
};

}