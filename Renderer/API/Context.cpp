#include "Context.h"

//struct RenderContextGlobalVariables
//{
//	Array<uint32> ShaderBinaries;
//	using ConstShaderTypes = const shaderc_shader_kind;
//	ConstShaderTypes ShaderTypes[4] = { shaderc_vertex_shader, shaderc_fragment_shader, shaderc_compute_shader, shaderc_geometry_shader };
//} GlobalVars;

//RenderContext::RenderContext() : GraphicsDriver() {}
//
//RenderContext::~RenderContext() {}
//
//bool RenderContext::InitializeContext()
//{
//	if (!InitializeDriver()) { return false; }
//	return true;
//}
//
//void RenderContext::TerminateContext()
//{
//	TerminateDriver();
//}
//
//void RenderContext::BlitToDefault(const IRFrameGraph& Graph)
//{
//	vk::HwBlitInfo blitInfo = {};
//
//	blitInfo.FramePassHandle	= DefaultFramebuffer;
//	blitInfo.Source				= Graph.GetColorImage();
//	blitInfo.Destination		= INVALID_HANDLE;
//
//	BlitImageToSwapchain(blitInfo);
//	PushCmdBufferForSubmit(DefaultFramebuffer);
//}
//
//void RenderContext::BindRenderPass(RenderPass& Pass)
//{
//	vk::HwCmdBufferRecordInfo recordInfo = {};
//	
//	recordInfo.FramePassHandle	= Pass.FramePassHandle;
//	recordInfo.PipelineHandle	= Pass.PipelineHandle;
//	recordInfo.NumOutputs		= static_cast<uint32>(Pass.ColorOutputs.Length());
//	recordInfo.HasDepthStencil	= (Pass.DepthStencilOutput.Handle != INVALID_HANDLE);
//
//	recordInfo.ClearColor[Color_Channel_Red]	= 0.0f;
//	recordInfo.ClearColor[Color_Channel_Green]	= 0.0f;
//	recordInfo.ClearColor[Color_Channel_Blue]	= 0.0f;
//	recordInfo.ClearColor[Color_Channel_Alpha]	= 1.0f;
//
//	if (!Pass.Flags.Has(RenderPass_Bit_No_Color_Render))
//	{
//		recordInfo.NumOutputs++;
//	}
//
//	if (!Pass.Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
//	{
//		recordInfo.NumOutputs++;
//		recordInfo.HasDepthStencil = true;
//	}
//
//	RecordCommandBuffer(recordInfo);
//}
//
//void RenderContext::UnbindRenderPass(RenderPass& Pass)
//{
//	vk::HwCmdBufferRecordInfo recordInfo = {};
//	recordInfo.FramePassHandle = Pass.FramePassHandle;
//	recordInfo.PipelineHandle = Pass.PipelineHandle;
//
//	vk::HwCmdBufferUnrecordInfo unrecordInfo = {};
//	unrecordInfo.FramePassHandle = Pass.FramePassHandle;
//	unrecordInfo.Unrecord = true;
//	unrecordInfo.UnbindRenderpass = true;
//
//	UnrecordCommandBuffer(unrecordInfo);
//	PushCmdBufferForSubmit(Pass.FramePassHandle);
//}
//
//void RenderContext::SubmitForRender(const DrawCommand& Command, RenderPass& Pass)
//{
//	vk::HwDrawInfo drawInfo = {};
//
//	drawInfo.FramePassHandle	= Pass.FramePassHandle;
//	drawInfo.VertexOffset		= Command.VertexOffset;
//	drawInfo.IndexOffset		= Command.IndexOffset;
//	drawInfo.VertexCount		= Command.NumVertices;
//	drawInfo.IndexCount			= Command.NumIndices;
//
//	Draw(drawInfo);
//}
//
//bool RenderContext::NewFrameImages(FrameImages& Images)
//{
//	vk::HwImageCreateStruct image = {};
//
//	image.Handle		= &Images.Color;
//	image.Width			= SwapChain.Extent.width;
//	image.Height		= SwapChain.Extent.height;
//	image.MipLevels		= 1;
//	image.Depth			= 1;
//	image.Samples		= Sample_Count_1;
//	image.Type			= Texture_Type_2D;
//	image.Usage			= Texture_Usage_Color;
//	image.UsageFlagBits = Image_Usage_Color_Attachment | Image_Usage_Transfer_Src | Image_Usage_Sampled;
//
//	if (!CreateImage(image)) { return false; }
//
//	image.Handle		= &Images.DepthStencil;
//	image.Usage			= Texture_Usage_Depth_Stencil;
//	image.UsageFlagBits = Image_Usage_Depth_Stencil_Attachment | Image_Usage_Sampled;
//
//	if (!CreateImage(image)) { return false; }
//
//	return true;
//}
//
//void RenderContext::DestroyFrameImages(FrameImages& Images)
//{
//	DestroyImage(Images.Color);
//	DestroyImage(Images.DepthStencil);
//}
//
//bool RenderContext::NewGraphicsPipeline(RenderPass& Pass)
//{
//	//if (!ValidateGraphicsPipelineCreateInfo(CreateInfo))
//	//{
//	//	return false;
//	//}
//
//	vk::HwPipelineCreateInfo pipelineCreateInfo;
//
//	// Compile shaders if they're not compiled.
//	vk::ShaderToSPIRVCompiler shaderCompiler;
//
//	for (Shader* shader : Pass.Shaders)
//	{
//		if (shader->Handle != -1)
//		{
//			continue;
//		}
//
//		GlobalVars.ShaderBinaries.Empty();;
//		if (!shaderCompiler.CompileShader(
//			shader->Name.C_Str(),
//			GlobalVars.ShaderTypes[shader->Type],
//			shader->Code.C_Str(),
//			GlobalVars.ShaderBinaries
//		))
//		{
//			// TODO(Ygsm):
//			// Include logging system.
//			// printf("%s\n\n", shaderCompiler.GetLastErrorMessage());
//			return false;
//		}
//
//		vk::HwShaderCreateInfo createInfo = {};
//
//		createInfo.DWordBuf = &GlobalVars.ShaderBinaries;
//		createInfo.Handle = &shader->Handle;
//
//		if (!CreateShader(createInfo))
//		{
//			return false;
//		}
//
//		vk::HwPipelineCreateInfo::ShaderInfo shaderInfo = {};
//
//		shaderInfo.Handle			= shader->Handle;
//		shaderInfo.Type				= shader->Type;
//		shaderInfo.Attributes		= shader->Attributes.First();
//		shaderInfo.AttributeCount	= shader->Attributes.Length();
//
//		pipelineCreateInfo.Shaders.Push(shaderInfo);
//	}
//
//	//
//	// TODO(Ygsm):
//	// Configure depth stencil out graphics pipeline properly.
//	//
//	Handle<HFramePass>* passHandle = &Pass.FramePassHandle;
//
//	if (Pass.IsSubpass())
//	{
//		passHandle = &Pass.Parent->FramePassHandle;
//	}
//
//	pipelineCreateInfo.VertexStride		= Pass.VertexStride;
//	pipelineCreateInfo.FramePassHandle	= *passHandle;
//	pipelineCreateInfo.Handle			= &Pass.PipelineHandle;
//	pipelineCreateInfo.CullMode			= Pass.CullMode;
//	pipelineCreateInfo.FrontFace		= Pass.FrontFace;
//	pipelineCreateInfo.PolyMode			= Pass.PolygonalMode;
//	pipelineCreateInfo.Topology			= Pass.Topology;
//	pipelineCreateInfo.HasDepth			= Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output) || !Pass.Flags.Has(RenderPass_Bit_No_DepthStencil_Render);
//	pipelineCreateInfo.HasStencil		= false;
//
//	for (DescriptorLayout* layout : Pass.BoundDescriptorLayouts)
//	{
//		pipelineCreateInfo.DescriptorLayouts.Push(layout->Handle);
//	}
//
//	pipelineCreateInfo.NumColorAttachments = static_cast<uint32>(Pass.ColorOutputs.Length());
//
//	if (!Pass.Flags.Has(RenderPass_Bit_No_Color_Render))
//	{
//		pipelineCreateInfo.NumColorAttachments++;
//	}
//
//	if (!CreatePipeline(pipelineCreateInfo))
//	{
//		// TODO(Ygsm):
//		// Include a logging system.
//		return false;
//	}
//
//	for (Shader* shader : Pass.Shaders)
//	{
//		DestroyShader(shader->Handle);
//	}
//
//	Pass.State = RenderPass_State_Built;
//
//	return true;
//}
//
//void RenderContext::DestroyGraphicsPipeline(RenderPass& Pass, bool Release)
//{
//	DestroyPipeline(Pass.PipelineHandle);
//	if (Release)
//	{
//		ReleasePipeline(Pass.PipelineHandle);
//	}
//}
//
//bool RenderContext::NewRenderPassFramebuffer(RenderPass& Pass)
//{
//	vk::HwFramebufferCreateInfo framebufferCreateInfo = {};
//
//	framebufferCreateInfo.Width		= Pass.Width;
//	framebufferCreateInfo.Height	= Pass.Height;
//	framebufferCreateInfo.Depth		= Pass.Depth;
//	framebufferCreateInfo.Samples	= Pass.Samples;
//	framebufferCreateInfo.Handle	= Pass.FramePassHandle;
//
//	framebufferCreateInfo.DefaultOutputs[vk::Default_Output_Color]			= Pass.Owner.GetColorImage();
//	framebufferCreateInfo.DefaultOutputs[vk::Default_Output_DepthStencil]	= Pass.Owner.GetDepthStencilImage();
//
//	if (Pass.Flags.Has(RenderPass_Bit_No_Color_Render))
//	{
//		framebufferCreateInfo.DefaultOutputs[vk::Default_Output_Color] = INVALID_HANDLE;
//	}
//
//	if (Pass.Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
//	{
//		framebufferCreateInfo.DefaultOutputs[vk::Default_Output_DepthStencil] = INVALID_HANDLE;
//	}
//
//	uint32 i = 0;
//
//	for (auto& pair : Pass.ColorOutputs)
//	{
//		auto& passAttachment = pair.Value;
//		vk::HwAttachmentInfo& colorAttachment = framebufferCreateInfo.Outputs[i++];
//		FMemory::InitializeObject(colorAttachment);
//
//		colorAttachment.Handle		= &passAttachment.Handle;
//		colorAttachment.Type		= passAttachment.Type;
//		colorAttachment.Usage		= passAttachment.Usage;
//
//		colorAttachment.UsageFlagBits = Image_Usage_Color_Attachment | Image_Usage_Sampled;
//	}
//
//	framebufferCreateInfo.NumColorOutputs = i;
//
//
//	if (Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output))
//	{
//		vk::HwAttachmentInfo& depthStencil = framebufferCreateInfo.Outputs[i++];
//
//		depthStencil.Handle	= &Pass.DepthStencilOutput.Handle;
//		depthStencil.Type = Pass.DepthStencilOutput.Type;
//		depthStencil.Usage = Pass.DepthStencilOutput.Usage;
//		depthStencil.UsageFlagBits	= Image_Usage_Depth_Stencil_Attachment | Image_Usage_Sampled;
//	}
//
//	framebufferCreateInfo.NumOutputs = i;
//
//	return CreateFramebuffer(framebufferCreateInfo);
//}
//
//void RenderContext::DestroyRenderPassFramebuffer(RenderPass& Pass, bool Release)
//{
//	for (auto& pair : Pass.ColorOutputs)
//	{
//		DestroyImage(pair.Value.Handle);
//	}
//
//	if (Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output))
//	{
//		DestroyImage(Pass.DepthStencilOutput.Handle);
//	}
//
//	DestroyFramebuffer(Pass.FramePassHandle);
//
//	if (Release)
//	{
//		ReleaseFramePass(Pass.FramePassHandle);
//	}
//}
//
//bool RenderContext::NewRenderPassRenderpass(RenderPass& Pass)
//{
//	const uint32 finalOrder = Pass.Owner.GetNumRenderPasses() - 1;
//
//	vk::HwRenderpassCreateInfo createInfo = {};
//
//	createInfo.Handle = &Pass.FramePassHandle;
//	createInfo.Samples = Pass.Samples;
//	createInfo.NumColorOutputs = static_cast<uint32>(Pass.ColorOutputs.Length());
//	createInfo.DefaultOutputs  = Pass.Flags;
//	createInfo.HasDepthStencilAttachment = Pass.Flags.Has(RenderPass_Bit_DepthStencil_Output);
//
//	if (!Pass.Order)
//	{
//		createInfo.Order.Set(RenderPass_Order_First);
//	}
//
//	if (Pass.Order == finalOrder)
//	{
//		createInfo.Order.Set(RenderPass_Order_Last);
//	}
//	
//	if (Pass.Order && Pass.Order != finalOrder)
//	{
//		createInfo.Order.Set(RenderPass_Order_InBetween);
//	}
//	
//	return CreateRenderPass(createInfo);
//}
//
//void RenderContext::DestroyRenderPassRenderpass(RenderPass& Pass)
//{
//	DestroyRenderPass(Pass.FramePassHandle);
//}
//
//bool RenderContext::NewDescriptorPool(DescriptorPool& Pool)
//{
//	vk::HwDescriptorPoolCreateInfo createInfo = {};
//	createInfo.Handle = &Pool.Handle;
//	createInfo.Type = Pool.DescriptorTypes.Value();
//	createInfo.NumDescriptorsOfType = Pool.Capacity;
//
//	return CreateDescriptorPool(createInfo);
//}
//
//bool RenderContext::NewDestriptorSetLayout(DescriptorLayout& Layout)
//{
//	vk::HwDescriptorSetLayoutCreateInfo createInfo = {};
//	createInfo.Binding = Layout.Binding;
//	createInfo.Handle = &Layout.Handle;
//	createInfo.ShaderStages = Layout.ShaderStages;
//	createInfo.Type = Layout.Type;
//
//	return CreateDescriptorSetLayout(createInfo);
//}
//
//bool RenderContext::NewDescriptorSet(DescriptorSet& Set)
//{
//	vk::HwDescriptorSetAllocateInfo allocInfo = {};
//	allocInfo.Handle = &Set.Handle;
//	allocInfo.Layout = Set.Layout->Handle;
//	allocInfo.Pool	 = Set.Pool->Handle;
//
//	return AllocateDescriptorSet(allocInfo);
//}
//
//bool RenderContext::CreateCmdPoolAndBuffers()
//{
//	if (!CreateCommandPool())		{ return false; }
//	//if (!AllocateCommandBuffers())	{ return false; }
//	return true;
//}
//
//bool RenderContext::NewImage(Texture& InTexture)
//{
//	vk::HwImageCreateStruct createInfo = {};
//
//	createInfo.Depth = 1;
//	createInfo.Width = InTexture.Width;
//	createInfo.Height = InTexture.Height;
//	createInfo.MipLevels = 1;
//	createInfo.Samples = Sample_Count_1;
//	createInfo.Type = Texture_Type_2D;
//	createInfo.Usage = Texture_Usage_Color;
//	createInfo.UsageFlagBits = Image_Usage_Sampled | Image_Usage_Transfer_Dst;
//	createInfo.Handle = &InTexture.Handle;
//
//	CreateImage(createInfo);
//
//	return true;
//}
//
//void RenderContext::TransferImageToGPU(Texture& InTexture, SRMemoryBuffer& Buffer)
//{
//	BeginTransferCommand();
//
//	vk::HwImageBarrierInfo barrierInfo = {};
//	barrierInfo.Handle = InTexture.Handle;
//	barrierInfo.OldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	barrierInfo.NewLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//	barrierInfo.SrcAccessMask = 0;
//	barrierInfo.DstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//	barrierInfo.SrcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//	barrierInfo.DstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
//
//	ImageBarrier(barrierInfo);
//
//	vk::HwImageTransferInfo txInfo = {};
//	txInfo.ImageHandle = InTexture.Handle;
//	txInfo.BufferHandle = Buffer.Handle;
//	txInfo.Width = InTexture.Width;
//	txInfo.Height = InTexture.Height;
//
//	TransferImage(txInfo);
//
//	barrierInfo.OldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//	barrierInfo.NewLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//	barrierInfo.SrcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//	barrierInfo.DstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//	barrierInfo.SrcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
//	barrierInfo.DstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//
//	ImageBarrier(barrierInfo);
//
//	EndTransferCommand();
//}
//
//bool RenderContext::NewBuffer(SRMemoryBuffer& Buffer, void* Data, size_t Size, size_t Count)
//{
//	if (Buffer.Handle != INVALID_HANDLE) { return true; }
//
//	vk::HwBufferCreateInfo createInfo = {};
//
//	createInfo.Handle = &Buffer.Handle;
//	createInfo.Data = Data;
//	createInfo.Size = Size;
//	createInfo.Locality = Buffer.Locality;
//	createInfo.Type = Buffer.Type;
//	createInfo.Count = Count;
//
//	return CreateBuffer(createInfo);
//}
//
//bool RenderContext::CopyDataToBuffer(SRMemoryBuffer& Buffer, void* Data, size_t Size)
//{
//	if (Buffer.Handle == INVALID_HANDLE) { return false; }
//
//	size_t paddedSize = PadSizeToAlignedSize(Size);
//
//	vk::HwMapDataToBufferInfo mapInfo = {};
//	mapInfo.BufferHandle = Buffer.Handle;
//	mapInfo.Offset = Buffer.Offset;
//	mapInfo.Data = Data;
//	mapInfo.Size = Size;
//
//	MapDataToBuffer(mapInfo);
//
//	Buffer.Offset += paddedSize;
//
//	return true;
//}
//
//void RenderContext::TransferToGPU(SRMemoryTransferContext* TransferContext, size_t Count)
//{
//	SRMemoryTransferContext* transferCtx = nullptr;
//	vk::HwMemoryTransferInfo transferInfo = {};
//
//	BeginTransferCommand();
//	for (size_t i = 0; i < Count; i++)
//	{
//		transferCtx = &TransferContext[i];
//		transferInfo.SrcHandle = transferCtx->SrcBuffer;
//		transferInfo.SrcOffset = transferCtx->SrcOffset;
//		transferInfo.SrcSize = transferCtx->SrcSize;
//		transferInfo.DstHandle = transferCtx->DstBuffer;
//		transferInfo.DstOffset = transferCtx->DstOffset;
//
//		TransferMemory(transferInfo);
//		transferInfo = {};
//	}
//	EndTransferCommand();
//}
//
////bool RenderContext::NewVertexBuffer(Handle<HBuffer>& Vbo, void* Data, size_t Count)
////{
////	//if (Vbo != 0 && Vbo != INVALID_HANDLE) { return true; }
////
////	vk::HwBufferCreateInfo vboCreateInfo;
////
////	vboCreateInfo.Handle = &Vbo;
////	vboCreateInfo.Data	= Data;
////	vboCreateInfo.Count = Count;
////	vboCreateInfo.Size = sizeof(Vertex);
////	vboCreateInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
////	vboCreateInfo.Type.Set(Buffer_Type_Vertex);
////
////	return CreateBuffer(vboCreateInfo);
////}
//
////bool RenderContext::NewIndexBuffer(Handle<HBuffer>& Ebo, void* Data, size_t Count)
////{
////	//if (Ebo != 0 && Ebo != INVALID_HANDLE) { return true; }
////
////	vk::HwBufferCreateInfo eboCreateInfo;
////
////	eboCreateInfo.Handle = &Ebo;
////	eboCreateInfo.Data	= Data;
////	eboCreateInfo.Count = Count;
////	eboCreateInfo.Size = sizeof(uint32);
////	eboCreateInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
////	eboCreateInfo.Type.Set(Buffer_Type_Index);
////
////	return CreateBuffer(eboCreateInfo);
////}
//
////bool RenderContext::NewUniformBuffer(Handle<HBuffer>& Ubo, void* Data, size_t Size)
////{
////	vk::HwBufferCreateInfo uboCreateInfo = {};
////	uboCreateInfo.Handle = &Ubo;
////	uboCreateInfo.Data = Data;
////	uboCreateInfo.Count = 1;
////	uboCreateInfo.Size = Size;
////	uboCreateInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
////	uboCreateInfo.Type.Set(Buffer_Type_Uniform);
////
////	return CreateBuffer(uboCreateInfo);
////}
//
////bool RenderContext::NewCPUBuffer(Handle<HBuffer>& Hnd, size_t Size)
////{
////	vk::HwBufferCreateInfo bufCreateInfo = {};
////	bufCreateInfo.Handle = &Hnd;
////	bufCreateInfo.Count = 1;
////	bufCreateInfo.Size = Size;
////	bufCreateInfo.Locality = Buffer_Locality_Cpu;
////	bufCreateInfo.Type.Set(Buffer_Type_Transfer_Src);
////
////	return CreateBuffer(bufCreateInfo);
////}
//
////bool RenderContext::NewGPUBuffer(Handle<HBuffer>& Hnd, size_t Size)
////{
////	vk::HwBufferCreateInfo bufCreateInfo = {};
////	bufCreateInfo.Handle = &Hnd;
////	bufCreateInfo.Count = 1;
////	bufCreateInfo.Size = Size;
////	bufCreateInfo.Locality = Buffer_Locality_Gpu;
////	bufCreateInfo.Type.Set(Buffer_Type_Vertex);
////	bufCreateInfo.Type.Set(Buffer_Type_Index);
////	bufCreateInfo.Type.Set(Buffer_Type_Transfer_Dst);
////
////	return CreateBuffer(bufCreateInfo);
////}
//
////size_t RenderContext::CopyDataToBuffer(Handle<HBuffer> Hnd, size_t Offset, void* Data, size_t Size)
////{
////	size_t paddedSize = PadSizeToAlignedSize(Size);
////
////	vk::HwMapDataToBufferInfo mapInfo = {};
////	mapInfo.BufferHandle = Hnd;
////	mapInfo.
////	mapInfo.Data = Data;
////}
//
//void RenderContext::BindDescriptorSetInstance(DescriptorSetInstance& Descriptor, RenderPass& Pass)
//{
//	DescriptorSet* descriptorSet = Descriptor.Owner;
//
//	const size_t frameIndex = static_cast<size_t>(GetCurrentFrameIndex());
//	size_t base = descriptorSet->Base + (frameIndex * descriptorSet->NumOfData * descriptorSet->TypeSize);
//	vk::HwDescriptorSetBindInfo bindInfo = {};
//
//	bindInfo.DescriptorType = descriptorSet->Type;
//	bindInfo.Offset = static_cast<uint32>(base) + static_cast<uint32>(Descriptor.Offset);
//	bindInfo.PipelineHandle = Pass.PipelineHandle;
//	bindInfo.FramePassHandle = Pass.FramePassHandle;
//	bindInfo.DescriptorSetHandle = descriptorSet->Handle;
//
//	BindDescriptorSets(bindInfo);
//}
//
//void RenderContext::BindDataToDescriptorSet(DescriptorSet& Set, void* Data, size_t Size)
//{
//	size_t paddedSize = PadSizeToAlignedSize(Size);
//	
//	const uint32 frameIndex = GetCurrentFrameIndex();
//	size_t originOffset = Set.Base + (frameIndex * Set.NumOfData * Set.TypeSize);
//
//	vk::HwMapDataToBufferInfo mapInfo = {};
//	mapInfo.Data = Data;
//	mapInfo.Offset = originOffset + Set.Offset;
//	mapInfo.Size = paddedSize;
//	mapInfo.BufferHandle = Set.Buffer->Handle;
//
//	MapDataToBuffer(mapInfo);
//
//	Set.Offset += static_cast<uint32>(paddedSize);
//}
//
//void RenderContext::MapDescriptorSetToBuffer(DescriptorSet& Set)
//{
//	size_t paddedRange = PadSizeToAlignedSize(Set.NumOfData * Set.TypeSize);
//
//	vk::HwDescriptorSetMapInfo mapInfo = {};
//	mapInfo.Binding = Set.Layout->Binding;
//	mapInfo.BufferHandle = Set.Buffer->Handle;
//	mapInfo.DescriptorSetHandle = Set.Handle;
//	mapInfo.Offset = Set.Base;
//	mapInfo.DescriptorType = Set.Type;
//	mapInfo.Range = paddedRange;
//
//	MapDescSetToBuffer(mapInfo);
//}
//
//uint32 RenderContext::GetCurrentFrameIndex() const
//{
//	return CurrentFrame;
//}
//
//void RenderContext::FinalizeRenderPass(RenderPass& Pass)
//{
//	PushCmdBufferForSubmit(Pass.FramePassHandle);
//}
//
//void RenderContext::BindVertexAndIndexBuffer(const DrawCommand& Command, RenderPass& Pass)
//{
//	vk::HwBindVboEboInfo bindInfo = {};
//	bindInfo.FramePassHandle = Pass.FramePassHandle;
//	bindInfo.Vbo = Command.Vbo;
//	bindInfo.Ebo = Command.Ebo;
//
//	BindVboAndEbo(bindInfo);
//}
