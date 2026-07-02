#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include "VkPhysicalDevice.hpp"
#include "VkDevice.hpp"

namespace azm::backend
{

class VulkanBuffer {
private:
    vk::raii::Buffer        _buffer         = nullptr;
    vk::raii::DeviceMemory  _bufferMemory   = nullptr;
    vk::DeviceSize          _size           = 0;
public:
    VulkanBuffer() = default;
    VulkanBuffer(
        VulkanDevice const& device,
        VulkanPhysicalDevice const& physDevice,
        const void* data,
        vk::raii::CommandPool const& pool,
        vk::DeviceSize size, 
        vk::BufferUsageFlags usage, 
        vk::MemoryPropertyFlags properties
    );

    VulkanBuffer(VulkanBuffer const&) = delete;
    VulkanBuffer& operator=(VulkanBuffer const&) = delete;
    VulkanBuffer(VulkanBuffer&&) noexcept = default;
    VulkanBuffer& operator=(VulkanBuffer&&) noexcept = default;

    vk::raii::Buffer const& handle() const {return _buffer;};
    vk::DeviceSize size() const { return _size; }

private:
    void copyBuffer(VulkanDevice const& device, vk::raii::CommandPool const& pool, vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size);
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(VulkanDevice const& device, VulkanPhysicalDevice const& physDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
};

} // namespace azm::backend
