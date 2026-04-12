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
        vk::raii::CommandBuffer  _commandBuffer    = nullptr;
        // Synchronization primitieves
        vk::raii::Semaphore 	 _presentCompleteSemaphore = nullptr;
        vk::raii::Semaphore 	 _renderFinishedSemaphore  = nullptr;
        vk::raii::Fence 		 _drawFence 				  = nullptr;

    public: 
        VkCore()  = default;
        ~VkCore() = default;
        void init(const char* pAppName, GLFWwindow* window);
        void drawFrame();
        
    private: 
        // Setup Vulkan Instance
        void createInstance(const char* pAppName);
        std::vector<const char*> getRequiredInstanceExtensions() const;

        // Setup Vulkan Debug messenger
        void setupDebugMessenger();

        // Surface
        void createSurface(GLFWwindow* window);

        void createImageViews();

        void createGraphicsPipeline();

        vk::raii::ShaderModule createShaderModule(const std::vector<char>& code) const;

        // Command buffer
        void createCommandBuffer();
        void createCommandPool();
        void recordCommandBuffer(uint32_t imageIndex);
        
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
    };    
} // namespace azm
