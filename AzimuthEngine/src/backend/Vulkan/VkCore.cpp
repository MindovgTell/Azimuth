#include "VkCore.hpp"
#include "core/Utility.hpp"
#include "log/log.hpp"


#include <cassert>
#include <array>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

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



	// Transformations

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

    void VkCore::init(
		const char* pAppName,
		GLFWwindow* window,
		std::span<Vertex const> vertices,
		std::span<uint16_t const> indices,
		std::vector<Object>&& objects)
	{
		_objects = std::move(objects);
        createInstance(pAppName);
        setupDebugMessenger();
		createSurface(window);
        _physicalDevice.pickPhysicalDevice(_instance, _surface);
		_logicalDevice.create(_physicalDevice);
        _swapChain.create(_physicalDevice, _logicalDevice, _surface, window);
		createDescriptorSetLayout();
		_graphicsPipeline.create(_logicalDevice,_swapChain.surfaceFormat().format, findDepthFormat(), _swapChain.extent(), _descriptorSetLayout);
		createCommandPool();
		auto mesh = std::make_shared<Mesh>(_logicalDevice, _physicalDevice, _commandPool, vertices, indices);
		for (auto& object : _objects)
		{
			object._pMesh = mesh;
		}
		createDepthResources();
		createTextureImage();
		createTextureImageView();
		createTextureSampler();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
    }

	void VkCore::setObjectTransform(std::size_t index, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
	{
		if (index >= _objects.size())
			return;

		_objects[index].position = position;
		_objects[index].rotation = rotation;
		_objects[index].scale = scale;
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

	void VkCore::createUniformBuffers(){
		for (auto& gameObject : _objects) {
			gameObject.uniformBuffers.clear();
			gameObject.uniformBuffersMemory.clear();
			gameObject.uniformBuffersMapped.clear();

			// Create uniform buffers for each frame in flight
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
				vk::raii::Buffer buffer({});
				vk::raii::DeviceMemory bufferMem({});
				std::tie(buffer, bufferMem) = createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
							vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
				gameObject.uniformBuffers.emplace_back(std::move(buffer));
				gameObject.uniformBuffersMemory.emplace_back(std::move(bufferMem));
				gameObject.uniformBuffersMapped.emplace_back(gameObject.uniformBuffersMemory[i].mapMemory(0, bufferSize));
			}
		}
	}

void VkCore::createDescriptorSets() {
    // For each game object
    for (auto& gameObject : _objects) {
        // Create descriptor sets for each frame in flight
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *_descriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = *_descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data()
        };

        gameObject.descriptorSets.clear();
        gameObject.descriptorSets = _logicalDevice.handle().allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo bufferInfo{
                .buffer = *gameObject.uniformBuffers[i],
                .offset = 0,
                .range = sizeof(UniformBufferObject)
            };
            vk::DescriptorImageInfo imageInfo{
                .sampler = *_textureSampler,
                .imageView = *_textureImageView,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            };
            std::array descriptorWrites{
                vk::WriteDescriptorSet{
                    .dstSet = *gameObject.descriptorSets[i],
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                    .pBufferInfo = &bufferInfo
                },
                vk::WriteDescriptorSet{
                    .dstSet = *gameObject.descriptorSets[i],
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                    .pImageInfo = &imageInfo
                }
            };
            _logicalDevice.handle().updateDescriptorSets(descriptorWrites, {});
        }
    }
}

	void VkCore::createDescriptorPool() {
		const uint32_t descriptorSetCount = static_cast<uint32_t>(_objects.size()) * MAX_FRAMES_IN_FLIGHT;
		std::array poolSize {
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, descriptorSetCount),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, descriptorSetCount)
		};
		vk::DescriptorPoolCreateInfo poolInfo{
			.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			.maxSets = descriptorSetCount,
			.poolSizeCount = static_cast<uint32_t>(poolSize.size()),
			.pPoolSizes = poolSize.data()
		};
		_descriptorPool = vk::raii::DescriptorPool(_logicalDevice.handle(), poolInfo);
	}

	void VkCore::createDescriptorSetLayout() {
		std::array<vk::DescriptorSetLayoutBinding, 2> bindings{
			{{.binding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eVertex},
			{.binding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = 1, .stageFlags = vk::ShaderStageFlagBits::eFragment}}
		};
		vk::DescriptorSetLayoutCreateInfo layoutInfo{
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings = bindings.data()};
		_descriptorSetLayout = vk::raii::DescriptorSetLayout(_logicalDevice.handle(), layoutInfo);
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

	std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> VkCore::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
		vk::BufferCreateInfo bufferInfo{
			.size = size,
			.usage = usage,
			.sharingMode = vk::SharingMode::eExclusive
		};
		vk::raii::Buffer buffer = vk::raii::Buffer(_logicalDevice.handle(), bufferInfo);
		vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
		};
		vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(_logicalDevice.handle(), allocInfo);
		buffer.bindMemory(*bufferMemory, 0);
		return {std::move(buffer), std::move(bufferMemory)};
	}

	vk::raii::CommandBuffer VkCore::beginSingleTimeCommands() {
		vk::CommandBufferAllocateInfo allocInfo{
			.commandPool = _commandPool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = 1
		};
		vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(_logicalDevice.handle(), allocInfo).front());
		vk::CommandBufferBeginInfo beginInfo{
			.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
		};
		commandBuffer.begin(beginInfo);

		return std::move(commandBuffer);
	}

	void VkCore::endSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer) {
		commandBuffer.end();
		vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
		_logicalDevice.queue().submit(submitInfo, nullptr);
		_logicalDevice.queue().waitIdle();
	}

	void VkCore::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {
    	vk::raii::CommandBuffer commandCopyBuffer = beginSingleTimeCommands();
		commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
		endSingleTimeCommands(std::move(commandCopyBuffer));
	}

	void VkCore::createCommandPool() {
		vk::CommandPoolCreateInfo poolInfo{
			.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		    .queueFamilyIndex = _physicalDevice.queues().idx
		};
		_commandPool = vk::raii::CommandPool(_logicalDevice.handle(), poolInfo);
	}

	void VkCore::createTextureImage() {
		int texWidth, texHeight, texChannels;
		stbi_uc *pixels = stbi_load("AzimuthEngine/assets/textures/texture2.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		vk::DeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels)
		{
			throw std::runtime_error("failed to load texture imgae!");
		}

		auto [stagingBuffer, stagingBufferMemory] =
    	createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* data = stagingBufferMemory.mapMemory(0, imageSize);
		memcpy(data,pixels,imageSize);
		stagingBufferMemory.unmapMemory();
		stbi_image_free(pixels);

		std::tie(_textureImage, _textureImageMemory) = createImage(
			texWidth,
			texHeight,
			vk::Format::eR8G8B8A8Srgb,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal);
			vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommands();
			transitionImageLayout(commandBuffer, _textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
			copyBufferToImage(commandBuffer, stagingBuffer, _textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
			transitionImageLayout(commandBuffer, _textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
			endSingleTimeCommands(std::move(commandBuffer));
	}

	std::pair<vk::raii::Image, vk::raii::DeviceMemory> VkCore::createImage(
		uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties
	) {
		vk::ImageCreateInfo imageInfo {
			.imageType 	= vk::ImageType::e2D,
			.format		= format,
			.extent		= {width, height, 1},
			.mipLevels 	= 1,
			.arrayLayers = 1,
			.samples	= vk::SampleCountFlagBits::e1,
			.tiling		= tiling,
			.usage 		= usage,
			.sharingMode	= vk::SharingMode::eExclusive
		};
		vk::raii::Image image = vk::raii::Image(_logicalDevice.handle(), imageInfo);

		vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo {
			.allocationSize  = memRequirements.size,
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
		};
		vk::raii::DeviceMemory imageMemory = vk::raii::DeviceMemory(_logicalDevice.handle(), allocInfo);
		image.bindMemory(imageMemory, 0);

		return {std::move(image), std::move(imageMemory)};
	}

	void VkCore::copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height)
	{
		vk::BufferImageCopy region{
						   .bufferOffset      = 0,
                           .bufferRowLength   = 0,
                           .bufferImageHeight = 0,
                           .imageSubresource  = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
                           .imageOffset       = {0, 0, 0},
                           .imageExtent       = {width, height, 1}
						};
		commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
	}

	void VkCore::transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Image &image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
		vk::ImageMemoryBarrier barrier{
							   .oldLayout           = oldLayout,
                               .newLayout           = newLayout,
                               .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                               .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                               .image               = image,
                               .subresourceRange    = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = 1}
							};
		vk::PipelineStageFlags sourceStage;
		vk::PipelineStageFlags destinationStage;

		if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal)
		{
			barrier.srcAccessMask = {};
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			sourceStage      = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			sourceStage      = vk::PipelineStageFlagBits::eTransfer;
			destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else
		{
			throw std::invalid_argument("unsupported layout transition!");
		}
		commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
	}

	void VkCore::createTextureImageView() {
		_textureImageView = _logicalDevice.createImageView(*_textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
	}

	void VkCore::createTextureSampler() {
		vk::PhysicalDeviceProperties properties = _physicalDevice.handle().getProperties();
		vk::SamplerCreateInfo   samplerInfo{
			.magFilter        = vk::Filter::eLinear,
			.minFilter        = vk::Filter::eLinear,
			.mipmapMode       = vk::SamplerMipmapMode::eLinear,
			.addressModeU     = vk::SamplerAddressMode::eRepeat,
			.addressModeV     = vk::SamplerAddressMode::eRepeat,
			.addressModeW     = vk::SamplerAddressMode::eRepeat,
			.anisotropyEnable = vk::True,
			.maxAnisotropy    = properties.limits.maxSamplerAnisotropy,
			.compareEnable    = vk::False,
            .compareOp        = vk::CompareOp::eAlways
		};

		_textureSampler = vk::raii::Sampler(_logicalDevice.handle(), samplerInfo);
	}

	bool VkCore::hasStencilComponent(vk::Format format) {
		return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
	}

	vk::Format VkCore::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
		for (const auto format : candidates) {
			vk::FormatProperties props = _physicalDevice.handle().getFormatProperties(format);

			if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}

		throw std::runtime_error("failed to find supported format!");
	}

	vk::Format VkCore::findDepthFormat() {
		return findSupportedFormat(
			{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment
		);
	}

    void VkCore::createDepthResources()
    {
		vk::Format depthFormat = findDepthFormat();
		std::tie(_depthImage, _depthImageMemory) = createImage(_swapChain.extent().width, _swapChain.extent().height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal);
		_depthImageView = _logicalDevice.createImageView(*_depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
    }

    void VkCore::recordCommandBuffer(uint32_t imageIndex)
	{
		auto& commandBuffer = _commandBuffers[_frameIndex];
		commandBuffer.begin({});

		// Before starting rendering, transition the swapchain image to vk::ImageLayout::eColorAttachmentOptimal
		transition_image_layout(
		    _swapChain.images()[imageIndex],
		    vk::ImageLayout::eUndefined,
		    vk::ImageLayout::eColorAttachmentOptimal,
		    {},                                                        // srcAccessMask (no need to wait for previous operations)
		    vk::AccessFlagBits2::eColorAttachmentWrite,                // dstAccessMask
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,         // dstStage
			vk::ImageAspectFlagBits::eColor
		);

		// New transition for the depth image
		transition_image_layout(
			*_depthImage,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthAttachmentOptimal,
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
			vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
			vk::ImageAspectFlagBits::eDepth
		);
		vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
		vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
		vk::RenderingAttachmentInfo attachmentInfo = {
		    .imageView   = _swapChain.imageViews()[imageIndex],
		    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		    .loadOp      = vk::AttachmentLoadOp::eClear,
		    .storeOp     = vk::AttachmentStoreOp::eStore,
		    .clearValue  = clearColor
		};

		vk::RenderingAttachmentInfo depthAttachmentInfo = {
			.imageView   = _depthImageView,
			.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
			.loadOp      = vk::AttachmentLoadOp::eClear,
			.storeOp     = vk::AttachmentStoreOp::eDontCare,
			.clearValue  = clearDepth
		};

		vk::RenderingInfo renderingInfo = {
		    .renderArea           = {.offset = {0, 0}, .extent = _swapChain.extent()},
		    .layerCount           = 1,
		    .colorAttachmentCount = 1,
		    .pColorAttachments    = &attachmentInfo,
			.pDepthAttachment     = &depthAttachmentInfo
		};

		commandBuffer.beginRendering(renderingInfo);
		_graphicsPipeline.bind(commandBuffer);
		commandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(_swapChain.extent().width), static_cast<float>(_swapChain.extent().height), 0.0f, 1.0f));
		commandBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), _swapChain.extent()));

		for (const auto& gameObject : _objects) {
			if (!gameObject._pMesh) {
				continue;
			}

			commandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				*_graphicsPipeline.layout(),
				0,
				*gameObject.descriptorSets[_frameIndex],
				{}
			);

			gameObject._pMesh->bind(commandBuffer);
			gameObject._pMesh->draw(commandBuffer);
		}


		commandBuffer.endRendering();

		// After rendering, transition the swapchain image to vk::ImageLayout::ePresentSrcKHR
		transition_image_layout(
		    _swapChain.images()[imageIndex],
		    vk::ImageLayout::eColorAttachmentOptimal,
		    vk::ImageLayout::ePresentSrcKHR,
		    vk::AccessFlagBits2::eColorAttachmentWrite,                // srcAccessMask
		    {},                                                        // dstAccessMask
		    vk::PipelineStageFlagBits2::eColorAttachmentOutput,        // srcStage
		    vk::PipelineStageFlagBits2::eBottomOfPipe,                  // dstStage
			vk::ImageAspectFlagBits::eColor
		);
		commandBuffer.end();
	}

	void VkCore::transition_image_layout(
	    vk::Image               image,
	    vk::ImageLayout         old_layout,
	    vk::ImageLayout         new_layout,
	    vk::AccessFlags2        src_access_mask,
	    vk::AccessFlags2        dst_access_mask,
	    vk::PipelineStageFlags2 src_stage_mask,
	    vk::PipelineStageFlags2 dst_stage_mask,
		vk::ImageAspectFlags    image_aspect_flags)
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
		    .image               = image,
			.subresourceRange    = {
				.aspectMask     = image_aspect_flags,
				.baseMipLevel   = 0,
				.levelCount     = 1,
				.baseArrayLayer = 0,
				.layerCount     = 1}
		};
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


		updateUniformBuffers(_frameIndex);

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

	void VkCore::updateUniformBuffers(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float>(currentTime - startTime).count();

		// Camera and projection matrices (shared by all objects)
		glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 6.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(glm::radians(45.0f),
										static_cast<float>(_swapChain.extent().width) / static_cast<float>(_swapChain.extent().height),
										0.1f, 20.0f);
		proj[1][1] *= -1; // Flip Y for Vulkan

		// Update uniform buffers for each object
		for (auto& gameObject : _objects) {
			// Get the model matrix for this object
			glm::mat4 initialRotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			glm::mat4 model = gameObject.getModelMatrix() * initialRotation;

			// Create and update the UBO
			UniformBufferObject ubo{
				.model = model,
				.view = view,
				.proj = proj
			};

			// Copy the UBO data to the mapped memory
			memcpy(gameObject.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
		}
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
		_swapChain.clear();
		_swapChain.create(_physicalDevice,_logicalDevice,_surface, window);
		createDepthResources();
	}

} // namespace azm::backend
