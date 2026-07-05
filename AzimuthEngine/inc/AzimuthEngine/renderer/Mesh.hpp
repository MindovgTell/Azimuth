#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstddef>

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "VkBuffer.hpp"

namespace azm {

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription()
    {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
        };
    }
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};

MeshData createCubeMesh();

class Mesh {
private:
    azm::backend::VulkanBuffer _vertexBuffer;
    azm::backend::VulkanBuffer _indexBuffer;
    uint32_t _indexCount = 0;
public:

    Mesh(
        azm::backend::VulkanDevice const& device,
        azm::backend::VulkanPhysicalDevice const& physicalDevice,
        vk::raii::CommandPool const& commandPool,
        std::span<Vertex const> vertices,
        std::span<uint16_t const> indices);

    Mesh(Mesh const&) = delete;
    Mesh& operator=(Mesh const&) = delete;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void bind(vk::raii::CommandBuffer const& commandBuffer) const;
    void draw(vk::raii::CommandBuffer const& commandBuffer) const;

    uint32_t indexCount() const { return _indexCount; }

private:
    void createVertexBuffer(
        azm::backend::VulkanDevice const& device,
        azm::backend::VulkanPhysicalDevice const& physicalDevice,
        vk::raii::CommandPool const& commandPool,
        std::span<Vertex const> vertices);

    void createIndexBuffer(
        azm::backend::VulkanDevice const& device,
        azm::backend::VulkanPhysicalDevice const& physicalDevice,
        vk::raii::CommandPool const& commandPool,
        std::span<uint16_t const> indices);
};

class Object {
public:
    std::shared_ptr<Mesh> _pMesh = nullptr;
    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};
    // Uniform buffer for this object (one per frame in flight)
    std::vector<vk::raii::Buffer> uniformBuffers;
    std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    // Descriptor sets for this object (one per frame in flight)
    std::vector<vk::raii::DescriptorSet> descriptorSets;

    Object() = default;
    Object(Object const&) = delete;
    Object& operator=(Object const&) = delete;
    Object(Object&&) noexcept = default;
    Object& operator=(Object&&) noexcept = default;


    // Calculate model matrix based on position, rotation, and scale
    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }
};

}
