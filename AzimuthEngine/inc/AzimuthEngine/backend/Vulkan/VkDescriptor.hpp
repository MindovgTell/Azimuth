#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include "VkDevice.hpp"

namespace azm::backend {

class VulkanDescriptorLayout {

};

class VulkanDescriptorSet {
private:
    vk::raii::DescriptorSetLayout   _descriptorSetLayout = nullptr;
    vk::raii::DescriptorPool        _descriptorPool      = nullptr;
    std::vector<vk::raii::DescriptorSet> _descriptorSets;
public:
    void create(VulkanDevice const& device);
};

}
