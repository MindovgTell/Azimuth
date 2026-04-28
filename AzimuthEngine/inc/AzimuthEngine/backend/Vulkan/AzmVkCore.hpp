#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>

#include "AzmVkLogicalDevice.hpp"
#include "AzmVkPhysDevice.hpp"
#include "AzmVkSwapChain.hpp"

#include <stdexcept>
#include <iostream>
#include <vector>

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
        VulkanLogicalDevice                   _logicalDevice;
        // Swapchain setup 
        VulkanSwapChain                       _swapChain;
        std::vector<vk::raii::ImageView> _swapChainImageViews;
        // Graphics pipeline
        vk::raii::PipelineLayout _pipelineLayout   = nullptr;
        vk::raii::Pipeline 		 _graphicsPipeline = nullptr;
        // Command buffers
        vk::raii::CommandPool    _commandPool 	   = nullptr;
        std::vector<vk::raii::CommandBuffer>  _commandBuffers;
        // Synchronization primitieves
        std::vector<vk::raii::Semaphore> 	 _presentCompleteSemaphores;
        std::vector<vk::raii::Semaphore> 	 _renderFinishedSemaphores;
        std::vector<vk::raii::Fence> 		 _inFlightFences;
        uint32_t                             _frameIndex = 0;
        bool _framebufferResized = false;
        // Vertex buffer
        vk::raii::Buffer        _vertexBuffer       = nullptr;
        vk::raii::DeviceMemory  _vertexBufferMemory = nullptr;
        // Index buffer
        vk::raii::Buffer        _indexBuffer        = nullptr;
        vk::raii::DeviceMemory  _indexBufferMemory  = nullptr;        

        std::vector<vk::raii::Buffer> _uniformBuffers;
        std::vector<vk::raii::DeviceMemory> _uniformBuffersMemory;
        std::vector<void*> _uniformBuffersMapped;

        vk::raii::DescriptorSetLayout   _descriptorSetLayout = nullptr;
        vk::raii::DescriptorPool        _descriptorPool      = nullptr;
        std::vector<vk::raii::DescriptorSet> _descriptorSets;

    public: 
        VkCore()  = default;
        ~VkCore() = default;
        void init(const char* pAppName, GLFWwindow* window);
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

        void createImageViews();

        void createDescriptorSetLayout();

        void createGraphicsPipeline();

        vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;

        // Command buffer
        void createCommandBuffers();
        void createCommandPool();
        void recordCommandBuffer(uint32_t imageIndex);
        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();
        
        void transition_image_layout(
            uint32_t                imageIndex,
            vk::ImageLayout         old_layout,
            vk::ImageLayout         new_layout,
            vk::AccessFlags2        src_access_mask,
            vk::AccessFlags2        dst_access_mask,
            vk::PipelineStageFlags2 src_stage_mask,
            vk::PipelineStageFlags2 dst_stage_mask);

        // Synchronization
        void createSyncObjects();

        void recreateSwapChain(GLFWwindow* window);
        void cleanupSwapChain();

        // Vertex buffer
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
        void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory);
        void copyBuffer(vk::raii::Buffer & srcBuffer, vk::raii::Buffer & dstBuffer, vk::DeviceSize size);
    
        void updateUniformBuffer(uint32_t currentImage);
    };    
} // namespace azm
