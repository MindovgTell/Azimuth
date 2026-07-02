#pragma once

#include "VkDevice.hpp"

namespace azm::backend {

class VulkanGraphicsPipeline
{
private:
    vk::raii::PipelineLayout _layout = nullptr;
    vk::raii::Pipeline _pipeline = nullptr;
    vk::raii::DescriptorSetLayout _descriptorSetLayout = nullptr;
public:
    void create(
        VulkanDevice const& device,
        vk::Format colorFormat,
        vk::Format depthFormat,
        vk::Extent2D extent,
        vk::raii::DescriptorSetLayout const& descriptorSetLayout);

    void bind(vk::raii::CommandBuffer const& commandBuffer) const;

    vk::raii::PipelineLayout const& layout() const {return _layout;}
    vk::raii::Pipeline const& handle() const {return _pipeline;}

private:
    vk::raii::ShaderModule createShaderModule(VulkanDevice const& device, const std::vector<char>& code) const;
};

} // namespace azm::backend
