#include "FrameGraph.h"
#include "Library/Algorithms/Hash.h"
#include "Renderer.h"
#include "API/Device.h"

//struct SRenderPass
//{
//	using VulkanFramebuffer = IRenderDevice::VulkanFramebuffer;
//	using AttachmentContainer = StaticArray<Attachment, MAX_RENDERPASS_ATTACHMENT_COUNT>;
//
//	Handle<SRenderPass> Hnd; // Not sure if I like this ... 
//	IFrameGraph* pOwner;
//	AttachmentContainer ColorInputs;
//	AttachmentContainer ColorOutputs;
//	VulkanFramebuffer Framebuffer;
//	Attachment DepthStencilInput;
//	Attachment DepthStencilOutput;
//	VkRenderPass RenderPassHnd;
//	BitSet<ERenderPassFlagBits> Flags;
//	//astl::Ref<SDescriptorSet> pSet;
//	//astl::Ref<SPipeline> pPipeline;
//	Extent2D Extent;
//	Position Pos;
//	float32 Depth;
//	ERenderPassType Type;
//	bool Bound = false;
//};

size_t IFrameGraph::_HashSeed = OS::GetPerfCounter();

astl::Ref<SRenderPass> IFrameGraph::GetRenderPass(Handle<SRenderPass> Hnd)
{
	for (auto [hnd, pass] : RenderPasses)
	{
		if (hnd != Hnd) { continue; }
		return pass;
	}
	return astl::Ref<SRenderPass>();
}

size_t IFrameGraph::HashIdentifier(const astl::String64& Identifier)
{
	size_t id = 0;
	astl::XXHash64(Identifier.First(), Identifier.Length(), &id, _HashSeed);
	return id;
}

bool IFrameGraph::CreateFramebuffer(astl::Ref<SRenderPass> pRenderPass)
{
	size_t totalOutputs = (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output)) ?
		pRenderPass->ColorOutputs.Length() + 1 : pRenderPass->ColorOutputs.Length();

	astl::StaticArray<VkFramebufferAttachmentImageInfo, MAX_RENDERPASS_ATTACHMENT_COUNT> attImgInfos;

	VkFormat colorFormat = VK_FORMAT_R8G8B8A8_SRGB;
	VkFormat depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;
	VkFramebufferAttachmentImageInfo attImgInfo = {};
	attImgInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
	attImgInfo.width = pRenderPass->Extent.Width;
	attImgInfo.height = pRenderPass->Extent.Height;
	attImgInfo.layerCount = 1;
	attImgInfo.viewFormatCount = 1;
	attImgInfo.pViewFormats = &colorFormat;

	if (!pRenderPass->Flags.Has(RenderPass_Bit_No_Color_Render))
	{
		attImgInfo.usage = Renderer.GetImageUsageFlags(ColorImage.pImg);
		attImgInfos.Push(attImgInfo);
	}

	for (auto& [hnd, pImg] : pRenderPass->ColorOutputs)
	{
		attImgInfo.usage = Renderer.GetImageUsageFlags(pImg);
		attImgInfos.Push(attImgInfo);
	}

	if (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output))
	{
		attImgInfo.usage = Renderer.GetImageUsageFlags(pRenderPass->DepthStencilOutput.pImg);
		attImgInfo.pViewFormats = &depthStencilFormat;
		attImgInfos.Push(attImgInfo);
	}
	else if (!pRenderPass->Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
	{
	
		attImgInfo.usage = Renderer.GetImageUsageFlags(pRenderPass->pOwner->DepthStencilImage.pImg);
		attImgInfo.pViewFormats = &depthStencilFormat;
		attImgInfos.Push(attImgInfo);
	}

	//Array<VkImageView> imageViews(totalOutputs);

	//if (!pRenderPass->Flags.Has(RenderPass_Bit_No_Color_Render))
	//{
	//	imageViews.Push(pRenderPass->pOwner->ColorImage.pImg->ImgViewHnd);
	//}

	//for (auto& [hnd, pImg] : pRenderPass->ColorOutputs)
	//{
	//	imageViews.Push(pImg->ImgViewHnd);
	//}

	//if (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output))
	//{
	//	imageViews.Push(pRenderPass->DepthStencilOutput.pImg->ImgViewHnd);
	//}
	//else
	//{
	//	if (!pRenderPass->Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
	//	{
	//		imageViews.Push(pRenderPass->pOwner->DepthStencilImage.pImg->ImgViewHnd);
	//	}
	//}

	VkFramebufferAttachmentsCreateInfo attCreateInfo = {};
	attCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
	attCreateInfo.attachmentImageInfoCount = static_cast<uint32>(attImgInfos.Length());
	attCreateInfo.pAttachmentImageInfos = attImgInfos.First();

	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
	info.pNext = &attCreateInfo;
	info.width = pRenderPass->Extent.Width;
	info.height = pRenderPass->Extent.Height;
	info.layers = 1;
	info.attachmentCount = attCreateInfo.attachmentImageInfoCount;
	info.renderPass = pRenderPass->RenderPassHnd;

	if (vkCreateFramebuffer(Renderer.pDevice->GetDevice(), &info, nullptr, &pRenderPass->Framebuffer) != VK_SUCCESS)
	{
		VKT_ASSERT("Unable to create framebuffer" && false);
		return false;
	}

	return true;
}

void IFrameGraph::DestroyFramebuffer(astl::Ref<SRenderPass> pRenderPass)
{
	Renderer.pDevice->MoveToZombieList(
		pRenderPass->Framebuffer, 
		IRenderDevice::EHandleType::Handle_Type_Framebuffer
	);
}

bool IFrameGraph::CreateRenderPass(astl::Ref<SRenderPass> pRenderPass)
{
	uint32 offset = 0;
	uint32 colorAttCount = 0;
	const size_t numPasses = pRenderPass->pOwner->GetNumRenderPasses();

	astl::Array<VkAttachmentDescription> descriptions;
	astl::Array<VkAttachmentReference> references;

	if (!pRenderPass->Flags.Has(RenderPass_Bit_No_Color_Render))
	{
		VkAttachmentDescription& desc = descriptions.Insert(VkAttachmentDescription());
		desc.format = VK_FORMAT_R8G8B8A8_SRGB;
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		//desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		//desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		switch (pRenderPass->Order.Value())
		{
		case RenderPass_Order_Value_First:
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			break;
		case RenderPass_Order_Value_InBetween:
			desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			break;
		case RenderPass_Order_Value_FirstLast:
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			desc.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			break;
		case RenderPass_Order_Value_Last:
		default:
			desc.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			break;
		}

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

	//StaticArray<VkSubpassDependency, 2> subpassDependencies;

	//VkSubpassDependency subpassDependency = {};
	//subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	//subpassDependency.dstSubpass = 0;
	//subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	//subpassDependency.srcAccessMask = 0;
	//subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	//subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	//subpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	//subpassDependencies.Push(subpassDependency);

	//subpassDependency.srcSubpass = 0;
	//subpassDependency.dstSubpass = VK_SUBPASS_EXTERNAL;
	//subpassDependencies.Push(subpassDependency);

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
	//renderPassInfo.pDependencies = &subpassDependency;
	//renderPassInfo.dependencyCount = 1;

	if (vkCreateRenderPass(Renderer.pDevice->GetDevice(), &renderPassInfo, nullptr, &pRenderPass->RenderPassHnd) != VK_SUCCESS)
	{
		VKT_ASSERT("Unable to create render pass." && false);
		return false;
	}

	return true;
}

void IFrameGraph::DestroyRenderPass(astl::Ref<SRenderPass> pRenderPass)
{
	Renderer.pDevice->MoveToZombieList(
		pRenderPass->RenderPassHnd, 
		IRenderDevice::EHandleType::Handle_Type_Renderpass
	);
	pRenderPass->RenderPassHnd = VK_NULL_HANDLE;
}

void IFrameGraph::SetupAttachments(astl::Ref<SImage> pColor, astl::Ref<SImage> pDepthStencil)
{
	pColor->Usage.Set(Image_Usage_Transfer_Src);
	pColor->Usage.Set(Image_Usage_Sampled);
	pDepthStencil->Usage = pColor->Usage;
	pColor->Usage.Set(Image_Usage_Color_Attachment);
	pDepthStencil->Usage.Set(Image_Usage_Depth_Stencil_Attachment);
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

bool IFrameGraph::AddColorInputFrom(Handle<SImage> Img, Handle<SRenderPass> Src, Handle<SRenderPass> Dst)
{
	astl::Ref<SRenderPass> src = GetRenderPass(Src);
	astl::Ref<SRenderPass> dst = GetRenderPass(Dst);
	if (!src || !dst) { return false; }
	
	SRenderPass::OAttachment* outAtt = nullptr;

	for (auto& att : src->ColorOutputs) 
	{
		if (att.Hnd != Img) { continue; }
		outAtt = &att;
		break;
	}

	if (!outAtt) { return false; }

	SRenderPass::IAttachment in = {};
	in.AtIndex = static_cast<uint32>(src->ColorOutputs.IndexOf(outAtt));
	in.pOwner = src;

	dst->ColorInputs.Push(astl::Move(in));

	return true;
}

Handle<SImage> IFrameGraph::AddColorOutput(Handle<SRenderPass> Hnd)
{
	astl::Ref<SRenderPass> pRenderPass = GetRenderPass(Hnd);
	if (!pRenderPass) { return INVALID_HANDLE; }

	Handle<SImage> hnd = Renderer.CreateImage(
		pRenderPass->Extent.Width,
		pRenderPass->Extent.Height,
		4,
		Texture_Type_2D,
    Texture_Format_Srgb
	);

	astl::Ref<SImage> pImg = Renderer.pStore->GetImage(hnd);
	VKT_ASSERT(pImg);

	pImg->Usage.Set(Image_Usage_Sampled);
	pImg->Usage.Set(Image_Usage_Color_Attachment);

	pRenderPass->ColorOutputs.Push({ hnd, pImg });

	return hnd;
}

bool IFrameGraph::AddDepthStencilInputFrom(Handle<SRenderPass> Src, Handle<SRenderPass> Dst)
{
	astl::Ref<SRenderPass> src = GetRenderPass(Src);
	astl::Ref<SRenderPass> dst = GetRenderPass(Dst);
	if (!src || !dst) { return false; }
	
	auto [hnd, pImg] = src->DepthStencilOutput;
	if (!pImg) { return false; }

	SRenderPass::IAttachment in = {};
	in.pOwner = src;
	in.AtIndex = 0;

	dst->DepthStencilInput = astl::Move(in);

	return true;
}

bool IFrameGraph::AddDepthStencilOutput(Handle<SRenderPass> Hnd)
{
	astl::Ref<SRenderPass> pRenderPass = GetRenderPass(Hnd);
	if (!pRenderPass) { return false; }
	if (pRenderPass->DepthStencilOutput.Hnd != INVALID_HANDLE) { return false; }

	Handle<SImage> hnd = Renderer.CreateImage(
										pRenderPass->Extent.Width,
										pRenderPass->Extent.Height,
										4,
										Texture_Type_2D,
                    Texture_Format_Srgb
									);

	if (hnd == INVALID_HANDLE) { return false; }

	astl::Ref<SImage> pImg = Renderer.pStore->GetImage(hnd);

	pImg->Usage.Set(Image_Usage_Sampled);
	pImg->Usage.Set(Image_Usage_Depth_Stencil_Attachment);

	pRenderPass->DepthStencilOutput = { hnd, pImg };

	return true;
}

bool IFrameGraph::SetRenderPassExtent(Handle<SRenderPass> Hnd, Extent2D Extent)
{
	astl::Ref<SRenderPass> renderPass = Renderer.pStore->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Extent = Extent;
	renderPass->Rebuild = true;

	return true;
}

bool IFrameGraph::SetRenderPassOrigin(Handle<SRenderPass> Hnd, Position Origin)
{
	astl::Ref<SRenderPass> renderPass = Renderer.pStore->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Pos = Origin;

	return true;
}

bool IFrameGraph::NoDefaultRender(Handle<SRenderPass> Hnd)
{
	astl::Ref<SRenderPass> renderPass = Renderer.pStore->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Flags.Set(RenderPass_Bit_No_Color_Render);
	renderPass->Flags.Set(RenderPass_Bit_No_DepthStencil_Render);

	return true;
}

bool IFrameGraph::NoDefaultColorRender(Handle<SRenderPass> Hnd)
{
	astl::Ref<SRenderPass> renderPass = Renderer.pStore->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Flags.Set(RenderPass_Bit_No_Color_Render);

	return true;
}

bool IFrameGraph::NoDefaultDepthStencilRender(Handle<SRenderPass> Hnd)
{
	astl::Ref<SRenderPass> renderPass = Renderer.pStore->GetRenderPass(Hnd);
	if (!renderPass) { return false; }

	renderPass->Flags.Set(RenderPass_Bit_No_DepthStencil_Render);

	return true;
}

Handle<SRenderPass> IFrameGraph::AddRenderPass(const RenderPassCreateInfo& CreateInfo)
{
	Handle<SRenderPass> hnd = HashIdentifier(CreateInfo.Identifier);
	astl::Ref<SRenderPass> pRenderPass = Renderer.pStore->NewRenderPass(hnd);

	if (!pRenderPass) { return INVALID_HANDLE; }

	pRenderPass->Extent = CreateInfo.Extent;
	pRenderPass->Pos = CreateInfo.Pos;
	pRenderPass->Depth = CreateInfo.Depth;
	pRenderPass->Hnd = hnd;
	pRenderPass->pOwner = this;
	RenderPasses.Push({ hnd, pRenderPass });

	return hnd;
}

bool IFrameGraph::GetRenderPassColorOutputCount(Handle<SRenderPass> Hnd, uint32& Count)
{
	astl::Ref<SRenderPass> pRenderPass = Renderer.pStore->GetRenderPass(Hnd);
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
	Extent.Width = Width;
	Extent.Height = Height;
	Built = false;
}

void IFrameGraph::OnWindowResize()
{
	if (!Built)
	{
		Renderer.DestroyImage(ColorImage.Hnd);
		Renderer.DestroyImage(DepthStencilImage.Hnd);	

		ColorImage.Hnd = Renderer.CreateImage(Extent.Width, Extent.Height, 4, Texture_Type_2D, Texture_Format_Srgb);
		DepthStencilImage.Hnd = Renderer.CreateImage(Extent.Width, Extent.Height, 4, Texture_Type_2D, Texture_Format_Srgb);

		ColorImage.pImg = Renderer.pStore->GetImage(ColorImage.Hnd);
		DepthStencilImage.pImg = Renderer.pStore->GetImage(DepthStencilImage.Hnd);

		SetupAttachments(ColorImage.pImg, DepthStencilImage.pImg);

		//Renderer.BuildImage(ColorImage.Hnd);
		//Renderer.BuildImage(DepthStencilImage.Hnd);

		Built = true;
	}

	for (auto& [key, pRenderPass] : RenderPasses)
	{
		if (pRenderPass->Rebuild)
		{
			const Extent2D& extent = pRenderPass->Extent;
			for (SRenderPass::OAttachment& attachment : pRenderPass->ColorOutputs)
			{
				Renderer.DestroyImage(attachment.Hnd);
				attachment.Hnd = Renderer.CreateImage(extent.Width, extent.Height, 4, Texture_Type_2D, Texture_Format_Srgb);
				attachment.pImg = Renderer.pStore->GetImage(attachment.Hnd);

				attachment.pImg->Usage.Set(Image_Usage_Sampled);
				attachment.pImg->Usage.Set(Image_Usage_Color_Attachment);

				//Renderer.BuildImage(attachment.Hnd);
			}

			if (pRenderPass->DepthStencilOutput.Hnd != INVALID_HANDLE)
			{
				SRenderPass::OAttachment& outDepthStencil = pRenderPass->DepthStencilOutput;
				Renderer.DestroyImage(outDepthStencil.Hnd);
				outDepthStencil.Hnd = Renderer.CreateImage(extent.Width, extent.Height, 4, Texture_Type_2D, Texture_Format_Srgb);
				outDepthStencil.pImg = Renderer.pStore->GetImage(outDepthStencil.Hnd);

				outDepthStencil.pImg->Usage.Set(Image_Usage_Sampled);
				outDepthStencil.pImg->Usage.Set(Image_Usage_Depth_Stencil_Attachment);

				//Renderer.BuildImage(outDepthStencil.Hnd);
			}

			DestroyFramebuffer(pRenderPass);
			CreateFramebuffer(pRenderPass);

			pRenderPass->Rebuild = false;
		}
	}
}

bool IFrameGraph::Initialize(const Extent2D& Extent)
{
	this->Extent = Extent;

	Handle<SImage> colorImgHnd = Renderer.CreateImage(Extent.Width, Extent.Height, 4, Texture_Type_2D, Texture_Format_Srgb);
	Handle<SImage> depthStencilImgHnd = Renderer.CreateImage(Extent.Width, Extent.Height, 4, Texture_Type_2D, Texture_Format_Srgb);

	if (colorImgHnd == INVALID_HANDLE || depthStencilImgHnd == INVALID_HANDLE)
	{
		return false;
	}

	astl::Ref<SImage> pColorImg = Renderer.pStore->GetImage(colorImgHnd);
	astl::Ref<SImage> pDepthStencilImg = Renderer.pStore->GetImage(depthStencilImgHnd);

	if (!pColorImg || !pDepthStencilImg) { return false; }

	SetupAttachments(pColorImg, pDepthStencilImg);

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
	Renderer.DestroyImage(ColorImage.Hnd);
	Renderer.DestroyImage(DepthStencilImage.Hnd);
}

bool IFrameGraph::Build()
{
	if (!RenderPasses.Length()) { return false; }
	if (IsBuilt()) { return false; }

	//if (!Renderer.BuildImage(ColorImage.Hnd)) { return false; }
	//if (!Renderer.BuildImage(DepthStencilImage.Hnd)) { return false; }

	// NOTE(Ygsm):
	// Should probably sort them according to dependencies in the future.
	// However, manual dependency declaration will suffice for now.

	for (size_t i = 0; i < RenderPasses.Length(); i++)
	{
		RenderPasses[i].Value->Order = RenderPass_Order_InBetween;

		if (!i || i == (RenderPasses.Length() - 1))
		{
			RenderPasses[i].Value->Order.Assign(0);
			if (!i)
			{
				RenderPasses[i].Value->Order.Set(RenderPass_Order_First);
			}
			if (i == (RenderPasses.Length() - 1))
			{
				RenderPasses[i].Value->Order.Set(RenderPass_Order_Last);
			}
		}
	}

  Renderer.Drawables.Reserve(RenderPasses.Length());

	for (auto [hnd, pRenderPass] : RenderPasses)
	{
		size_t handleValue = hnd;
		//for (auto [imgHnd, pImg] : pRenderPass->ColorOutputs)
		//{
		//	if (!Renderer.BuildImage(imgHnd)) { return false; }
		//}

		//if (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output))
		//{
		//	if (!Renderer.BuildImage(pRenderPass->DepthStencilOutput.Hnd)) { return false; }
		//}

		if (!CreateRenderPass(pRenderPass)) { return false; }
		if (!CreateFramebuffer(pRenderPass)) { return false; }

    size_t index = Renderer.Drawables.Push(astl::Array<DrawCommand>());
    pRenderPass->IndexForDraw = static_cast<uint32>(index);
	}

	Built = true;

	return Built;
}
