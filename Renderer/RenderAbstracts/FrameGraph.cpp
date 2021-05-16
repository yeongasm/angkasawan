#include "FrameGraph.h"
#include "Library/Algorithms/Hash.h"
#include "Renderer.h"

size_t IFrameGraph::_HashSeed = OS::GetPerfCounter();

SRenderPass* IFrameGraph::GetRenderPass(Handle<SRenderPass> Hnd)
{
	for (auto [hnd, pass] : RenderPasses)
	{
		if (hnd != Hnd) { continue; }
		return pass;
	}
	return nullptr;
}

size_t IFrameGraph::HashIdentifier(const String64& Identifier)
{
	size_t id = 0;
	XXHash64(Identifier.First(), Identifier.Length(), &id, _HashSeed);
	return id;
}

bool IFrameGraph::CreateFramebuffer(SRenderPass* pRenderPass)
{
	size_t totalOutputs = (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output)) ?
		pRenderPass->ColorOutputs.Length() + 1 : pRenderPass->ColorOutputs.Length();

	Array<VkImageView> imageViews(totalOutputs);

	if (!pRenderPass->Flags.Has(RenderPass_Bit_No_Color_Render))
	{
		imageViews.Push(pRenderPass->pOwner->ColorImage.Value->ImgViewHnd);
	}

	for (auto& [hnd, pImg] : pRenderPass->ColorOutputs)
	{
		imageViews.Push(pImg->ImgViewHnd);
	}

	if (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output))
	{
		imageViews.Push(pRenderPass->DepthStencilOutput.Value->ImgViewHnd);
	}
	else
	{
		if (!pRenderPass->Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
		{
			imageViews.Push(pRenderPass->pOwner->DepthStencilImage.Value->ImgViewHnd);
		}
	}

	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.width = pRenderPass->Extent.Width;
	info.height = pRenderPass->Extent.Height;
	info.layers = 1;
	info.pAttachments = imageViews.First();
	info.attachmentCount = static_cast<uint32>(imageViews.Length());
	info.renderPass = pRenderPass->RenderPassHnd;

	for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
	{
		if (vkCreateFramebuffer(Renderer.Device->GetDevice(), &info, nullptr, &pRenderPass->Framebuffer.Hnd[i]) != VK_SUCCESS)
		{
			VKT_ASSERT("Unable to create framebuffer" && false);
			return false;
		}
	}

	return true;
}

void IFrameGraph::DestroyFramebuffer(SRenderPass* pRenderPass)
{
	const VkDevice device = Renderer.Device->GetDevice();
	vkDeviceWaitIdle(device);
	for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
	{
		vkDestroyFramebuffer(device, pRenderPass->Framebuffer.Hnd[i], nullptr);
		pRenderPass->Framebuffer.Hnd[i] = VK_NULL_HANDLE;
	}
}

bool IFrameGraph::CreateRenderPass(SRenderPass* pRenderPass)
{
	uint32 offset = 0;
	uint32 colorAttCount = 0;
	const size_t numPasses = pRenderPass->pOwner->GetNumRenderPasses();

	Array<VkAttachmentDescription> descriptions;
	Array<VkAttachmentReference> references;

	if (!pRenderPass->Flags.Has(RenderPass_Bit_No_Color_Render))
	{
		VkAttachmentDescription& desc = descriptions.Insert(VkAttachmentDescription());
		desc.format = VK_FORMAT_R8G8B8A8_SRGB;
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		//switch (pRenderPass->Order)
		//{
		//case RenderPass_Order_First:
		//	desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		//	desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		//	desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//	break;
		//case RenderPass_Order_InBetween:
		//	desc.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		//	break;
		//case RenderPass_Order_Last:
		//default:
		//	desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//	break;
		//}

		VkAttachmentReference& ref = references.Insert(VkAttachmentReference());
		ref.attachment = offset++;
		ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		colorAttCount++;
	}

	for (size_t i = 0; i < pRenderPass->ColorOutputs.Length(); i++, offset++)
	{
		VkAttachmentDescription& desc = descriptions.Insert(VkAttachmentDescription());
		desc.format = VK_FORMAT_R8G8B8A8_SRGB;
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference& ref = references.Insert(VkAttachmentReference());
		ref.attachment = offset;
		ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		colorAttCount++;
	}

	if (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output) ||
		!pRenderPass->Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
	{
		VkAttachmentDescription& desc = descriptions.Insert(VkAttachmentDescription());
		desc.format = VK_FORMAT_D24_UNORM_S8_UINT;
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		//if (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output))
		//{
		//	desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//}

		VkAttachmentReference& ref = references.Insert(VkAttachmentReference());
		ref.attachment = offset++;
		ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = Renderer.Device->GetPipelineBindPoint(pRenderPass->Type);
	subpassDescription.pColorAttachments = references.First();
	subpassDescription.colorAttachmentCount = colorAttCount;

	if (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output) ||
		!pRenderPass->Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
	{
		subpassDescription.pDepthStencilAttachment = &references[offset - 1];
	}

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pAttachments = descriptions.First();
	renderPassInfo.attachmentCount = static_cast<uint32>(descriptions.Length());	
	//renderPassInfo.pDependencies = nullptr;
	//renderPassInfo.dependencyCount = 0;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.subpassCount = 1;

	if (vkCreateRenderPass(Renderer.Device->GetDevice(), &renderPassInfo, nullptr, &pRenderPass->RenderPassHnd) != VK_SUCCESS)
	{
		VKT_ASSERT("Unable to create render pass." && false);
		return false;
	}

	return true;
}

void IFrameGraph::DestroyRenderPass(SRenderPass* pRenderPass)
{
	const VkDevice device = Renderer.Device->GetDevice();
	vkDeviceWaitIdle(device);
	vkDestroyRenderPass(device, pRenderPass->RenderPassHnd, nullptr);
}

IFrameGraph::IFrameGraph(IRenderSystem& InRenderer) :
	Renderer{ InRenderer },
	ColorImage{},
	DepthStencilImage{},
	RenderPasses{},
	Extent{},
	Built{ false }
{}

IFrameGraph::~IFrameGraph() {}

void IFrameGraph::BeginRenderPass(SRenderPass* pRenderPass)
{
	using ClearValues = StaticArray<VkClearValue, MAX_RENDERPASS_ATTACHMENT_COUNT>;
	static ClearValues clearValues;
	bool hasDepthStencil = false;
	const uint32 nextSwapchainIndex = Renderer.Device->GetNextSwapchainImageIndex();

	size_t numClearValues = pRenderPass->ColorOutputs.Length();

	if (!pRenderPass->Flags.Has(RenderPass_Bit_No_Color_Render))
	{
		numClearValues++;
	}

	if (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output) &&
		pRenderPass->DepthStencilOutput.Key != INVALID_HANDLE)
	{
		numClearValues++;
		hasDepthStencil = true;
	}
	else if (!pRenderPass->Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
	{
		hasDepthStencil = true;
		numClearValues++;
	}

	for (size_t i = 0; i < numClearValues; i++)
	{
		VkClearValue& clearValue = clearValues.Insert(VkClearValue());
		clearValue.color = { 0, 0, 0, 1 };

		if (i == numClearValues - 1 && hasDepthStencil)
		{
			clearValue.depthStencil = { 1.0f, 0 };
		}
	}

	// Need to bind descriptor sets here ... 

	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = pRenderPass->RenderPassHnd;
	beginInfo.framebuffer = pRenderPass->Framebuffer.Hnd[nextSwapchainIndex];
	beginInfo.renderArea.offset = { pRenderPass->Pos.x, pRenderPass->Pos.y };
	beginInfo.renderArea.extent = { pRenderPass->Extent.Width, pRenderPass->Extent.Height };
	beginInfo.clearValueCount = static_cast<uint32>(clearValues.Length());
	beginInfo.pClearValues = clearValues.First();

	vkCmdBeginRenderPass(Renderer.Device->GetCommandBuffer(), &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	clearValues.Empty();
}

void IFrameGraph::EndRenderPass(SRenderPass* pRenderPass)
{
	VkCommandBuffer cmd = Renderer.Device->GetCommandBuffer();
	const IRenderDevice::VulkanQueue& gfxQueue = Renderer.Device->GetGraphicsQueue();

	vkCmdEndRenderPass(cmd);

	VkImageSubresourceRange range = {};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.baseArrayLayer = 1;
	range.levelCount = 1;
	range.layerCount = 1;

	for (auto [hnd, img] : pRenderPass->ColorOutputs)
	{
		Renderer.Device->ImageBarrier(
			cmd,
			img->ImgHnd,
			&range,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			gfxQueue.FamilyIndex,
			gfxQueue.FamilyIndex
		);
	}

	auto [hnd, depthImg] = pRenderPass->DepthStencilOutput;

	if (depthImg)
	{
		range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		Renderer.Device->ImageBarrier(
			cmd,
			depthImg->ImgHnd,
			&range,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			gfxQueue.FamilyIndex,
			gfxQueue.FamilyIndex
		);
	}
}

bool IFrameGraph::AddColorInputFrom(const String64& AttId, Handle<SRenderPass> Src, Handle<SRenderPass> Dst)
{
	SRenderPass* src = GetRenderPass(Src);
	SRenderPass* dst = GetRenderPass(Dst);
	if (!src || !dst) { return false; }
	
	SImage* img = nullptr;
	Handle<SImage> imgHnd = HashIdentifier(AttId);

	for (auto [hnd, pImg] : src->ColorOutputs) 
	{
		if (imgHnd != hnd) { continue; }
		img = pImg;
		break;
	}

	if (!img) { return false; }

	dst->ColorInputs.Push({ imgHnd, img });

	return true;
}

bool IFrameGraph::AddColorOutput(const String64& AttId, Handle<SRenderPass> Hnd)
{
	SRenderPass* pRenderPass = GetRenderPass(Hnd);
	if (!pRenderPass) { return false; }

	Handle<SImage> imgHnd = HashIdentifier(AttId);
	
	ImageCreateInfo info = {};
	info.Identifier = imgHnd;
	info.Width = pRenderPass->Extent.Width;
	info.Height = pRenderPass->Extent.Height;
	info.Channels = 4;
	info.Type = Texture_Type_2D;

	Renderer.CreateImage(info);

	SImage* pImg = Renderer.Store->GetImage(imgHnd);
	VKT_ASSERT(pImg);

	pRenderPass->ColorOutputs.Push({ imgHnd, pImg });

	return true;
}

bool IFrameGraph::AddDepthStencilInputFrom(Handle<SRenderPass> Src, Handle<SRenderPass> Dst)
{
	SRenderPass* src = GetRenderPass(Src);
	SRenderPass* dst = GetRenderPass(Dst);
	if (!src || !dst) { return false; }
	
	auto [hnd, pImg] = src->DepthStencilOutput;
	if (!pImg) { return false; }

	dst->DepthStencilInput = src->DepthStencilOutput;

	return true;
}

bool IFrameGraph::AddDepthStencilOutput(Handle<SRenderPass> Hnd)
{
	SRenderPass* pRenderPass = GetRenderPass(Hnd);
	if (!pRenderPass) { return false; }
	if (pRenderPass->DepthStencilOutput.Key != INVALID_HANDLE) { return false; }

	ImageCreateInfo info = {};
	info.Channels = 4;
	info.Type = Texture_Type_2D;
	info.Width = pRenderPass->Extent.Width;
	info.Height = pRenderPass->Extent.Height;

	Handle<SImage> hnd = Renderer.CreateImage(info);
	if (hnd == INVALID_HANDLE) { return false; }

	SImage* pImg = Renderer.Store->GetImage(hnd);

	pRenderPass->DepthStencilOutput = { hnd, pImg };

	return true;
}

bool IFrameGraph::SetRenderPassExtent(Handle<SRenderPass> Hnd, WindowInfo::Extent2D Extent)
{
	SRenderPass* renderPass = Renderer.Store->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Extent = Extent;

	return true;
}

bool IFrameGraph::SetRenderPassOrigin(Handle<SRenderPass> Hnd, WindowInfo::Position Origin)
{
	SRenderPass* renderPass = Renderer.Store->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Pos = Origin;

	return true;
}

bool IFrameGraph::NoDefaultRender(Handle<SRenderPass> Hnd)
{
	SRenderPass* renderPass = Renderer.Store->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Flags.Set(RenderPass_Bit_No_Color_Render);
	renderPass->Flags.Set(RenderPass_Bit_No_DepthStencil_Render);

	return true;
}

bool IFrameGraph::NoDefaultColorRender(Handle<SRenderPass> Hnd)
{
	SRenderPass* renderPass = Renderer.Store->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Flags.Set(RenderPass_Bit_No_Color_Render);

	return true;
}

bool IFrameGraph::NoDefaultDepthStencilRender(Handle<SRenderPass> Hnd)
{
	SRenderPass* renderPass = Renderer.Store->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Flags.Set(RenderPass_Bit_No_DepthStencil_Render);

	return true;
}

Handle<SRenderPass> IFrameGraph::AddRenderPass(const RenderPassCreateInfo& CreateInfo)
{
	Handle<SRenderPass> hnd = HashIdentifier(CreateInfo.Identifier);
	SRenderPass* renderPass = Renderer.Store->NewRenderPass(hnd);

	if (!renderPass) { return INVALID_HANDLE; }

	renderPass->Extent = CreateInfo.Extent;
	renderPass->Pos = CreateInfo.Pos;
	renderPass->Depth = CreateInfo.Depth;

	return hnd;
}

size_t IFrameGraph::GetNumRenderPasses() const
{
	return RenderPasses.Length();
}

bool IFrameGraph::IsBuilt() const
{
	return Built;
}

void IFrameGraph::SetOutputExtent(uint32 Width, uint32 Height)
{
	SImage* colorImg = ColorImage.Value;
	SImage* depthStencilImg = DepthStencilImage.Value;

	colorImg->Width = Width;
	colorImg->Height = Height;
	depthStencilImg->Width = Width;
	depthStencilImg->Height = Height;
}

void IFrameGraph::Terminate()
{
	for (auto [hnd, pRenderPass] : RenderPasses)
	{
		
	}
}

//void IRFrameGraph::Destroy()
//{
//	for (auto& pair : RenderPasses)
//	{
//		RenderPass* renderPass = pair.Value;
//		gpu::DestroyRenderpass(*renderPass);
//		gpu::DestroyFramebuffer(*renderPass);
//	}
//	gpu::DestroyTexture(ColorImage);
//	gpu::DestroyTexture(DepthStencilImage);
//}

//bool IRFrameGraph::Compile()
//{
//	if (!RenderPasses.Length()) { return false; }
//	if (Compiled()) { return false; }
//
//	ColorImage.Channels = 4;
//	ColorImage.Usage.Set(Image_Usage_Transfer_Src);
//	ColorImage.Usage.Set(Image_Usage_Sampled);
//
//	DepthStencilImage = ColorImage;
//
//	ColorImage.Usage.Set(Image_Usage_Color_Attachment);
//	DepthStencilImage.Usage.Set(Image_Usage_Depth_Stencil_Attachment);
//
//	gpu::CreateTexture(ColorImage);
//	gpu::CreateTexture(DepthStencilImage);
//
//	for (Pair<RenderPassEnum, RenderPass*>& pair : RenderPasses)
//	{
//		RenderPass* renderPass = pair.Value;
//
//		if (!gpu::CreateRenderpass(*renderPass))  { return false; }
//		if (!gpu::CreateFramebuffer(*renderPass)) { return false; }
//	}
//
//	IsCompiled = true;
//
//	return true;
//}