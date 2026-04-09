#include "AzmVkDevice.hpp"

namespace azm::backend
{
	void pickPhysicalDevice(const vk::raii::Instance &instance) {
		std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
		auto const devIter = std::ranges::find_if( physicalDevices, [&]( auto const & physicalDevice ) { return isDeviceSuitable( physicalDevice ); } );
		if ( devIter == physicalDevices.end() )
		{
			throw std::runtime_error( "failed to find a suitable GPU!" );
		}
		physicalDevice = *devIter;
	}

	bool isDeviceSuitable(vk::raii::PhysicalDevice const &physicalDevice) {
		// Check Vulkan 1.3 support 
		bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;
		
		// Check if any of the queue families support graphics operations
		auto queueFamilies	= physicalDevice.getQueueFamilyProperties();
		bool supportsGraphics = std::ranges::any_of(queueFamilies, [](auto const &qfp){ return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics);});

		// Check if all required physicalDevice extensions are available
		auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
		auto requiredDeviceExtensions = getRequiredDeviceExtensions();
		bool supportsAllRequiredExtensions = 
			std::ranges::all_of(requiredDeviceExtensions,
								 [&availableDeviceExtensions](auto const &requiredDeviceExtension) {
									return std::ranges::any_of(availableDeviceExtensions,
															   [requiredDeviceExtension](auto const &availableDeviceExtension) {
																	return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0;
															   });
								 });
		
		// Check if the physicalDevice supports the required features
		auto features = physicalDevice.template getFeatures2<
			vk::PhysicalDeviceFeatures2,
			vk::PhysicalDeviceVulkan11Features,
			vk::PhysicalDeviceVulkan13Features,
			vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
								 
		bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
										features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
										features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
		//Return true if the physicalDevice meets all the criteria
		return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;		
	}
}