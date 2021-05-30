#include "FrameGraph.h"
#include "Library/Algorithms/Hash.h"
#include "Renderer.h"

size_t IFrameGraph::_HashSeed = OS::GetPerfCounter();

Ref<SRenderPass> IFrameGraph::GetRenderPass(Handle<SRenderPass> Hnd)
{
	for (auto [hnd, pass] : RenderPasses)
	{
		if (hnd != Hnd) { continue; }
		return pass;
	}
	return Ref<SRenderPass>();
}

void IFrameGraph::ResetRenderPassBindState()
{
	for (auto& [hnd, renderPass] : RenderPasses)
	{
		renderPass->Bound = false;
	}
}

size_t IFrameGraph::HashIdentifier(const String64& Identifier)
{
	size_t id = 0;
	XXHash64(Identifier.First(), Identifier.Length(), &id, _HashSeed);
	return id;
}

bool IFrameGraph::CreateFramebuffer(Ref<SRenderPass> pRenderPass)
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
		if (vkCreateFramebuffer(Renderer.pDevice->GetDevice(), &info, nullptr, &pRenderPass->Framebuffer.Hnd[i]) != VK_SUCCESS)
		{
			VKT_ASSERT("Unable to create framebuffer" && false);
			return false;
		}
	}

	return true;
}

void IFrameGraph::DestroyFramebuffer(Ref<SRenderPass> pRenderPass)
{
	const VkDevice device = Renderer.pDevice->GetDevice();
	vkDeviceWaitIdle(device);
	for (size_t i = 0; i < MAX_SWAPCHAIN_IMAGE_ALLOWED; i++)
	{
		vkDestroyFramebuffer(device, pRenderPass->Framebuffer.Hnd[i], nullptr);
		pRenderPass->Framebuffer.Hnd[i] = VK_NULL_HANDLE;
	}
}

bool IFrameGraph::CreateRenderPass(Ref<SRenderPass> pRenderPass)
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
	subpassDescription.pipelineBindPoint = Renderer.pDevice->GetPipelineBindPoint(pRenderPass->Type);
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
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.subpassCount = 1;

	if (vkCreateRenderPass(Renderer.pDevice->GetDevice(), &renderPassInfo, nullptr, &pRenderPass->RenderPassHnd) != VK_SUCCESS)
	{
		VKT_ASSERT("Unable to create render pass." && false);
		return false;
	}

	return true;
}

void IFrameGraph::DestroyRenderPass(Ref<SRenderPass> pRenderPass)
{
	const VkDevice device = Renderer.pDevice->GetDevice();
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

bool IFrameGraph::AddColorInputFrom(const String64& AttId, Handle<SRenderPass> Src, Handle<SRenderPass> Dst)
{
	Ref<SRenderPass> src = GetRenderPass(Src);
	Ref<SRenderPass> dst = GetRenderPass(Dst);
	if (!src || !dst) { return false; }
	
	Ref<SImage> img;
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
	Ref<SRenderPass> pRenderPass = GetRenderPass(Hnd);
	if (!pRenderPass) { return false; }

	Handle<SImage> imgHnd = HashIdentifier(AttId);
	
	ImageCreateInfo info = {};
	info.Identifier = imgHnd;
	info.Width = pRenderPass->Extent.Width;
	info.Height = pRenderPass->Extent.Height;
	info.Channels = 4;
	info.Type = Texture_Type_2D;

	Renderer.CreateImage(info);

	SImage* pImg = Renderer.pStore->GetImage(imgHnd);
	VKT_ASSERT(pImg);

	pRenderPass->ColorOutputs.Push({ imgHnd, pImg });

	return true;
}

bool IFrameGraph::AddDepthStencilInputFrom(Handle<SRenderPass> Src, Handle<SRenderPass> Dst)
{
	Ref<SRenderPass> src = GetRenderPass(Src);
	Ref<SRenderPass> dst = GetRenderPass(Dst);
	if (!src || !dst) { return false; }
	
	auto [hnd, pImg] = src->DepthStencilOutput;
	if (!pImg) { return false; }

	dst->DepthStencilInput = src->DepthStencilOutput;

	return true;
}

bool IFrameGraph::AddDepthStencilOutput(Handle<SRenderPass> Hnd)
{
	Ref<SRenderPass> pRenderPass = GetRenderPass(Hnd);
	if (!pRenderPass) { return false; }
	if (pRenderPass->DepthStencilOutput.Key != INVALID_HANDLE) { return false; }

	ImageCreateInfo info = {};
	info.Channels = 4;
	info.Type = Texture_Type_2D;
	info.Width = pRenderPass->Extent.Width;
	info.Height = pRenderPass->Extent.Height;

	Handle<SImage> hnd = Renderer.CreateImage(info);
	if (hnd == INVALID_HANDLE) { return false; }

	SImage* pImg = Renderer.pStore->GetImage(hnd);

	pRenderPass->DepthStencilOutput = { hnd, pImg };

	return true;
}

bool IFrameGraph::SetRenderPassExtent(Handle<SRenderPass> Hnd, WindowInfo::Extent2D Extent)
{
	SRenderPass* renderPass = Renderer.pStore->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Extent = Extent;

	return true;
}

bool IFrameGraph::SetRenderPassOrigin(Handle<SRenderPass> Hnd, WindowInfo::Position Origin)
{
	SRenderPass* renderPass = Renderer.pStore->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Pos = Origin;

	return true;
}

bool IFrameGraph::NoDefaultRender(Handle<SRenderPass> Hnd)
{
	SRenderPass* renderPass = Renderer.pStore->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Flags.Set(RenderPass_Bit_No_Color_Render);
	renderPass->Flags.Set(RenderPass_Bit_No_DepthStencil_Render);

	return true;
}

bool IFrameGraph::NoDefaultColorRender(Handle<SRenderPass> Hnd)
{
	SRenderPass* renderPass = Renderer.pStore->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Flags.Set(RenderPass_Bit_No_Color_Render);

	return true;
}

bool IFrameGraph::NoDefaultDepthStencilRender(Handle<SRenderPass> Hnd)
{
	SRenderPass* renderPass = Renderer.pStore->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Flags.Set(RenderPass_Bit_No_DepthStencil_Render);

	return true;
}

Handle<SRenderPass> IFrameGraph::AddRenderPass(const RenderPassCreateInfo& CreateInfo)
{
	Handle<SRenderPass> hnd = HashIdentifier(CreateInfo.Identifier);
	SRenderPass* pRenderPass = Renderer.pStore->NewRenderPass(hnd);

	if (!pRenderPass) { return INVALID_HANDLE; }
	//if (CreateInfo.PipelineHnd == INVALID_HANDLE) { return INVALID_HANDLE; }

	//Ref<SPipeline> pPipeline = Renderer.pStore->GetPipeline(CreateInfo.PipelineHnd);
	//if (!pPipeline) { return INVALID_HANDLE; }

	pRenderPass->Extent = CreateInfo.Extent;
	pRenderPass->Pos = CreateInfo.Pos;
	pRenderPass->Depth = CreateInfo.Depth;
	pRenderPass->Hnd = hnd;

	//pRenderPass->pPipeline = pPipeline;

	//Ref<SDescriptorSet> pSet = Renderer.pStore->GetDescriptorSet(CreateInfo.DescriptorSetHnd);
	//if (pSet)
	//{
	//	pRenderPass->pSet = pSet;
	//}

	RenderPasses.Push({ hnd, pRenderPass });

	return hnd;
}

bool IFrameGraph::GetRenderPassColorOutputCount(Handle<SRenderPass> Hnd, uint32& Count)
{
	Ref<SRenderPass> pRenderPass = Renderer.pStore->GetRenderPass(Hnd);
	if (!pRenderPass) { return false; }

	Count = static_cast<uint32>(pRenderPass->ColorOutputs.Length());

	return true;
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
	Ref<SImage> colorImg = ColorImage.Value;
	Ref<SImage> depthStencilImg = DepthStencilImage.Value;

	colorImg->Width = Width;
	colorImg->Height = Height;
	depthStencilImg->Width = Width;
	depthStencilImg->Height = Height;
}

bool IFrameGraph::Initialize(const Extent2D& Extent)
{
	this->Extent = Extent;
	ImageCreateInfo info = {};
	info.Width = Extent.Width;
	info.Height = Extent.Height;
	info.Channels = 4;
	info.Type = Texture_Type_2D;

	Handle<SImage> colorImgHnd = Renderer.CreateImage(info);
	Handle<SImage> depthStencilImgHnd = Renderer.CreateImage(info);

	if (colorImgHnd == INVALID_HANDLE || depthStencilImgHnd == INVALID_HANDLE)
	{
		return false;
	}

	Ref<SImage> pColorImg = Renderer.pStore->GetImage(colorImgHnd);
	Ref<SImage> pDepthStencilImg = Renderer.pStore->GetImage(depthStencilImgHnd);

	if (!pColorImg || !pDepthStencilImg) { return false; }

	pColorImg->Usage.Set(Image_Usage_Transfer_Src);
	pColorImg->Usage.Set(Image_Usage_Sampled);

	pDepthStencilImg->Usage = pColorImg->Usage;

	pColorImg->Usage.Set(Image_Usage_Color_Attachment);
	pDepthStencilImg->Usage.Set(Image_Usage_Depth_Stencil_Attachment);

	ColorImage = { colorImgHnd, pColorImg };
	DepthStencilImage = { depthStencilImgHnd, pDepthStencilImg };

	return true;
}

void IFrameGraph::Terminate()
{
	for (auto [rHnd, pRenderPass] : RenderPasses)
	{
		for (auto [iHnd, pImg] : pRenderPass->ColorOutputs)
		{
			Renderer.DestroyImage(iHnd);
		}
		DestroyRenderPass(pRenderPass);
		DestroyFramebuffer(pRenderPass);
	}
	Renderer.DestroyImage(ColorImage.Key);
	Renderer.DestroyImage(DepthStencilImage.Key);
}

bool IFrameGraph::Build()
{
	if (!RenderPasses.Length()) { return false; }
	if (IsBuilt()) { return false; }

	if (!Renderer.BuildImage(ColorImage.Key)) { return false; }
	if (!Renderer.BuildImage(DepthStencilImage.Key)) { return false; }

	// NOTE(Ygsm):
	// Should probably sort them according to dependencies in the future.
	// However, manual dependency declaration will suffice for now.

	for (auto [hnd, pRenderPass] : RenderPasses)
	{
		size_t handleValue = hnd;
		for (auto [imgHnd, pImg] : pRenderPass->ColorOutputs)
		{
			if (!Renderer.BuildImage(imgHnd)) { return false; }
		}

		if (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output))
		{
			if (!Renderer.BuildImage(pRenderPass->DepthStencilOutput.Key)) { return false; }
		}

		if (!CreateFramebuffer(pRenderPass)) { return false; }
		if (!CreateRenderPass(pRenderPass)) { return false; }

		IRenderSystem::DrawManager::_InstancedDraws.Insert(handleValue, {});
		IRenderSystem::DrawManager::_NonInstancedDraws.Insert(handleValue, {});
	}

	Built = true;

	return Built;
}