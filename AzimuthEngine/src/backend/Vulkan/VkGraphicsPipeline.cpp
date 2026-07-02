#include "VkGraphicsPipeline.hpp"
#include "Utility.hpp"
#include "Mesh.hpp"

namespace azm::backend {

    [[nodiscard]] vk::raii::ShaderModule VulkanGraphicsPipeline::createShaderModule(VulkanDevice const& device,const std::vector<char>& code) const {
        vk::ShaderModuleCreateInfo createInfo{ 
			.codeSize = code.size() * sizeof(char), 
			.pCode = reinterpret_cast<const uint32_t*>(code.data())
		};
		vk::raii::ShaderModule shaderModule{device.handle(), createInfo};
		return shaderModule;
    }

    void VulkanGraphicsPipeline::create(
        VulkanDevice const& device, 
        vk::Format colorFormat,
        vk::Format depthFormat,
        vk::Extent2D extent,
        vk::raii::DescriptorSetLayout const& descriptorSetLayout)
	{
		vk::raii::ShaderModule shaderModule = createShaderModule(device, readFile("build/shaders/VulkanEngine.spv"));
		
		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ 
			.stage = vk::ShaderStageFlagBits::eVertex, 
			.module = shaderModule,  
			.pName = "vertMain" 
		};

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ 
			.stage = vk::ShaderStageFlagBits::eFragment, 
			.module = shaderModule, 
			.pName = "fragMain" 
		};

		vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
			
		auto bindingDescription    = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		vk::PipelineVertexInputStateCreateInfo   vertexInputInfo{
			.vertexBindingDescriptionCount   = 1,
			.pVertexBindingDescriptions      = &bindingDescription,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
			.pVertexAttributeDescriptions    = attributeDescriptions.data()};
		
		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
			.topology = vk::PrimitiveTopology::eTriangleList
		};

		vk::Viewport viewport{0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f};
		vk::Rect2D scissor{vk::Offset2D{ 0, 0 }, extent};
		vk::PipelineViewportStateCreateInfo viewportState{
			.viewportCount = 1, 
			.pViewports = &viewport, 
			.scissorCount = 1, 
			.pScissors = &scissor
		};

		// Setting rasterizer
		vk::PipelineRasterizationStateCreateInfo rasterizer{
			.depthClampEnable        = vk::False,
			.rasterizerDiscardEnable = vk::False,
			.polygonMode             = vk::PolygonMode::eFill,
			.cullMode                = vk::CullModeFlagBits::eBack,
			.frontFace               = vk::FrontFace::eCounterClockwise,
			.depthBiasEnable         = vk::False,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp          = 0.0f,
			.depthBiasSlopeFactor    = 0.0f,
			.lineWidth               = 1.0f
		};

		vk::PipelineMultisampleStateCreateInfo multisampling{
			.rasterizationSamples = vk::SampleCountFlagBits::e1, 
			.sampleShadingEnable = vk::False
		};

		vk::PipelineColorBlendAttachmentState colorBlendAttachment{
    		.blendEnable    = vk::False,
    		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

		vk::PipelineColorBlendStateCreateInfo colorBlending{
    		.logicOpEnable = vk::False, 
			.logicOp = vk::LogicOp::eCopy, 
			.attachmentCount = 1, 
			.pAttachments = &colorBlendAttachment
		};

		std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

		vk::PipelineDynamicStateCreateInfo dynamicState{
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), 
			.pDynamicStates = dynamicStates.data()
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ 
			.setLayoutCount = 1, 
			.pSetLayouts = &*descriptorSetLayout, 
			.pushConstantRangeCount = 0 
		};

		_layout = vk::raii::PipelineLayout(device.handle(), pipelineLayoutInfo);

		vk::PipelineDepthStencilStateCreateInfo depthStencil{
			.depthTestEnable       = vk::True,
			.depthWriteEnable      = vk::True,
			.depthCompareOp        = vk::CompareOp::eLess,
			.depthBoundsTestEnable = vk::False,
			.stencilTestEnable     = vk::False
		};

		vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
			{.stageCount          = 2,
			.pStages             = shaderStages,
			.pVertexInputState   = &vertexInputInfo,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState      = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState   = &multisampling,
			.pColorBlendState    = &colorBlending,
			.pDynamicState       = &dynamicState,
			.pDepthStencilState  = &depthStencil,
			.layout              = _layout,
			.renderPass          = nullptr},
			{.colorAttachmentCount = 1, .pColorAttachmentFormats = &colorFormat, .depthAttachmentFormat = depthFormat}
		};
		
		_pipeline = vk::raii::Pipeline(device.handle(), nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
	}

    void VulkanGraphicsPipeline::bind(vk::raii::CommandBuffer const &commandBuffer) const
    {
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline);
    }

} // namespace azm::backend
