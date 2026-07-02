#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

namespace azm::backend {

class VulkanInstance {
private:
    vk::raii::Context                     _context;
    vk::raii::Instance                    _instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
public:


};

} // namespace azm::backend