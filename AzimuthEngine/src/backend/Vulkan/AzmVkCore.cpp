#include "AzmVkCore.hpp"


namespace azm::backend 
{

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

    VkCore::VkCore() {}

    VkCore::~VkCore()
    {

    }

    void VkCore::init(const char* pAppName) 
    {
        createInstance(pAppName);
    }

    std::vector<const char*> VkCore::getRequiredDeviceExtensions() const 
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

    void VkCore::createInstance(const char* pAppName)
    {
        const vk::ApplicationInfo appInfo{
            .pApplicationName   = pAppName,
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName        = "No Engine",
			.engineVersion      = VK_MAKE_VERSION(1, 0, 0),
			.apiVersion         = vk::ApiVersion14
        };

        const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
        };

        // Get the required layers
		std::vector<char const*> requiredLayers;
		if(enableValidationLayers) {
			requiredLayers.assign(validationLayers.begin(), validationLayers.end());
		}

        // Check if the required layers are supported by the Vulkan implementation.
		auto layerProperties = _context.enumerateInstanceLayerProperties();
		auto unsupportedLayerIt = std::ranges::find_if(requiredLayers,
													   [&layerProperties](auto const &requiredLayer) {
															return std::ranges::none_of(layerProperties, 
																						[requiredLayer](auto const &layerProperty){return strcmp(layerProperty.layerName, requiredLayer) == 0;});
													    });
        
        if (unsupportedLayerIt != requiredLayers.end()){
            //TODO: Add Logging info about layers and throw exception
            throw std::runtime_error("Required layer not supported: " + std::string(*unsupportedLayerIt));
        }

        // Get required instance extensions
        std::vector<const char*> requiredExtensions = getRequiredDeviceExtensions();
        //Check that all required extensions supported
        auto extensionProperties = _context.enumerateInstanceExtensionProperties();

        auto unsupportedPropertyIt = std::ranges::find_if(requiredExtensions,
								                        [&extensionProperties](auto const &requiredExtension) {
									                        return std::ranges::none_of(extensionProperties,
																                        [requiredExtension](auto const &extensionProperty) {return strcmp(extensionProperty.extensionName, requiredExtension) == 0;});
								                        });
        if(unsupportedPropertyIt != requiredExtensions.end()) {
            //TODO: Add Logging info about layers and throw exception
            throw std::runtime_error("Required extension not supported: " + std::string(*unsupportedPropertyIt));            
        }

        vk::InstanceCreateInfo createInfo{
#ifdef __APPLE__
            .flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR,
#endif
            .pApplicationInfo        = &appInfo,
			.enabledLayerCount		 = static_cast<uint32_t>(requiredLayers.size()),
			.ppEnabledLayerNames	 = requiredLayers.data(),
			.enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
			.ppEnabledExtensionNames = requiredExtensions.data()
        };

        _instance = vk::raii::Instance(_context, createInfo);
    }

        // TODO: Check if it correct way to define this function
        // Also cheng std::cerr to logging file in future
        static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT			severity,
                                                            vk::DebugUtilsMessageTypeFlagsEXT 				type,
                                                            const vk::DebugUtilsMessengerCallbackDataEXT  * 	pCallbackData,
                                                            void * 											pUserData) {
            std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
            return vk::False;
        }

    	void VkCore::setupDebugMessenger() {
            // TODO

            vk::DebugUtilsMessageSeverityFlagsEXT 	severityFlags(
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
            vk::DebugUtilsMessageTypeFlagsEXT		messageTypeFlags(
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
            vk::DebugUtilsMessengerCreateInfoEXT	debugUtilsMessengerCreateInfoEXT{
                .messageSeverity = severityFlags,
                .messageType 	  = messageTypeFlags,
                .pfnUserCallback = &debugCallback};
            
            debugMessenger = _instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
        }

}