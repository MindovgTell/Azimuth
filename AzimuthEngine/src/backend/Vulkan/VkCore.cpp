#include "AzmVkCore.hpp"
#include "core/Utility.hpp"
#include "log/log.hpp"

#include "AzmVkPhysDevice.hpp"
#include "AzmVkLogicalDevice.hpp"
#include "AzmVkSwapChain.hpp"

#include <cassert>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

namespace azm::backend 
{

    DEFINE_LOG_CATEGORY_STATIC(ValidationLayerLog);
	constexpr int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

	//Temporary solution to put vertecies logic in VkCore
	struct Vertex
	{
		glm::vec2 pos;
		glm::vec3 color;

		static vk::VertexInputBindingDescription getBindingDescription()
		{
			return {.binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex};
		}

		static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions()
		{
			return {{{.location = 0, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, pos)},
               {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)}}};
		}
	};

	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};

	// Transformations

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

    void VkCore::init(const char* pAppName, GLFWwindow* window) 
    {
        createInstance(pAppName);
        setupDebugMessenger();
		createSurface(window);
        _physicalDevice.pickPhysicalDevice(_instance, _surface);
		_logicalDevice.create(_physicalDevice);
        _swapChain.create(_physicalDevice, _logicalDevice, _surface, window);
		createImageViews();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createCommandPool();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
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

	void VkCore::createUniformBuffers(){
		_uniformBuffers.clear();
		_uniformBuffersMemory.clear();
		_uniformBuffersMapped.clear();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
			vk::raii::Buffer buffer({});
			vk::raii::DeviceMemory bufferMem({});
			createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, bufferMem);
			_uniformBuffers.emplace_back(std::move(buffer));
			_uniformBuffersMemory.emplace_back(std::move(bufferMem));
			_uniformBuffersMapped.emplace_back( _uniformBuffersMemory[i].mapMemory(0, bufferSize));
		}
	}

	void VkCore::createDescriptorSets() {
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *_descriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo {
			.descriptorPool = _descriptorPool, 
			.descriptorSetCount = static_cast<uint32_t>(layouts.size()), 
			.pSetLayouts = layouts.data()
		};

		_descriptorSets.clear();
		_descriptorSets = _logicalDevice.handle().allocateDescriptorSets(allocInfo);
		
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vk::DescriptorBufferInfo bufferInfo{ 
				.buffer = _uniformBuffers[i], 
				.offset = 0, 
				.range = sizeof(UniformBufferObject)
			};

			vk::WriteDescriptorSet descriptorWrite{ 
				.dstSet = _descriptorSets[i], 
				.dstBinding = 0,
				.dstArrayElement = 0, 
				.descriptorCount = 1, 
				.descriptorType = vk::DescriptorType::eUniformBuffer, 
				.pBufferInfo = &bufferInfo 
			};

			_logicalDevice.handle().updateDescriptorSets(descriptorWrite, {});
		}
	
	}

	void VkCore::createDescriptorPool() {
		vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT);
		vk::DescriptorPoolCreateInfo poolInfo{ 
			.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 
			.maxSets = MAX_FRAMES_IN_FLIGHT, 
			.poolSizeCount = 1, 
			.pPoolSizes = &poolSize
		};

		_descriptorPool = vk::raii::DescriptorPool(_logicalDevice.handle(), poolInfo);
	}

	void VkCore::createDescriptorSetLayout() {
		vk::DescriptorSetLayoutBinding uboLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);
		vk::DescriptorSetLayoutCreateInfo layoutInfo{
			.bindingCount = 1, 
			.pBindings = &uboLayoutBinding
		};
		_descriptorSetLayout = vk::raii::DescriptorSetLayout(_logicalDevice.handle(), layoutInfo);
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
			
		auto                                     bindingDescription    = Vertex::getBindingDescription();
		auto                                     attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo   vertexInputInfo{.vertexBindingDescriptionCount   = 1,
																.pVertexBindingDescriptions      = &bindingDescription,
																.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
																.pVertexAttributeDescriptions    = attributeDescriptions.data()};
		
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
			.frontFace               = vk::FrontFace::eCounterClockwise,
			.depthBiasEnable         = vk::False,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp          = 0.0f,
			.depthBiasSlopeFactor    = 0.0f,
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


		// vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
		// 	.setLayoutCount = 0, 
		// 	.pushConstantRangeCount = 0
		// };
		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ 
			.setLayoutCount = 1, 
			.pSetLayouts = &*_descriptorSetLayout, 
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

	void VkCore::createCommandBuffers() {
		vk::CommandBufferAllocateInfo allocInfo{ 
			.commandPool = _commandPool, 
			.level = vk::CommandBufferLevel::ePrimary, 
			.commandBufferCount = MAX_FRAMES_IN_FLIGHT 
		};

		_commandBuffers = std::move(vk::raii::CommandBuffers(_logicalDevice.handle(), allocInfo));
	}

	uint32_t VkCore::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
	{
		vk::PhysicalDeviceMemoryProperties memProperties = _physicalDevice.handle().getMemoryProperties();
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void VkCore::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory) {
		vk::BufferCreateInfo bufferInfo{ .size = size, .usage = usage, .sharingMode = vk::SharingMode::eExclusive };
		buffer = vk::raii::Buffer(_logicalDevice.handle(), bufferInfo);
		vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{ .allocationSize = memRequirements.size, .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties) };
		bufferMemory = vk::raii::DeviceMemory(_logicalDevice.handle(), allocInfo);
		buffer.bindMemory(*bufferMemory, 0);
	}

	void VkCore::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {
		vk::CommandBufferAllocateInfo allocInfo{ 
			.commandPool = _commandPool, 
			.level = vk::CommandBufferLevel::ePrimary, 
			.commandBufferCount = 1 
		};
    	vk::raii::CommandBuffer commandCopyBuffer = std::move(_logicalDevice.handle().allocateCommandBuffers(allocInfo).front());
		commandCopyBuffer.begin(vk::CommandBufferBeginInfo { 
			.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit 
		});
		commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
		commandCopyBuffer.end();

		_logicalDevice.queue().submit(
			vk::SubmitInfo{ 
				.commandBufferCount = 1, 
				.pCommandBuffers = &*commandCopyBuffer 
			}, 
			nullptr);
		_logicalDevice.queue().waitIdle();
	}

	void VkCore::createVertexBuffer()
	{
		vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		vk::BufferCreateInfo stagingInfo {
			.size = bufferSize,
			.usage = vk::BufferUsageFlagBits::eTransferSrc, 
			.sharingMode = vk::SharingMode::eExclusive 
		};
		vk::raii::Buffer stagingBuffer(_logicalDevice.handle(), stagingInfo);
		vk::MemoryRequirements memRequirementsStaging = stagingBuffer.getMemoryRequirements();
    	vk::MemoryAllocateInfo memoryAllocateInfoStaging{  
			.allocationSize = memRequirementsStaging.size, 
			.memoryTypeIndex = findMemoryType(memRequirementsStaging.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent) 
		};
    	vk::raii::DeviceMemory stagingBufferMemory(_logicalDevice.handle(), memoryAllocateInfoStaging);

   	 	stagingBuffer.bindMemory(stagingBufferMemory, 0);
		void* dataStaging = stagingBufferMemory.mapMemory(0, stagingInfo.size);
		memcpy(dataStaging, vertices.data(), stagingInfo.size);
		stagingBufferMemory.unmapMemory();

		vk::BufferCreateInfo bufferInfo{ 
			.size = bufferSize,  
			.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, 
			.sharingMode = vk::SharingMode::eExclusive 
		};
		_vertexBuffer = vk::raii::Buffer(_logicalDevice.handle(), bufferInfo);

		vk::MemoryRequirements memRequirements = _vertexBuffer.getMemoryRequirements();
		vk::MemoryAllocateInfo memoryAllocateInfo{  
			.allocationSize = memRequirements.size, 
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal) };
		_vertexBufferMemory = vk::raii::DeviceMemory( _logicalDevice.handle(), memoryAllocateInfo );

		_vertexBuffer.bindMemory( *_vertexBufferMemory, 0 );

		copyBuffer(stagingBuffer, _vertexBuffer, stagingInfo.size);
	}

	void VkCore::createIndexBuffer() {
		vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		vk::raii::Buffer stagingBuffer({});
		vk::raii::DeviceMemory stagingBufferMemory({});
		createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

		void* data = stagingBufferMemory.mapMemory(0, bufferSize);
		memcpy(data, indices.data(), (size_t) bufferSize);
		stagingBufferMemory.unmapMemory();

		createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, _indexBuffer, _indexBufferMemory);

		copyBuffer(stagingBuffer, _indexBuffer, bufferSize);
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
		_commandBuffers[_frameIndex].begin({});

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

		_commandBuffers[_frameIndex].beginRendering(renderingInfo);
		_commandBuffers[_frameIndex].bindPipeline(vk::PipelineBindPoint::eGraphics, *_graphicsPipeline);
		_commandBuffers[_frameIndex].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(_swapChain.extent().width), static_cast<float>(_swapChain.extent().height), 0.0f, 1.0f));
		_commandBuffers[_frameIndex].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swapChain.extent()));
		_commandBuffers[_frameIndex].bindVertexBuffers(0, *_vertexBuffer, {0});
		_commandBuffers[_frameIndex].bindIndexBuffer(*_indexBuffer, 0, vk::IndexType::eUint16);
		_commandBuffers[_frameIndex].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _pipelineLayout, 0, *_descriptorSets[_frameIndex], nullptr);
		_commandBuffers[_frameIndex].drawIndexed(indices.size(), 1, 0, 0, 0);
		_commandBuffers[_frameIndex].endRendering();

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
		_commandBuffers[_frameIndex].end();
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
		_commandBuffers[_frameIndex].pipelineBarrier2(dependency_info);
	}

	void VkCore::createSyncObjects()
	{
		assert(_presentCompleteSemaphores.empty() && _renderFinishedSemaphores.empty() && _inFlightFences.empty());
		
		for (size_t i = 0; i < _swapChain.images().size(); i++)
		{
			_renderFinishedSemaphores.emplace_back(_logicalDevice.handle(), vk::SemaphoreCreateInfo());
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			_presentCompleteSemaphores.emplace_back(_logicalDevice.handle(), vk::SemaphoreCreateInfo());
			_inFlightFences.emplace_back(_logicalDevice.handle(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
		}	
	}

	void VkCore::drawFrame(GLFWwindow* window)
	{
		auto fenceResult = _logicalDevice.handle().waitForFences(*_inFlightFences[_frameIndex], vk::True, UINT64_MAX);
		if (fenceResult != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to wait for fence!");
		}
		_logicalDevice.handle().resetFences(*_inFlightFences[_frameIndex]);

		auto [result, imageIndex] = _swapChain.handle().acquireNextImage(UINT64_MAX, *_presentCompleteSemaphores[_frameIndex], nullptr);

		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			recreateSwapChain(window);
			return;
		}
		if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
		{
			assert(result == vk::Result::eTimeout || result == vk::Result::eNotReady);
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		_logicalDevice.handle().resetFences(*_inFlightFences[_frameIndex]);

		_commandBuffers[_frameIndex].reset();
		recordCommandBuffer(imageIndex);

		_logicalDevice.queue().waitIdle();        // NOTE: for simplicity, wait for the queue to be idle before starting the frame


		updateUniformBuffer(_frameIndex);

		vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
		const vk::SubmitInfo   submitInfo{
			.waitSemaphoreCount   = 1,
		    .pWaitSemaphores      = &*_presentCompleteSemaphores[_frameIndex],
			.pWaitDstStageMask    = &waitDestinationStageMask,
			.commandBufferCount   = 1,
			.pCommandBuffers      = &*_commandBuffers[_frameIndex],
			.signalSemaphoreCount = 1,
			.pSignalSemaphores    = &*_renderFinishedSemaphores[imageIndex]
		};
		_logicalDevice.queue().submit(submitInfo, *_inFlightFences[_frameIndex]);

		const vk::PresentInfoKHR presentInfoKHR{
			.waitSemaphoreCount = 1, 
			.pWaitSemaphores = &*_renderFinishedSemaphores[imageIndex], 
			.swapchainCount = 1, 
			.pSwapchains = &*_swapChain.handle(), 
			.pImageIndices = &imageIndex
		};
		result = _logicalDevice.queue().presentKHR(presentInfoKHR);
		if ((result == vk::Result::eSuboptimalKHR) || (result == vk::Result::eErrorOutOfDateKHR) || _framebufferResized)
		{
			_framebufferResized = false;
			recreateSwapChain(window);
		}	
		else
		{
		// There are no other success codes than eSuccess; on any error code, presentKHR already threw an exception.
			assert(result == vk::Result::eSuccess);
		}
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

		_frameIndex = (_frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void VkCore::updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(_swapChain.extent().width) / static_cast<float>(_swapChain.extent().height), 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;
		memcpy(_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}

	void VkCore::waitIdle()
	{
		if (_logicalDevice.handle() == nullptr)
		{
			return;
		}

		_logicalDevice.handle().waitIdle();
	}

	void VkCore::notifyFramebufferResized()
	{
		_framebufferResized = true;
	}

	void VkCore::recreateSwapChain(GLFWwindow* window)
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		_logicalDevice.handle().waitIdle();
		cleanupSwapChain();
		_swapChain.create(_physicalDevice,_logicalDevice,_surface, window);
		createImageViews();
	}

	// Later better to push it into VulkanSwapChain class
	void VkCore::cleanupSwapChain()
	{
		_swapChainImageViews.clear();
		_swapChain.handle() = nullptr;
	}
}
