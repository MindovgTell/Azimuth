#include "AzmVkCore.hpp"
#include "core/Utility.hpp"
#include "log/log.hpp"

#include "AzmVkPhysDevice.hpp"
#include "AzmVkLogicalDevice.hpp"
#include "AzmVkSwapChain.hpp"

#include "core/Utility.hpp"

namespace azm::backend 
{

    DEFINE_LOG_CATEGORY_STATIC(ValidationLayerLog);

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif


    void VkCore::init(const char* pAppName, GLFWwindow* window) 
    {
        createInstance(pAppName);
        setupDebugMessenger();
		createSurface(window);
        _physicalDevice.pickPhysicalDevice(_instance, _surface);
		_logicalDevice.create(_physicalDevice);
        _swapChain.create(_physicalDevice, _logicalDevice, _surface, window);
		createImageViews();
		createGraphicsPipeline();
		createCommandPool();
		createCommandBuffer();
		createSyncObjects();
    }

    std::vector<const char*> VkCore::getRequiredInstanceExtensions() const {
		uint32_t glfwExtensionCount = 0;
		auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		if (enableValidationLayers) {
			extensions.push_back(vk::EXTDebugUtilsExtensionName);
		}

#ifdef __APPLE__
		// Required for Vulkan portability implementations on macOS.
		extensions.push_back(vk::KHRPortabilityEnumerationExtensionName);
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
        std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();
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
                                                        vk::DebugUtilsMessageTypeFlagsEXT 				    type,
                                                        const vk::DebugUtilsMessengerCallbackDataEXT*    	pCallbackData,
                                                        void * 											    pUserData) {
        AZM_LOG(ValidationLayerLog, Error, "validation layer: type {}, msg: {}", to_string(type), pCallbackData->pMessage);
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

	void VkCore::createSurface(GLFWwindow* window) {
		VkSurfaceKHR 	surface;
		if (glfwCreateWindowSurface(*_instance, window, nullptr, &surface) != 0) {
			throw std::runtime_error("failed to create window surface!");
		}
		_surface = vk::raii::SurfaceKHR(_instance, surface);
	}

	void VkCore::createImageViews() {
		assert(_swapChainImageViews.empty());
		vk::ImageViewCreateInfo imageViewCreateInfo{
			.viewType = vk::ImageViewType::e2D,
			.format	  = _swapChain.surfaceFormat().format,
			.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
		};

		for (auto &image : _swapChain.images()) {
			imageViewCreateInfo.image = image;
			_swapChainImageViews.emplace_back(_logicalDevice.handle(), imageViewCreateInfo);
		}
	}

	[[nodiscard]] vk::raii::ShaderModule VkCore::createShaderModule(const std::vector<char>& code) const {
		vk::ShaderModuleCreateInfo createInfo{ 
			.codeSize = code.size() * sizeof(char), 
			.pCode = reinterpret_cast<const uint32_t*>(code.data())
		};
		vk::raii::ShaderModule shaderModule{_logicalDevice.handle(), createInfo};
		return shaderModule;
	}

	void VkCore::createGraphicsPipeline()
	{
		vk::raii::ShaderModule shaderModule = createShaderModule(readFile("build/shaders/VulkanEngine.spv"));
		
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ 
			.stage = vk::ShaderStageFlagBits::eVertex, 
			.module = shaderModule,  
			.pName = "vertMain" 
		};

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ 
			.stage = vk::ShaderStageFlagBits::eFragment, 
			.module = shaderModule, 
			.pName = "fragMain" 
		};

		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
			
		vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
		
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
			.topology = vk::PrimitiveTopology::eTriangleList
		};

		vk::Viewport viewport{0.0f, 0.0f, static_cast<float>(_swapChain.extent().width), static_cast<float>(_swapChain.extent().height), 0.0f, 1.0f};
		vk::Rect2D scissor{vk::Offset2D{ 0, 0 }, _swapChain.extent()};
		vk::PipelineViewportStateCreateInfo viewportState{
			.viewportCount = 1, 
			.pViewports = &viewport, 
			.scissorCount = 1, 
			.pScissors = &scissor
		};

		// Setting rasterizer
		vk::PipelineRasterizationStateCreateInfo rasterizer{
			.depthClampEnable        = vk::False,
			.rasterizerDiscardEnable = vk::False,
            .polygonMode             = vk::PolygonMode::eFill,
			.cullMode                = vk::CullModeFlagBits::eBack,
			.frontFace               = vk::FrontFace::eClockwise,
			.depthBiasEnable         = vk::False,
			.lineWidth               = 1.0f
		};

		vk::PipelineMultisampleStateCreateInfo multisampling{
			.rasterizationSamples = vk::SampleCountFlagBits::e1, 
			.sampleShadingEnable = vk::False
		};

		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
    		.blendEnable    = vk::False,
    		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

		vk::PipelineColorBlendStateCreateInfo colorBlending{
    		.logicOpEnable = vk::False, 
			.logicOp = vk::LogicOp::eCopy, 
			.attachmentCount = 1, 
			.pAttachments = &colorBlendAttachment
		};

		std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

		vk::PipelineDynamicStateCreateInfo dynamicState{
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), 
			.pDynamicStates = dynamicStates.data()
		};


		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
			.setLayoutCount = 0, 
			.pushConstantRangeCount = 0
		};
		_pipelineLayout = vk::raii::PipelineLayout(_logicalDevice.handle(), pipelineLayoutInfo);

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
			{.stageCount          = 2,
			.pStages             = shaderStages,
			.pVertexInputState   = &vertexInputInfo,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState      = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState   = &multisampling,
			.pColorBlendState    = &colorBlending,
			.pDynamicState       = &dynamicState,
			.layout              = _pipelineLayout,
			.renderPass          = nullptr},
			{.colorAttachmentCount = 1, .pColorAttachmentFormats = &_swapChain.surfaceFormat().format}
		};
		
		_graphicsPipeline = vk::raii::Pipeline(_logicalDevice.handle(), nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
	}

	void VkCore::createCommandBuffer() {
		vk::CommandBufferAllocateInfo allocInfo{ 
			.commandPool = _commandPool, 
			.level = vk::CommandBufferLevel::ePrimary, 
			.commandBufferCount = 1 
		};

		_commandBuffer = std::move(vk::raii::CommandBuffers(_logicalDevice.handle(), allocInfo).front());
	}

	void VkCore::createCommandPool() {
		vk::CommandPoolCreateInfo poolInfo{
			.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		    .queueFamilyIndex = _physicalDevice.queues().idx
		};
		_commandPool = vk::raii::CommandPool(_logicalDevice.handle(), poolInfo);
	}

	void VkCore::recordCommandBuffer(uint32_t imageIndex)
	{
		_commandBuffer.begin({});

		// Before starting rendering, transition the swapchain image to vk::ImageLayout::eColorAttachmentOptimal
		transition_image_layout(
		    imageIndex,
		    vk::ImageLayout::eUndefined,
		    vk::ImageLayout::eColorAttachmentOptimal,
		    {},                                                        // srcAccessMask (no need to wait for previous operations)
		    vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput         // dstStage
		);
		vk::ClearValue              clearColor     = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
		vk::RenderingAttachmentInfo attachmentInfo = {
		    .imageView   = _swapChainImageViews[imageIndex],
		    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		    .loadOp      = vk::AttachmentLoadOp::eClear,
		    .storeOp     = vk::AttachmentStoreOp::eStore,
		    .clearValue  = clearColor};
		vk::RenderingInfo renderingInfo = {
		    .renderArea           = {.offset = {0, 0}, .extent = _swapChain.extent()},
		    .layerCount           = 1,
		    .colorAttachmentCount = 1,
		    .pColorAttachments    = &attachmentInfo};

		_commandBuffer.beginRendering(renderingInfo);
		_commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_graphicsPipeline);
		_commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(_swapChain.extent().width), static_cast<float>(_swapChain.extent().height), 0.0f, 1.0f));
		_commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swapChain.extent()));
		_commandBuffer.draw(3, 1, 0, 0);
		_commandBuffer.endRendering();

		// After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
		transition_image_layout(
		    imageIndex,
		    vk::ImageLayout::eColorAttachmentOptimal,
		    vk::ImageLayout::ePresentSrcKHR,
		    vk::AccessFlagBits2::eColorAttachmentWrite,                // srcAccessMask
		    {},                                                        // dstAccessMask
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
		    vk::PipelineStageFlagBits2::eBottomOfPipe                  // dstStage
		);
		_commandBuffer.end();
	}

	void VkCore::transition_image_layout(
	    uint32_t                imageIndex,
	    vk::ImageLayout         old_layout,
	    vk::ImageLayout         new_layout,
	    vk::AccessFlags2        src_access_mask,
	    vk::AccessFlags2        dst_access_mask,
	    vk::PipelineStageFlags2 src_stage_mask,
	    vk::PipelineStageFlags2 dst_stage_mask)
	{
		vk::ImageMemoryBarrier2 barrier = {
		    .srcStageMask        = src_stage_mask,
		    .srcAccessMask       = src_access_mask,
		    .dstStageMask        = dst_stage_mask,
		    .dstAccessMask       = dst_access_mask,
		    .oldLayout           = old_layout,
		    .newLayout           = new_layout,
		    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		    .image               = _swapChain.images()[imageIndex],
		    .subresourceRange    = {
		           .aspectMask     = vk::ImageAspectFlagBits::eColor,
		           .baseMipLevel   = 0,
		           .levelCount     = 1,
		           .baseArrayLayer = 0,
		           .layerCount     = 1}};
		vk::DependencyInfo dependency_info = {
		    .dependencyFlags         = {},
		    .imageMemoryBarrierCount = 1,
		    .pImageMemoryBarriers    = &barrier};
		_commandBuffer.pipelineBarrier2(dependency_info);
	}

	void VkCore::createSyncObjects()
	{
		_presentCompleteSemaphore = vk::raii::Semaphore(_logicalDevice.handle(), vk::SemaphoreCreateInfo());
		_renderFinishedSemaphore  = vk::raii::Semaphore(_logicalDevice.handle(), vk::SemaphoreCreateInfo());
		_drawFence                = vk::raii::Fence(_logicalDevice.handle(), {.flags = vk::FenceCreateFlagBits::eSignaled});
	}

	void VkCore::drawFrame()
	{
		auto fenceResult = _logicalDevice.handle().waitForFences(*_drawFence, vk::True, UINT64_MAX);
		if (fenceResult != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to wait for fence!");
		}
		_logicalDevice.handle().resetFences(*_drawFence);

		auto [result, imageIndex] = _swapChain.handle().acquireNextImage(UINT64_MAX, *_presentCompleteSemaphore, nullptr);

		recordCommandBuffer(imageIndex);

		_logicalDevice.queue().waitIdle();        // NOTE: for simplicity, wait for the queue to be idle before starting the frame

		vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo   submitInfo{.waitSemaphoreCount   = 1,
		                                  .pWaitSemaphores      = &*_presentCompleteSemaphore,
		                                  .pWaitDstStageMask    = &waitDestinationStageMask,
		                                  .commandBufferCount   = 1,
		                                  .pCommandBuffers      = &*_commandBuffer,
		                                  .signalSemaphoreCount = 1,
		                                  .pSignalSemaphores    = &*_renderFinishedSemaphore};
		_logicalDevice.queue().submit(submitInfo, *_drawFence);

		const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1, .pWaitSemaphores = &*_renderFinishedSemaphore, .swapchainCount = 1, .pSwapchains = &*_swapChain.handle(), .pImageIndices = &imageIndex};
		result = _logicalDevice.queue().presentKHR(presentInfoKHR);
		switch (result)
		{
			case vk::Result::eSuccess:
				break;
			case vk::Result::eSuboptimalKHR:
				std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n";
				break;
			default:
				break;        // an unexpected result is returned!
		}
	}
}
