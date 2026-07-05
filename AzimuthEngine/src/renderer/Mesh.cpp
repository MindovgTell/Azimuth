#include "renderer/Mesh.hpp"

#include <limits>
#include <stdexcept>

namespace azm {

    Mesh::Mesh(
        azm::backend::VulkanDevice const& device,
        azm::backend::VulkanPhysicalDevice const& physicalDevice,
        vk::raii::CommandPool const& commandPool,
        std::span<Vertex const> vertices,
        std::span<uint16_t const> indices)
    {
        if (vertices.empty())
        {
            throw std::invalid_argument("Cannot create a mesh without vertices");
        }
        if (indices.empty())
        {
            throw std::invalid_argument("Cannot create a mesh without indices");
        }
        if (indices.size() > std::numeric_limits<uint32_t>::max())
        {
            throw std::overflow_error("Mesh index count exceeds Vulkan drawIndexed limit");
        }

        createVertexBuffer(device, physicalDevice, commandPool, vertices);
        createIndexBuffer(device, physicalDevice, commandPool, indices);
        _indexCount = static_cast<uint32_t>(indices.size());
    }

    void Mesh::bind(vk::raii::CommandBuffer const& commandBuffer) const
	{
        commandBuffer.bindVertexBuffers(0, *_vertexBuffer.handle(), {0});
        commandBuffer.bindIndexBuffer(*_indexBuffer.handle(), 0, vk::IndexType::eUint16);
	}

    void Mesh::draw(vk::raii::CommandBuffer const& commandBuffer) const
    {
        commandBuffer.drawIndexed(_indexCount, 1, 0, 0, 0);
    }

    void Mesh::createVertexBuffer(
        azm::backend::VulkanDevice const& device,
        azm::backend::VulkanPhysicalDevice const& physicalDevice,
        vk::raii::CommandPool const& commandPool,
        std::span<Vertex const> vertices)
	{
		vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
        _vertexBuffer = azm::backend::VulkanBuffer(
            device,
            physicalDevice,
            vertices.data(),
            commandPool,
            bufferSize,
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal);
	}

    void Mesh::createIndexBuffer(
        azm::backend::VulkanDevice const& device,
        azm::backend::VulkanPhysicalDevice const& physicalDevice,
        vk::raii::CommandPool const& commandPool,
        std::span<uint16_t const> indices)
    {
		vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        _indexBuffer = azm::backend::VulkanBuffer(
            device,
            physicalDevice,
            indices.data(),
            commandPool,
            bufferSize,
            vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::MemoryPropertyFlagBits::eDeviceLocal);
    }



    MeshData createCubeMesh()
    {
        const std::vector<Vertex> vertices = {
            // Front face (+Z)
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 0
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 1
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 2
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 3

            // Back face (-Z)
            {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 4
            {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 5
            {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 6
            {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 7

            // Right face (+X)
            {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 8
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 9
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 10
            {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 11

            // Left face (-X)
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 12
            {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 13
            {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 14
            {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 15

            // Top face (+Y)
            {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 16
            {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 17
            {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 18
            {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 19

            // Bottom face (-Y)
            {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 20
            {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 21
            {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 22
            {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}  // 23
        };

        const std::vector<uint16_t> indices = {
            0,  1,  2,   2,  3,  0,   // Front
            4,  5,  6,   6,  7,  4,   // Back
            8,  9, 10,  10, 11,  8,   // Right
            12, 13, 14,  14, 15, 12,   // Left
            16, 17, 18,  18, 19, 16,   // Top
            20, 21, 22,  22, 23, 20    // Bottom
        };

        return {
            .vertices = vertices,
            .indices = indices
        };
    }

} // namespace azm
