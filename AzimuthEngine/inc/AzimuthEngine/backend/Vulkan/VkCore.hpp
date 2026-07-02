#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>

#include "VkDevice.hpp"
#include "VkPhysicalDevice.hpp"
#include "VkSwapChain.hpp"
#include "VkGraphicsPipeline.hpp"
#include "Mesh.hpp"

namespace azm::backend
{

    class VkCore 
    {
    private:
        // All necessary Vulkan context elements represented by classes
        //TOTO
        vk::raii::Context                     _context;
        vk::raii::Instance                    _instance = nullptr;
        vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
        
        vk::raii::SurfaceKHR                  _surface  = nullptr;
        // Device setup
        VulkanPhysicalDevice                  _physicalDevice;
        VulkanDevice                          _logicalDevice;
        // Swapchain setup 
        VulkanSwapChain                       _swapChain;
        // Graphics pipeline
        VulkanGraphicsPipeline                _graphicsPipeline;

        // Command buffers
        vk::raii::CommandPool    _commandPool 	   = nullptr;
        std::vector<vk::raii::CommandBuffer>  _commandBuffers;
        // Synchronization primitieves
        std::vector<vk::raii::Semaphore> 	 _presentCompleteSemaphores;
        std::vector<vk::raii::Semaphore> 	 _renderFinishedSemaphores;
        std::vector<vk::raii::Fence> 		 _inFlightFences;
        uint32_t                             _frameIndex = 0;
        bool _framebufferResized = false;

        vk::raii::DescriptorSetLayout   _descriptorSetLayout = nullptr;
        vk::raii::DescriptorPool        _descriptorPool      = nullptr;

        // Objects own descriptor sets, so they must be destroyed before the descriptor pool.
        std::vector<Object> _objects;

        // Textures
        vk::raii::Image         _textureImage       = nullptr;
        vk::raii::DeviceMemory  _textureImageMemory = nullptr;
        vk::raii::ImageView     _textureImageView   = nullptr;
        vk::raii::Sampler       _textureSampler     = nullptr;

        vk::raii::Image _depthImage = nullptr;
        vk::raii::DeviceMemory _depthImageMemory = nullptr;
        vk::raii::ImageView _depthImageView = nullptr;



    public: 
        VkCore()  = default;
        ~VkCore() = default;
        void init(
            const char* pAppName,
            GLFWwindow* window,
            std::span<Vertex const> vertices,
            std::span<uint16_t const> indices,
            std::vector<Object>&& objects);
        void drawFrame(GLFWwindow* window);
        void notifyFramebufferResized();
        void waitIdle();

    private: 
        // Setup Vulkan Instance
        void createInstance(const char* pAppName);
        std::vector<const char*> getRequiredInstanceExtensions() const;

        // Setup Vulkan Debug messenger
        void setupDebugMessenger();

        // Surface
        void createSurface(GLFWwindow* window);

        void createDescriptorSetLayout();

        // Command buffer
        void createCommandBuffers();
        void createCommandPool();
        void recordCommandBuffer(uint32_t imageIndex);
        void createTextureImage();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();
        
        void transition_image_layout(
            vk::Image               image,
            vk::ImageLayout         old_layout,
            vk::ImageLayout         new_layout,
            vk::AccessFlags2        src_access_mask,
            vk::AccessFlags2        dst_access_mask,
            vk::PipelineStageFlags2 src_stage_mask,
            vk::PipelineStageFlags2 dst_stage_mask,
            vk::ImageAspectFlags    image_aspect_flags);

        // Synchronization
        void createSyncObjects();

        void recreateSwapChain(GLFWwindow* window);

        // Vertex buffer
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
        std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        void copyBuffer(vk::raii::Buffer & srcBuffer, vk::raii::Buffer & dstBuffer, vk::DeviceSize size);
        void endSingleTimeCommands(vk::raii::CommandBuffer&& commandBuffer);
        vk::raii::CommandBuffer beginSingleTimeCommands();
        void updateUniformBuffers(uint32_t currentImage);

        //Texture
        std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
        void transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Image &image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
        void copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height);
        void createTextureImageView();

        void createTextureSampler();
        void createDepthResources();
        vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
        vk::Format findDepthFormat();
        bool hasStencilComponent(vk::Format format);
    };    
} // namespace azm
