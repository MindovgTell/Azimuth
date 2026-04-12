#pragma once 

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>
#include <optional>

namespace azm::backend
{

    struct QueueLookup
    {
        std::optional<uint32_t> idx;

        bool complete() const
        {
            return idx.has_value();
        }
    };

    // In future will be extended for different queue families support, now we are looking for 
    // a family which support both graphics and surface
    struct QueueLayout
    {
        uint32_t idx = 0;
    };

    struct DeviceRequirements
    {
        bool shaderDrawParameters = true;
        bool dynamicRendering = true;
        bool extendedDynamicState = true;
    };

    struct DeviceCapabilities
    {
        vk::PhysicalDeviceProperties properties{};
        vk::PhysicalDeviceMemoryProperties memory{};
        QueueLayout queues{};

        bool shaderDrawParameters = false;
        bool dynamicRendering = false;
        bool extendedDynamicState = false;
    };

    class VulkanPhysicalDevice 
    {
    private:
    // For now we are limiting only for one device in application
    // in future we can extend usage to multiple devices during execution
        DeviceRequirements _requirements{};
        vk::raii::PhysicalDevice _physicalDevice = nullptr;
        DeviceCapabilities _capabilities{};
        
    public: 
        VulkanPhysicalDevice() = default;
        ~VulkanPhysicalDevice() {}

        std::vector<const char*> getRequiredDeviceExtensions() const;
        void pickPhysicalDevice(vk::raii::Instance const& instance, vk::raii::SurfaceKHR const& surface);

        vk::raii::PhysicalDevice const& handle() const
        {
            return _physicalDevice;
        }

        DeviceCapabilities const& capabilities() const
        {
            return _capabilities;
        }

        QueueLayout const& queues() const
        {
            return _capabilities.queues;
        }

        vk::PhysicalDeviceProperties const& properties() const
        {
            return _capabilities.properties;
        }

        vk::PhysicalDeviceMemoryProperties const& memory() const
        {
            return _capabilities.memory;
        }

    private:
        bool isDeviceSuitable(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::SurfaceKHR const& surface) const;
        QueueLookup findQueues(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::SurfaceKHR const& surface) const;
        void buildCapabilities(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::SurfaceKHR const& surface);
    };
}