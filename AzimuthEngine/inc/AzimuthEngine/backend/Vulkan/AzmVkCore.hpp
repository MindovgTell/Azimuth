#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include "AzmVkDevice.hpp"

#include <stdexcept>
#include <iostream>

namespace azm::backend
{
    class VkCore 
    {
    private:
        // All necessary Vulkan context elements represented by classes
        //TOTO
        vk::raii::Context                     _context;
        vk::raii::Instance                    _instance = VK_NULL_HANDLE;
        vk::raii::DebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

        VkPhysicalDevice                      _physDevice;
        vk::raii::Device                      _device   = VK_NULL_HANDLE;


    public: 
        VkCore();
        ~VkCore();
        void init(const char* pAppName);

    private: 
        // Setup Vulkan Instance
        void createInstance(const char* pAppName);
        std::vector<const char*> getRequiredDeviceExtensions() const;

        // Setup Vulkan Debug messenger
        void setupDebugMessenger();

        
    };    
} // namespace azm
