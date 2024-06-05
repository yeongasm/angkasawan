#include "vulkan/vkgpu.h"

namespace gpu
{
auto RasterPipeline::info() const -> RasterPipelineInfo const&
{
	return m_info;
}

Pipeline::Pipeline(Device& device) :
	DeviceResource{ device },
	m_pipelineVariant{},
	m_type{}
{}

auto Pipeline::type() const -> PipelineType
{
	return m_type;
}

auto Pipeline::valid() const -> bool
{
	auto&& self = *static_cast<vk::PipelineImpl const*>(this);

	return m_device != nullptr && self.handle != VK_NULL_HANDLE;
}

auto Pipeline::from(Device& device, PipelineShaderInfo const& pipelineShaderInfo, RasterPipelineInfo&& info) -> Resource<Pipeline>
{
	// It is necessary for raster pipelines to have a vertex shader and fragment shader.
	if (!pipelineShaderInfo.vertexShader.valid() ||
		!pipelineShaderInfo.pixelShader.valid())
	{
		return null_resource;
	}

	vk::DeviceImpl& vkdevice = *static_cast<vk::DeviceImpl*>(&device);

	vk::ShaderImpl const& vertexShader = static_cast<vk::ShaderImpl const&>(*pipelineShaderInfo.vertexShader);
	vk::ShaderImpl const& pixelShader  = static_cast<vk::ShaderImpl const&>(*pipelineShaderInfo.pixelShader);

	// Vertex shader.
	VkPipelineShaderStageCreateInfo vertexStage{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = vertexShader.stage,
		.module = vertexShader.handle,
		.pName = vertexShader.info().entryPoint.c_str()
	};

	// Pixel shader.
	VkPipelineShaderStageCreateInfo pixelShadingStage{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = pixelShader.stage,
		.module = pixelShader.handle,
		.pName = pixelShader.info().entryPoint.c_str()
	};

	std::array shaderStages = { vertexStage, pixelShadingStage };

	uint32 numBindings = 0;
	uint32 numAttributes = 0;
	std::array<VkVertexInputBindingDescription, 32> inputBindings{};
	std::array<VkVertexInputAttributeDescription, 64> attributeDescriptions{};

	for (VertexInputBinding const& input : info.vertexInputBindings)
	{
		inputBindings[numBindings] = {
			.binding = numBindings,
			.stride = input.stride,
			.inputRate = (input.instanced) ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX
		};
		if (!pipelineShaderInfo.vertexInputAttrib.empty())
		{
			uint32 stride = 0;
			for (uint32 i = input.from; i <= input.to; ++i)
			{
				auto&& attribute = pipelineShaderInfo.vertexInputAttrib[i];
				attributeDescriptions[numAttributes] = {
					.location = attribute.location,
					.binding = numBindings,
					.format = vk::translate_shader_attrib_format(attribute.format),
					.offset = stride,
				};
				stride += vk::stride_for_shader_attrib_format(attribute.format);
				++numAttributes;
			}
		}
		++numBindings;
	}

	VkPipelineVertexInputStateCreateInfo pipelineVertexState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = numBindings,
		.pVertexBindingDescriptions = inputBindings.data(),
		.vertexAttributeDescriptionCount = numAttributes,
		.pVertexAttributeDescriptions = attributeDescriptions.data(),
	};

	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssembly{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = vk::translate_topology(info.topology),
		.primitiveRestartEnable = VK_FALSE
	};

	// NOTE(Afiq):
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkGraphicsPipelineCreateInfo.html
	// The vulkan spec states that a VkPipelineViewportStateCreateInfo is unnecessary for pipelines with dynamic states "VK_DYNAMIC_STATE_VIEWPORT" and "VK_DYNAMIC_STATE_SCISSOR".
	/*VkViewport viewport{ 0.f, 0.f, info.viewport.width, info.viewport.height, 0.f, 1.0f };

	VkRect2D scissor{
		.offset = {
			.x = info.scissor.offset.x,
			.y = info.scissor.offset.y
		},
		.extent = {
			.width = info.scissor.extent.width,
			.height = info.scissor.extent.height
		}
	};*/

	VkPipelineViewportStateCreateInfo pipelineViewportState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1
	};

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo pipelineDynamicState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = states
	};

	VkPipelineRasterizationStateCreateInfo pipelineRasterState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = info.rasterization.enableDepthClamp,
		.polygonMode = vk::translate_polygon_mode(info.rasterization.polygonalMode),
		.cullMode = vk::translate_cull_mode(info.rasterization.cullMode),
		.frontFace = vk::translate_front_face_dir(info.rasterization.frontFace),
		.lineWidth = 1.f
	};

	VkPipelineMultisampleStateCreateInfo pipelineMultisampleSate{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 0.f
	};

	size_t attachmentCount = info.colorAttachments.size();
	ASSERTION(
		((attachmentCount <= vkdevice.properties.limits.maxColorAttachments) && (attachmentCount < 32)) &&
		"attachmentCount must not exceed the device's maxColorAttachments limit. If the device can afford more, we limit to 32."
	);
	std::array<VkPipelineColorBlendAttachmentState, 32> colorAttachmentBlendStates{};
	std::array<VkFormat, 32> colorAttachmentFormats{};

	for (size_t i = 0; i < attachmentCount; ++i)
	{
		auto const& attachment = info.colorAttachments[i];
		colorAttachmentBlendStates[i] = {
			.blendEnable = attachment.blendInfo.enable ? VK_TRUE : VK_FALSE,
			.srcColorBlendFactor = vk::translate_blend_factor(attachment.blendInfo.srcColorBlendFactor),
			.dstColorBlendFactor = vk::translate_blend_factor(attachment.blendInfo.dstColorBlendFactor),
			.colorBlendOp = vk::translate_blend_op(attachment.blendInfo.colorBlendOp),
			.srcAlphaBlendFactor = vk::translate_blend_factor(attachment.blendInfo.srcAlphaBlendFactor),
			.dstAlphaBlendFactor = vk::translate_blend_factor(attachment.blendInfo.dstAlphaBlendFactor),
			.alphaBlendOp = vk::translate_blend_op(attachment.blendInfo.alphaBlendOp),
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};
		colorAttachmentFormats[i] = vk::translate_format(attachment.format);
	}

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_CLEAR,
		.attachmentCount = static_cast<uint32>(attachmentCount),
		.pAttachments = colorAttachmentBlendStates.data(),
		.blendConstants = { 1.f, 1.f, 1.f, 1.f }
	};

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = info.depthTest.enableDepthTest,
		.depthWriteEnable = info.depthTest.enableDepthWrite,
		.depthCompareOp = vk::translate_compare_op(info.depthTest.depthTestCompareOp),
		.depthBoundsTestEnable = info.depthTest.enableDepthBoundsTest,
		.minDepthBounds = info.depthTest.minDepthBounds,
		.maxDepthBounds = info.depthTest.maxDepthBounds
	};

	VkPipelineRenderingCreateInfo pipelineRendering{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
		.colorAttachmentCount = static_cast<uint32>(attachmentCount),
		.pColorAttachmentFormats = colorAttachmentFormats.data(),
		.depthAttachmentFormat = vk::translate_format(info.depthAttachmentFormat),
		.stencilAttachmentFormat = vk::translate_format(info.stencilAttachmentFormat)
	};

	VkPipelineLayout layoutHandle = vkdevice.push_constant_pipeline_layout(info.pushConstantSize, vkdevice.properties.limits.maxPushConstantsSize);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &pipelineRendering,
		.stageCount = static_cast<uint32>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.pVertexInputState = &pipelineVertexState,
		.pInputAssemblyState = &pipelineInputAssembly,
		.pTessellationState = nullptr,
		.pViewportState = &pipelineViewportState,
		.pRasterizationState = &pipelineRasterState,
		.pMultisampleState = &pipelineMultisampleSate,
		.pDepthStencilState = &pipelineDepthStencilState,
		.pColorBlendState = &pipelineColorBlendState,
		.pDynamicState = &pipelineDynamicState,
		.layout = layoutHandle,
		.renderPass = nullptr,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = 0
	};

	VkPipeline handle = VK_NULL_HANDLE;

	// TODO(Afiq):
	// The only way this can fail is when we run out of host / device memory OR shader linkage has failed.
	VkResult result = vkCreateGraphicsPipelines(vkdevice.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &handle);

	if (result != VK_SUCCESS)
	{
		return null_resource;
	}

	auto&& [id, vkpipeline] = vkdevice.gpuResourcePool.pipelines.emplace(vkdevice);

	if (!info.name.empty())
	{
		info.name.format("<pipeline.rasterization>:{}", info.name.c_str());
	}

	vkpipeline.handle = handle;
	vkpipeline.layout = layoutHandle;
	vkpipeline.m_type = PipelineType::Rasterization;

	RasterPipeline& rasterPipeline = vkpipeline.m_pipelineVariant.emplace<RasterPipeline>();

	rasterPipeline.m_info = std::move(info);

	if constexpr (ENABLE_DEBUG_RESOURCE_NAMES)
	{
		vkdevice.setup_debug_name(vkpipeline);
	}

	return Resource<Pipeline>{ id.to_uint64(), vkpipeline };
}

auto Pipeline::destroy(Pipeline& resource, uint64 id) -> void
{
	/*
	 * At this point, the ref count on the resource is only 1 which means ONLY the resource has a reference to itself and can be safely deleted.
	 */
	vk::DeviceImpl& vkdevice = *static_cast<vk::DeviceImpl*>(resource.m_device);

	std::lock_guard const lock{ vkdevice.gpuResourcePool.zombieMutex };

	uint64 const cpuTimelineValue = vkdevice.cpu_timeline();

	vkdevice.gpuResourcePool.zombies.emplace_back(cpuTimelineValue, id, vk::ResourceType::Pipeline);
}

namespace vk
{
PipelineImpl::PipelineImpl(DeviceImpl& device) :
	Pipeline{ device }
{}
}
}