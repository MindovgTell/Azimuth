#include "AzmVkLogicalDevice.hpp"
#include "AzmVkPhysDevice.hpp"

#include <stdexcept>

namespace azm::backend
{
    void VulkanLogicalDevice::create(VulkanPhysicalDevice const& physicalDevice)
    {

        if (physicalDevice.handle() == nullptr)
        {
            throw std::runtime_error("Cannot create logical device: physical device is invalid");
        }

        // query for Vulkan 1.3 features
		vk::StructureChain<
			vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceVulkan11Features,
			vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
			{},
			{.shaderDrawParameters = true},
			{
				.dynamicRendering = true,
				.synchronization2 = true
			},
			{.extendedDynamicState = true}
		};

		auto requiredDeviceExtensions = physicalDevice.getRequiredDeviceExtensions();
		float queuePriority = 1.0f;		

        uint32_t queueIdx = physicalDevice.queues().idx;

		vk::DeviceQueueCreateInfo deviceQueueCreateInfo { 
            .queueFamilyIndex = queueIdx, 
            .queueCount = 1, 
            .pQueuePriorities = &queuePriority };

		vk::DeviceCreateInfo deviceCreateInfo {
			.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
			.queueCreateInfoCount = 1,
			.pQueueCreateInfos = &deviceQueueCreateInfo,
			.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
			.ppEnabledExtensionNames = requiredDeviceExtensions.data()
		};

		_device = vk::raii::Device(physicalDevice.handle(), deviceCreateInfo);
		_queue = vk::raii::Queue(_device, queueIdx, 0);
    }
    
}
