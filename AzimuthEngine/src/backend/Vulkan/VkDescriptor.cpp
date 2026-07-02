#include "VkDescriptor.hpp"



namespace azm::backend
{
	constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    void VulkanDescriptorSet::create(VulkanDevice const& device) {
		std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *_descriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo {
			.descriptorPool = _descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(layouts.size()),
			.pSetLayouts = layouts.data()
		};

		_descriptorSets.clear();
		_descriptorSets = device.handle().allocateDescriptorSets(allocInfo);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vk::DescriptorBufferInfo bufferInfo{
				.buffer = _uniformBuffers[i],
				.offset = 0,
				.range = sizeof(UniformBufferObject)
			};
    		vk::DescriptorImageInfo  imageInfo{
				.sampler = _textureSampler,
				.imageView = _textureImageView,
				.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
			};

			std::array<vk::WriteDescriptorSet, 2> descriptorWrites{{
				{
					.dstSet          = _descriptorSets[i],
					.dstBinding      = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType  = vk::DescriptorType::eUniformBuffer,
					.pBufferInfo     = &bufferInfo
				},
				{
					.dstSet          = _descriptorSets[i],
					.dstBinding      = 1,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType  = vk::DescriptorType::eCombinedImageSampler,
					.pImageInfo      = &imageInfo
				}}
			};

			device.handle().updateDescriptorSets(descriptorWrites, {});
		}

	}
} // namespace azm::backend
