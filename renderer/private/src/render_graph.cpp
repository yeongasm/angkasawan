#include "graphs/dag.h"
#include "render_graph.h"
#include "containers/string.h"
#include "task_queue.h"

namespace gpu
{

namespace graph
{

template <typename... T>
void log([[maybe_unused]] std::string* debugOutput, [[maybe_unused]] fmt::format_string<T...> fmt, [[maybe_unused]] T&&... args)
{
#if RENDERER_ENABLE_LOGGING
	if (debugOutput)
	{
		(*debugOutput).append(fmt::format(fmt, std::forward<T>(args)...));
	}
#endif
}

bool Pass::operator==(const Pass& rhs) const
{
	return name == rhs.name;
}

bool Pass::operator!=(const Pass& rhs) const
{
	return name != rhs.name;
}

RenderPassAttachment* RenderGraph::get_attachment(std::string_view name)
{
	if (!m_context->store->attachments.contains(name))
	{
		return nullptr;
	}
	return &m_context->store->attachments[name];;
}

// Stores the render pass attachment into the graph, but does not create the actual image resource yet.
RenderPassAttachment* RenderGraph::register_attachment_internal(RenderPassAttachmentInfo const& info, std::string* debugOutput)
{
	if (get_attachment(info.name))
	{
		log(
			debugOutput, 
			"Existing attachment name \"{}\" supplied in [Raster|Compute]PassInfo.attachments. Attachment names must be unique. Skipping.\n", 
			info.name
		);
		return nullptr;
	}

	Image imageResource{ .info = info.info.value().imageInfo };
	resource::Entry<Image> entry = m_context->store->resourceStore.store_image(std::move(imageResource));

	ftl::HashStringView name = m_context->store->stringPool.append(info.name);

	RenderPassAttachment attachment{
		.name		= name,
		.image		= entry.resource.get(),
		.blendInfo	= info.info.value().blendInfo,
		.references = 1
	};

	auto& storedResult = m_context->store->attachments.emplace(name, std::move(attachment));

	return &storedResult.second;
}

void RenderGraph::resolve_color_attachments(std::span<RenderPassAttachmentInfo> colorAttachmentInfos, Pass& pass, std::string* debugOutput)
{
	RenderPassAttachment* pAttachment = nullptr;

	if (!pass.rasterPipeline->colorAttachments.capacity())
	{
		pass.rasterPipeline->colorAttachments.reserve(colorAttachmentInfos.size());
	}

	for (RenderPassAttachmentInfo const& attachment : colorAttachmentInfos)
	{
		if (attachment.info.has_value())
		{
			rhi::ImageFormat format = attachment.info.value().imageInfo.format;
			if (!rhi::is_color_format(format))
			{
				log(
					debugOutput,
					"Invalid format specified in RenderPassAttachmentInfo.info.imageInfo.format \"{}\". \"{}\" has the format \"{}\". Pass expects a color format for color attachments. Skipping.\n",
					attachment.name,
					pAttachment->name,
					rhi::get_image_format_string(format)
				);
				continue;
			}
			pAttachment = register_attachment_internal(attachment, debugOutput);
		}
		else
		{
			if (!attachment.dependsOn.empty())
			{
				log(
					debugOutput,
					"Empty pass name supplied for RenderPassAttachmentInfo.dependsOn for attachment \"{}\". Inherited attachments must be inherited from a pass. Skipping.\n",
					attachment.name
				);
				continue;
			}

			if (!m_context->indices.contains(attachment.dependsOn))
			{
				log(
					debugOutput,
					"Non-existent pass name \"{}\" supplied. The pass \"{}\" does not exist in \"{}\" graph. Skipping.\n",
					attachment.dependsOn,
					attachment.dependsOn,
					m_context->name
				);
				continue;
			}

			pAttachment = get_attachment(attachment.name);

			if (!pAttachment)
			{
				log(
					debugOutput,
					"Non-existent attachment name \"{}\" supplied. The attachment \"{}\" does not exist in \"{}\" graph.\n",
					attachment.name,
					attachment.name,
					m_context->name
				);
				continue;
			}

			rhi::ImageFormat format = pAttachment->image->info.format;
			if (!rhi::is_color_format(format))
			{
				log(
					debugOutput,
					"Invalid format detected for inherited render pass attachment \"{}\" in RenderPassAttachmentInfo.name. \"{}\" has the format \"{}\". Pass expects a color format for color attachments. Skipping.\n",
					pAttachment->name,
					pAttachment->name,
					rhi::get_image_format_string(format)
				);
				continue;
			}
			++pAttachment->references;
		}
		auto& [_, colorAttachment] = pass.colorAttachments.emplace(pAttachment->name, Pass::Attachment{ .attachment = pAttachment });
		// if the reference count of the current attachment is > 1, that means it's an inherited attachment.
		if (pAttachment->references > 1 && !attachment.dependsOn.empty())
		{
			std::optional dependentPass = m_context->indices.at(attachment.dependsOn);
			if (dependentPass.has_value())
			{
				colorAttachment.dependency = dependentPass.value()->first;
			}
		}
	}
}

void RenderGraph::resolve_depth_attachment(RenderPassAttachmentInfo const& depthAttachmentInfo, Pass& pass, std::string* debugOutput)
{
	RenderPassAttachment* pDepthAttachment = nullptr;

	if (depthAttachmentInfo.info.has_value())
	{
		AttachmentInfo const& createInfo = depthAttachmentInfo.info.value();
		rhi::ImageFormat format = createInfo.imageInfo.format;
		if (!rhi::is_depth_format(format))
		{
			log(
				debugOutput,
				"Invalid format supplied in RenderPassAttachmentInfo.info.imageInfo.format \"{}\". \"{}\" has the format \"{}\". Pass expects a depth | depth stencil format for depth attachments. Skipping.\n",
				depthAttachmentInfo.name,
				depthAttachmentInfo.name,
				rhi::get_image_format_string(format)
			);
			return;
		}
		pDepthAttachment = register_attachment_internal(depthAttachmentInfo, debugOutput);
	}
	else
	{
		if (!depthAttachmentInfo.dependsOn.empty())
		{
			log(
				debugOutput,
				"Empty pass name supplied for RenderPassAttachmentInfo.dependsOn for attachment \"{}\". Inherited attachments must be inherited from a pass. Skipping.\n",
				depthAttachmentInfo.name
			);
			return;
		}

		if (!m_context->indices.contains(depthAttachmentInfo.dependsOn))
		{
			log(
				debugOutput,
				"Non-existent pass name \"{}\" supplied. The pass \"{}\" does not exist in \"{}\" graph. Skipping.\n",
				depthAttachmentInfo.dependsOn,
				depthAttachmentInfo.dependsOn,
				m_context->name
			);
			return;
		}

		pDepthAttachment = get_attachment(depthAttachmentInfo.name);

		if (!pDepthAttachment)
		{
			log(
				debugOutput,
				"Non-existent attachment name \"{}\" supplied. The attachment \"{}\" does not exist in the graph \"{}\".\n",
				depthAttachmentInfo.name,
				depthAttachmentInfo.name,
				m_context->name
			);
			return;
		}
		
		rhi::ImageFormat format = pDepthAttachment->image->info.format;
		if (!rhi::is_depth_format(format))
		{
			log(
				debugOutput,
				"Invalid format detected for inherited render pass attachment \"{}\" in RenderPassAttachmentInfo.inheritInfo.name. \"{}\" has the format \"{}\". Pass expects a depth | depth stencil format for depth attachments. Skipping.\n",
				pDepthAttachment->name,
				pDepthAttachment->name,
				rhi::get_image_format_string(format)
			);
			return;
		}
		++pDepthAttachment->references;
	}
	pass.depthAttachment = Pass::Attachment{ .attachment = pDepthAttachment };
	// if the reference count of the current attachment is > 1, that means it's an inherited attachment.
	if (pDepthAttachment->references > 1 && !depthAttachmentInfo.dependsOn.empty())
	{
		std::optional dependentPass = m_context->indices.at(depthAttachmentInfo.dependsOn);
		if (dependentPass.has_value())
		{
			pass.depthAttachment->dependency = dependentPass.value()->first;
		}
	}
}

void RenderGraph::resolve_stencil_attachment(RenderPassAttachmentInfo const& stencilAttachmentInfo, Pass& pass, std::string* debugOutput)
{
	RenderPassAttachment* pStencilAttachment = nullptr;

	if (stencilAttachmentInfo.info.has_value())
	{
		AttachmentInfo const& createInfo = stencilAttachmentInfo.info.value();
		rhi::ImageFormat format = createInfo.imageInfo.format;
		if (!rhi::is_stencil_format(format))
		{
			log(
				debugOutput,
				"Invalid format supplied in RenderPassAttachmentInfo.createInfo.imageInfo.format \"{}\". \"{}\" has the format \"{}\". Pass expects a stencil format for stencil attachments. Skipping.\n",
				stencilAttachmentInfo.name,
				stencilAttachmentInfo.name,
				rhi::get_image_format_string(format)
			);
			return;
		}
		pStencilAttachment = register_attachment_internal(stencilAttachmentInfo, debugOutput);
	}
	else
	{
		if (!stencilAttachmentInfo.dependsOn.empty())
		{
			log(
				debugOutput,
				"Empty pass name supplied for RenderPassAttachmentInfo.dependsOn for attachment \"{}\". Inherited attachments must be inherited from a pass. Skipping.\n",
				stencilAttachmentInfo.name
			);
			return;
		}

		if (!m_context->indices.contains(stencilAttachmentInfo.dependsOn))
		{
			log(
				debugOutput,
				"Non-existent pass name \"{}\" supplied. The pass \"{}\" does not exist in \"{}\" graph. Skipping.\n",
				stencilAttachmentInfo.dependsOn,
				stencilAttachmentInfo.dependsOn,
				m_context->name
			);
			return;
		}

		pStencilAttachment = get_attachment(stencilAttachmentInfo.name);

		if (!pStencilAttachment)
		{
			log(
				debugOutput,
				"Non-existent attachment name \"{}\" supplied. The attachment \"{}\" does not exist in the graph \"{}\".\n",
				stencilAttachmentInfo.name,
				stencilAttachmentInfo.name,
				m_context->name
			);
			return;
		}

		rhi::ImageFormat format = pStencilAttachment->image->info.format;
		if (!rhi::is_stencil_format(format))
		{
			log(
				debugOutput,
				"Invalid format detected for inherited render pass attachment \"{}\" in RenderPassAttachmentInfo.inheritInfo.name. \"{}\" has the format \"{}\". Pass expects a stencil format for stencil attachments. Skipping.\n",
				pStencilAttachment->name,
				pStencilAttachment->name,
				rhi::get_image_format_string(format)
			);
			return;
		}
		++pStencilAttachment->references;
	}
	pass.stencilAttachment = Pass::Attachment{ .attachment = pStencilAttachment };
	// if the reference count of the current attachment is > 1, that means it's an inherited attachment.
	if (pStencilAttachment->references > 1 && !stencilAttachmentInfo.dependsOn.empty())
	{
		std::optional dependentPass = m_context->indices.at(stencilAttachmentInfo.dependsOn);
		if (dependentPass.has_value())
		{
			pass.stencilAttachment->dependency = dependentPass.value()->first;
		}
	}
}

void RenderGraph::resolve_inputs(std::span<std::string_view> inputs, Pass& pass, std::string* debugOutput)
{
	// Passes that register a resource with the same name as an attachment will not be registered as an input.
	// Writes to resource takes priority over reads of a resource.
	for (std::string_view const input : inputs)
	{
		// First check if an attachment with the specified name exists.
		RenderPassAttachment* attachment = get_attachment(input);
		if (!attachment)
		{
			log(debugOutput, "Failed to find attachment \"{}\" from any Pass. Skipping.\n", input);
			continue;
		}

		if (pass.colorAttachments.contains(input))
		{
			log(debugOutput, "Pass will not register \"{}\" as input. \"{}\" is currently registered as a color attachment. Skipping to prevent cyclic dependency.\n", input, input);
			continue;
		}

		if (pass.depthAttachment.has_value() &&
			pass.depthAttachment.value().attachment->name == input)
		{
			log(debugOutput, "Pass will not register \"{}\" as input. \"{}\" is currently registered as a depth attachment. Skipping to prevent cyclic dependency.\n", input, input);
			continue;
		}

		if (pass.stencilAttachment.has_value() &&
			pass.stencilAttachment.value().attachment->name == input)
		{
			log(debugOutput, "Pass will not register \"{}\" as input. \"{}\" is currently registered as a stencil attachment. Skipping to prevent cyclic dependency.\n", input, input);
			continue;
		}

		// Register the current attachment as an input to the pass.
		pass.inputs.emplace(attachment->name, attachment);
	}
}

bool RenderGraph::does_pass_name_exist(std::string_view passName, std::string* debugOutput) const
{
	if (m_context->indices.contains(passName))
	{
		log(
			debugOutput, 
			"Existing name \"{}\" supplied in [Raster|Compute]PassInfo.name. Pass names must be unique.\n", 
			passName
		);
		return true;
	}
	return false;
}

RasterPipeline* RenderGraph::store_raster_pipeline(std::span<Handle<Shader>> hshaders, rhi::RasterPipelineInfo const& pipelineInfo, std::string* debugOutput)
{
	std::vector<ftl::Ref<Shader>> shaders;
	for (Handle<Shader> hnd : hshaders)
	{
		auto shdr = m_context->store->resourceStore.get_shader(hnd);
		if (!shdr.valid)
		{
			if (debugOutput)
			{
				(*debugOutput).append(fmt::format("Invalid shader handle supplied in RasterPassInfo.shaders. Invalid handle value is {}.\n", hnd.get()));
			}
			return nullptr;
		}
		shaders.push_back(shdr.resource);
	}

	// Check for the existence of the vertex and pixel shader.
	bool hasVertexShader = false, hasPixelShader = false;
	for (auto const& shader : shaders)
	{
		if (shader->info.type == rhi::ShaderType::Vertex)
		{
			hasVertexShader = true;
			continue;
		}

		if (shader->info.type == rhi::ShaderType::Pixel)
		{
			hasPixelShader = true;
			continue;
		}
	}

	if (!hasPixelShader || !hasVertexShader)
	{
		if (debugOutput)
		{
			if (!hasVertexShader)
			{
				log(debugOutput, "No vertex shader supplied to RasterPassInfo.shaders. A Pass of type \"Raster\" requires a vertex shader in it's pipeline.\n");
			}

			if (!hasPixelShader)
			{
				log(debugOutput, "No fragment shader supplied to RasterPassInfo.shaders. A Pass of type \"Raster\" requires a fragment shader in it's pipeline.\n");
			}
		}
		return nullptr;
	}

	RasterPipeline pipeline{ .info = pipelineInfo };
	for (auto shader : shaders)
	{
		pipeline.shaders.push_back(shader.get());
	}

	resource::Entry<RasterPipeline> entry = m_context->store->resourceStore.store_raster_pipeline(std::move(pipeline));

	return entry.resource.get();
}

RenderGraph::RenderGraph(Context& context) :
	m_context{ &context }
{}

bool RenderGraph::add_pass(RasterPassInfo&& info, std::string* debugOutput)
{
	if (does_pass_name_exist(info.name, debugOutput))
	{
		return false;
	}

	RasterPipeline* pRasterPipeline = nullptr;
	
	if (info.pipelineInfo.rasterPipelineInfo.has_value())
	{
		rhi::RasterPipelineInfo& pipelineInfo = info.pipelineInfo.rasterPipelineInfo.value();
		pRasterPipeline = store_raster_pipeline(info.pipelineInfo.shaders, pipelineInfo, debugOutput);

		if (!pRasterPipeline)
		{
			log(debugOutput, "Failed to create raster pipeline \"{}\".\n", info.name);
			return false;
		}
	}
	else
	{
		log(debugOutput, "No RasterPipelineInfo object referenced in RasterPassInfo.pipelineInfo.rasterPipelineInfo.\n");
		return false;
	}

	Pass pass{
		.type			= Pass::Type::Raster,
		.runsOn			= rhi::DeviceQueueType::Graphics,
		.name			= m_context->store->stringPool.append(info.name),
		.rasterPipeline = pRasterPipeline,
		.viewport		= info.viewport,
		.scissor		= info.scissor
	};

	size_t index = m_context->passes.emplace(std::move(pass));
	Pass& renderPass = m_context->passes[index];
	// Index the render pass into the map.
	m_context->indices.emplace(renderPass.name, index);

	/*size_t i = m_context->passCommands.emplace(&renderPass, command::CommandStream{});
	PassCommands& passCmd = m_context->passCommands[i];*/

	if (info.colorAttachments.size())
	{
		std::span<RenderPassAttachmentInfo> colorAttachmentInfos{ info.colorAttachments.data(), info.colorAttachments.size() };
		resolve_color_attachments(colorAttachmentInfos, renderPass, debugOutput);
		// TODO(afiq):
		// Steps below needs to be done in the compile phase.
		for (auto& [_, colorAttachment] : renderPass.colorAttachments)
		{
			rhi::AttachmentLoadOp loadOp = rhi::AttachmentLoadOp::Clear;

			if (colorAttachment.attachment->references > 1)
			{
				loadOp = rhi::AttachmentLoadOp::Load;
			}
			renderPass.metadata.colorAttachments.emplace(
				colorAttachment.attachment->image.get(),
				rhi::ImageLayout::Color_Attachment,
				loadOp,
				rhi::AttachmentStoreOp::Store,
				colorAttachment.attachment->image->info.clearValue
			);
		}
	}

	if (info.depthAttachment.has_value())
	{
		resolve_depth_attachment(info.depthAttachment.value(), renderPass, debugOutput);
		// TODO(afiq):
		// Steps below needs to be done in the compile phase.
		auto& depthAttachment = renderPass.depthAttachment.value();

		rhi::ImageFormat format = depthAttachment.attachment->image->info.format;
		rhi::ImageLayout layout = rhi::ImageLayout::Depth_Stencil_Attachment;
		rhi::AttachmentLoadOp loadOp = rhi::AttachmentLoadOp::Clear;

		if (depthAttachment.attachment->references > 1)
		{
			loadOp = rhi::AttachmentLoadOp::Load;
		}

		if (format == rhi::ImageFormat::D16_Unorm ||
			format == rhi::ImageFormat::D32_Float ||
			format == rhi::ImageFormat::X8_D24_Unorm_Pack32)
		{
			layout = rhi::ImageLayout::Depth_Attachment;
		}

		rhi::command::RenderAttachment metadataAttachment{
			depthAttachment.attachment->image.get(),
			layout, 
			loadOp, 
			rhi::AttachmentStoreOp::Store, 
			depthAttachment.attachment->image->info.clearValue
		};
		renderPass.metadata.depthAttachment = metadataAttachment;
	}

	if (info.stencilAttachment.has_value())
	{
		resolve_stencil_attachment(info.stencilAttachment.value(), renderPass, debugOutput);
		// TODO(afiq):
		// Steps below needs to be done in the compile phase.
		auto& stencilAttachment = renderPass.stencilAttachment.value();

		rhi::AttachmentLoadOp loadOp = rhi::AttachmentLoadOp::Clear;

		if (stencilAttachment.attachment->references > 1)
		{
			loadOp = rhi::AttachmentLoadOp::Load;
		}

		rhi::command::RenderAttachment metadataAttachment{
			stencilAttachment.attachment->image.get(),
			rhi::ImageLayout::Stencil_Attachment,
			loadOp,
			rhi::AttachmentStoreOp::Store,
			stencilAttachment.attachment->image->info.clearValue
		};
		renderPass.metadata.stencilAttachment = metadataAttachment;
	}

	if (info.inputs.size())
	{
		std::span<std::string_view> inputs{ info.inputs.data(), info.inputs.size() };
		resolve_inputs(inputs, renderPass, debugOutput);
	}

	// This part just registers work into the pass's command stream. It does not actually execute anything.
	std::optional<task::TaskQueue::Node&> nodeOptional = m_context->store->queue.get_task_node(m_context->name);
	task::TaskQueue::Node& node = nodeOptional.value();

	task::TaskInterface taskInterface{ renderPass, node };
	info.execute(taskInterface);

	m_context->dirty = true;

	return true;
}

bool RenderGraph::create_attachment(std::string_view name, AttachmentInfo const& info, std::string* debugOutput)
{
	return register_attachment_internal(RenderPassAttachmentInfo{ .name = name, .info = info }, debugOutput) != nullptr;
}

bool RenderGraph::compile()
{
	// TODO(afiq):
	// Figure out a more elegant solution.

	if (!m_context->dirty && m_context->compiled)
	{
		return false;
	}

	m_context->compiled = false;

	// A digraph that stores the dependecies between the pass bucket indices.
	ftl::Digraph<size_t> graph{ m_context->passes.length() };

	// Resolve attachments.
	// "to" represents the index of the dependee pass inside of m_context->passes.
	// "from" represents the index of the dependent pass inside of m_context->passes.
	for (auto& [key, to] : m_context->indices)
	{
		Pass& pass = m_context->passes[to];
		// Resolve color attachments of each pass.
		for (auto& [__, colorAttachment] : pass.colorAttachments)
		{
			if (colorAttachment.dependency.empty())
			{
				graph.add_edge(size_t{ std::numeric_limits<size_t>::max() }, size_t{ to });
				continue;
			}
			size_t from = m_context->indices[colorAttachment.dependency];
			graph.add_edge(size_t{ from }, size_t{ to });
		}

		if (pass.depthAttachment.has_value() &&
			!pass.depthAttachment.value().dependency.empty())
		{
			size_t from = m_context->indices.bucket(pass.depthAttachment.value().dependency);
			graph.add_edge(size_t{ from }, size_t{ to });
		}
		else
		{
			graph.add_edge(size_t{ std::numeric_limits<size_t>::max() }, size_t{ to });
		}

		if (pass.stencilAttachment.has_value() &&
			!pass.stencilAttachment.value().dependency.empty())
		{
			size_t from = m_context->indices.bucket(pass.stencilAttachment.value().dependency);
			graph.add_edge(size_t{ from }, size_t{ to });
		}
		else
		{
			graph.add_edge(size_t{ std::numeric_limits<size_t>::max() }, size_t{ to });
		}
	}

	// Resolve inputs.
	// "to" represents the index of the dependee pass inside of m_context->passes.
	// "from" represents the index of the dependent pass inside of m_context->passes.
	for (auto& [i, to] : m_context->indices)
	{
		Pass& currentPass = m_context->passes[to];
		// If a pass does not contain an input, we assign an imaginary edge "std::numeric_limits<size_t>::max()" that acts as the root node in our DAG.
		if (currentPass.inputs.empty())
		{
			// TODO(afiq):
			// Need to see if this method does indeed work.
			graph.add_edge(size_t{ std::numeric_limits<size_t>::max() }, size_t{ to });
			continue;
		}

		// Iterate through each pass's input.
		for (auto& [_, input] : currentPass.inputs)
		{
			bool found = false;
			ftl::Ref<gpu::Image> currentInputImage = input.attachment->image;

			for (auto& [j, from] : m_context->indices)
			{
				Pass& otherPass = m_context->passes[from];

				if (&currentPass == &otherPass)
				{
					continue;
				}
				
				if (rhi::is_depth_format(currentInputImage->info.format))
				{
					found = (otherPass.depthAttachment.has_value() && 
							 input.attachment == otherPass.depthAttachment.value().attachment);
				}
				else if (rhi::is_stencil_format(currentInputImage->info.format))
				{
					found = (otherPass.stencilAttachment.has_value() && 
							 input.attachment == otherPass.stencilAttachment.value().attachment);
				}
				else
				{
					for (auto& [__, colorAttachment] : otherPass.colorAttachments)
					{
						if (input.attachment == colorAttachment.attachment)
						{
							found = true;
							break;
						}
					}
				}

				if (found)
				{
					graph.add_edge(size_t{ from }, size_t{ to });
					break;
				}
			}
		}
	}

	// An array that represents the sorted order of indices that the passes needs to run in.
	ftl::Array<size_t> sorted = graph.sort();

	task::TaskQueue::Node& node = m_context->store->queue.get_task_node(m_context->name).value();

	for (size_t i = 0; i < sorted.size(); ++i)
	{
		size_t sortedIndex = sorted[i];

		Pass& src = m_context->passes[sortedIndex];
		Pass& dst = m_context->passes[i];

		std::swap(dst, src);
		// Swap the span elements inside of the task queue as well.
		task::TaskQueue::Node::index_span& srcSpan = node.taskGroups[sortedIndex];
		task::TaskQueue::Node::index_span& dstSpan = node.taskGroups[i];

		std::swap(dstSpan, srcSpan);


		// Re-index the entries.
		m_context->indices[dst.name] = sortedIndex;
	}

	m_context->compiled = true;



	return true;
}

void RenderGraph::run()
{
	m_context->execute = true;
}

void RenderGraph::stop()
{
	m_context->execute = false;
}

bool RenderGraph::is_compiled() const
{
	return m_context->compiled;
}

bool RenderGraph::is_dirty() const
{
	return m_context->dirty;
}

std::string_view RenderGraph::get_name()
{
	return m_context->name;
}

Context* RenderGraphStore::add_render_graph(std::string_view name)
{
	if (ids.contains(name))
	{
		return nullptr;
	}

	ftl::HashStringView identifier = stringPool.append(name);

	Context context{
		.name = identifier,
		.store = this
	};

	size_t index = contexts.push(std::move(context));
	ids.emplace(identifier, index);

	queue.add_task_node(identifier);

	return &contexts[index];
}

}

}