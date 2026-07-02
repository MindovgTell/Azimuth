#include "VkBuffer.hpp"

#include <cstring>
#include <stdexcept>

namespace azm::backend {

VulkanBuffer::VulkanBuffer(        
    VulkanDevice const& device,
    VulkanPhysicalDevice const& physDevice,
    const void* data,
    vk::raii::CommandPool const& pool,
    vk::DeviceSize size, 
    vk::BufferUsageFlags usage, 
    vk::MemoryPropertyFlags properties) : _size(size)
    {
		auto [stagingBuffer, stagingBufferMemory] = createBuffer(device, physDevice, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		void* stagingData = stagingBufferMemory.mapMemory(0, size);
		memcpy(stagingData, data, size);
		stagingBufferMemory.unmapMemory();

		std::tie(_buffer, _bufferMemory) = createBuffer(device, physDevice, size, usage, properties);

		copyBuffer(device, pool, stagingBuffer, _buffer, size);
	}

void VulkanBuffer::copyBuffer(VulkanDevice const& device, vk::raii::CommandPool const& pool, vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size) {
        vk::CommandBufferAllocateInfo allocInfo{
			.commandPool = pool, 
			.level = vk::CommandBufferLevel::ePrimary, 
			.commandBufferCount = 1
		};
		vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers(device.handle(), allocInfo).front());
		vk::CommandBufferBeginInfo beginInfo{
			.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
		};
		commandBuffer.begin(beginInfo);
		commandBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
		commandBuffer.end();
		vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
		device.queue().submit(submitInfo, nullptr);
		device.queue().waitIdle();
	}
  
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> VulkanBuffer::createBuffer(VulkanDevice const &device, VulkanPhysicalDevice const& physDevice, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
    {
        vk::BufferCreateInfo bufferInfo{ 
			.size = size, 
			.usage = usage, 
			.sharingMode = vk::SharingMode::eExclusive 
		};
		vk::raii::Buffer buffer = vk::raii::Buffer(device.handle(), bufferInfo);
		vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
		vk::MemoryAllocateInfo allocInfo{ 
			.allocationSize = memRequirements.size, 
			.memoryTypeIndex = findMemoryType(physDevice, memRequirements.memoryTypeBits, properties) 
		};
		vk::raii::DeviceMemory bufferMemory = vk::raii::DeviceMemory(device.handle(), allocInfo);
		buffer.bindMemory(*bufferMemory, 0);
		return {std::move(buffer), std::move(bufferMemory)};
    }

} // namespace azm::backend
