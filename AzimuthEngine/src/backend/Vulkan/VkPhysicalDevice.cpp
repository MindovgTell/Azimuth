#include "AzmVkPhysDevice.hpp"
#include "log/log.hpp"

namespace azm::backend
{

	DEFINE_LOG_CATEGORY_STATIC(PhysicalDeviceLog);

	std::vector<const char*> VulkanPhysicalDevice::getRequiredDeviceExtensions() const 
    {
        constexpr const char* kPortabilitySubsetExtensionName = "VK_KHR_portability_subset";
		std::vector<const char*> extensions = {
			vk::KHRSwapchainExtensionName
		};

// TODO: Later add support for another OS, currently MacOS
// #ifdef _WIN32
//         extensions.push_back();
// #endif
// #ifdef __linux__

// #endif

#ifdef __APPLE__
		extensions.push_back(kPortabilitySubsetExtensionName);
#endif

        return extensions;
    }

	void VulkanPhysicalDevice::pickPhysicalDevice(vk::raii::Instance const& instance, vk::raii::SurfaceKHR const& surface) {
		std::vector<vk::raii::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
		auto const devIter = std::ranges::find_if(physicalDevices, [&]( auto const & physicalDevice ) { return isDeviceSuitable( physicalDevice, surface); } );
		if ( devIter == physicalDevices.end() )
		{
			AZM_LOG(PhysicalDeviceLog, Error, "failed to find a suitable GPU");
			throw std::runtime_error( "failed to find a suitable GPU!" );
		}

		_physicalDevice = std::move(*devIter);
		buildCapabilities(_physicalDevice, surface);
	}

	QueueLookup VulkanPhysicalDevice::findQueues(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::SurfaceKHR const& surface) const
	{
		QueueLookup indices;
		std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();
		// get the first index into queueFamilyProperties which supports both graphics and present
		for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); ++qfpIndex) {
			auto const& family = queueFamilyProperties[qfpIndex];
			if ((family.queueFlags & vk::QueueFlagBits::eGraphics) &&
				physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface)) {
				indices.idx = qfpIndex;
				break;
			}
		}
		return indices;
	}

	bool VulkanPhysicalDevice::isDeviceSuitable(const vk::raii::PhysicalDevice& physicalDevice, vk::raii::SurfaceKHR const& surface) const {
		// Check Vulkan 1.3 support 
		bool supportsVulkan1_3 = physicalDevice.getProperties().apiVersion >= vk::ApiVersion13;
		
		// Check if any of the queue families support graphics operations
		QueueLookup queueLookup = findQueues(physicalDevice, surface);
    	bool supportsRequiredQueueFamily = queueLookup.complete();

		// Check if all required physicalDevice extensions are available
		auto availableDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
		auto requiredDeviceExtensions  = getRequiredDeviceExtensions();

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
		return supportsVulkan1_3 && 
			   supportsRequiredQueueFamily  && 
			   supportsAllRequiredExtensions && 
			   supportsRequiredFeatures;		
	}

	void VulkanPhysicalDevice::buildCapabilities(vk::raii::PhysicalDevice const& physicalDevice, vk::raii::SurfaceKHR const& surface)
	{
		QueueLookup queueLookup = findQueues(physicalDevice, surface);
		if (!queueLookup.complete())
		{
			AZM_LOG(PhysicalDeviceLog, Error, "selected GPU does not provide required queue family");
			throw std::runtime_error("selected GPU does not provide required queue family");
		}

    auto features = physicalDevice.getFeatures2<
        vk::PhysicalDeviceFeatures2,
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();

		_capabilities.properties = physicalDevice.getProperties();
		_capabilities.memory = physicalDevice.getMemoryProperties();
		_capabilities.queues = QueueLayout{
			.idx = *queueLookup.idx
		};
		_capabilities.shaderDrawParameters =
			features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters;
		_capabilities.dynamicRendering =
			features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering;
		_capabilities.extendedDynamicState =
			features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

	}

}


/*
auto queues = findQueues(physicalDevice, surface);

    if (!queues.complete())
    {
        return false;
    }
*/