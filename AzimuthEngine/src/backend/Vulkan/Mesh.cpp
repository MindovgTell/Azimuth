#include "Mesh.hpp"

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

} // namespace azm
