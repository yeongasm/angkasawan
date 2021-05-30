#include "Renderer.h"
#include "Library/Containers/Node.h"
#include "Library/Algorithms/QuickSort.h"
#include "Library/Random/Xoroshiro.h"
#include "RenderAbstracts/StagingManager.h"
#include "API/Vk/Src/spirv_reflect.h"
#include "RenderAbstracts/FrameGraph.h"

Handle<ISystem> g_RenderSystemHandle;
LinearAllocator g_RenderSystemAllocator;
Xoroshiro64 g_Uid(OS::GetPerfCounter());

LinearAllocator IRenderSystem::DrawManager::_DrawCommandAllocator = {};
LinearAllocator IRenderSystem::DrawManager::_TransformAllocator = {};
uint32 IRenderSystem::DrawManager::_NumDrawables = 0;

IRenderSystem::DescriptorUpdate::Container<SImage*>  IRenderSystem::DescriptorUpdate::_Images = {};
IRenderSystem::DescriptorUpdate::Container<SMemoryBuffer*>  IRenderSystem::DescriptorUpdate::_Buffers = {};

Map<size_t, IRenderSystem::DrawManager::EntryContainer> IRenderSystem::DrawManager::_InstancedDraws = {};
Map<size_t, IRenderSystem::DrawManager::EntryContainer> IRenderSystem::DrawManager::_NonInstancedDraws = {};

LinearAllocator IRenderSystem::BindableManager::_BindableAllocator = {};
Map<size_t, IRenderSystem::BindableManager::BindableRange> IRenderSystem::BindableManager::_Bindables = {};

uint32 IRenderSystem::CalcStrideForFormat(EShaderAttribFormat Format)
{
	switch (Format)
	{
	case Shader_Attrib_Type_Float:
		return 4;
	case Shader_Attrib_Type_Vec2:
		return 8;
	case Shader_Attrib_Type_Vec3:
		return 12;
	case Shader_Attrib_Type_Vec4:
		return 16;
	default:
		break;
	}
	return 0;
}

void IRenderSystem::PreprocessShader(Ref<SShader> pShader)
{
	uint32 count = 0;
	spv_reflect::ShaderModule reflection(pShader->SpirV.Length() * sizeof(uint32), pShader->SpirV.First());
	reflection.EnumerateInputVariables(&count, nullptr);

	if (count)
	{
		static auto sortLambda = [](const ShaderAttrib& A, const ShaderAttrib& B) -> bool {
			return A.Location < B.Location;
		};

		ShaderAttrib attribute = {};
		Array<SpvReflectInterfaceVariable*> variables(count + 1);
		reflection.EnumerateInputVariables(&count, variables.First());
		variables[count] = {};

		for (auto var : variables)
		{
			if (!var) continue;

			if (var->type_description->op == SpvOpTypeMatrix)
			{
				for (uint32 i = 0; i < 4; i++)
				{
					attribute.Format = Shader_Attrib_Type_Vec4;
					attribute.Location = var->location + i;
					attribute.Offset = 0;
					pShader->Attributes.Push(Move(attribute));
				}
				continue;
			}
			else
			{
				switch (var->format)
				{
				case SPV_REFLECT_FORMAT_R32_SFLOAT:
					attribute.Format = Shader_Attrib_Type_Float;
					break;
				case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
					attribute.Format = Shader_Attrib_Type_Vec2;
					break;
				case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
					attribute.Format = Shader_Attrib_Type_Vec3;
					break;
				case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
					attribute.Format = Shader_Attrib_Type_Vec4;
					break;
				}
			}
			attribute.Location = var->location;
			attribute.Offset = 0;
			pShader->Attributes.Push(Move(attribute));
		}

		QuickSort(pShader->Attributes, sortLambda);
	}
}

// TODO(Ygsm):
// Figure this out in the long run. Descriptor set layout creation should not be done manually.
// 
//void IRenderSystem::GetShaderDescriptorLayoutInformation(Ref<SShader> pShader)
//{
//	uint32 count = 0;
//	spv_reflect::ShaderModule reflection(pShader->SpirV.Length() * sizeof(uint32), pShader->SpirV.First());
//	reflection.EnumerateDescriptorBindings(&count, nullptr);
//
//	if (count)
//	{
//		// NOTE: Do this later ... 
//	}
//}

void IRenderSystem::BindBindable(SBindable& Bindable)
{
	if (Bindable.Bound) { return; }

	VkCommandBuffer cmd = pDevice->GetCommandBuffer();
	const uint32 index = pDevice->GetCurrentFrameIndex();
	switch (Bindable.Type)
	{
	case EBindableType::Bindable_Type_Descriptor_Set: 
		{
			StaticArray<uint32, MAX_DESCRIPTOR_BINDING_UPDATES> offsets;
			for (SDescriptorSetLayout::Binding& binding : Bindable.pSet->pLayout->Bindings)
			{
				if (binding.Type == Descriptor_Type_Dynamic_Storage_Buffer ||
					binding.Type == Descriptor_Type_Dynamic_Uniform_Buffer)
				{
					offsets.Push(static_cast<uint32>(binding.Offset[index]));
				}
			}
			vkCmdBindDescriptorSets(
				cmd,
				pDevice->GetPipelineBindPoint(Bindable.pSet->pLayout->pPipeline->BindPoint),
				Bindable.pSet->pLayout->pPipeline->LayoutHnd,
				Bindable.pSet->Slot,
				1,
				&Bindable.pSet->Hnd[index],
				static_cast<uint32>(offsets.Length()),
				offsets.First()
			);
			break;
		}
	case EBindableType::Bindable_Type_Pipeline:
	default:
		vkCmdBindPipeline(
			cmd,
			pDevice->GetPipelineBindPoint(Bindable.pPipeline->BindPoint),
			Bindable.pPipeline->Hnd
		);
		break;
	}
	Bindable.Bound = true;
}

void IRenderSystem::BindBindablesForRenderpass(Handle<SRenderPass> Hnd)
{
	BindableManager::BindableRange* range = BindableManager::_Bindables.Get(Hnd);
	VKT_ASSERT(range && "Range with render pass does not exist.");
	ForwardNode<SBindable>* bindableNode = range->pBegin;
	while (bindableNode)
	{
		BindBindable(bindableNode->Data);
		bindableNode = bindableNode->Next;
	}
}

void IRenderSystem::BindBuffers()
{
	VkCommandBuffer cmd = pDevice->GetCommandBuffer();
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &VertexBuffer.Value->Hnd, &offset);
	vkCmdBindVertexBuffers(cmd, 1, 1, &InstanceBuffer.Value->Hnd, &offset);
	vkCmdBindIndexBuffer(cmd, IndexBuffer.Value->Hnd, 0, VK_INDEX_TYPE_UINT32);
}

void IRenderSystem::BeginRenderPass(Ref<SRenderPass> pRenderPass)
{
	if (pRenderPass->Bound) { return; }

	using ClearValues = StaticArray<VkClearValue, MAX_RENDERPASS_ATTACHMENT_COUNT>;
	using DynamicOffsets = StaticArray<uint32, MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS>;

	ClearValues clearValues;
	DynamicOffsets dynamicOffsets;

	bool hasDepthStencil = false;
	const uint32 nextSwapchainIndex = pDevice->GetNextSwapchainImageIndex();
	const uint32 currentFrameIndex = pDevice->GetCurrentFrameIndex();

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
	//if (pRenderPass->pSet)
	//{
	//	auto& pSet = pRenderPass->pSet;
	//	for (const SDescriptorSetLayout::Binding& binding : pSet->pLayout->Bindings)
	//	{
	//		if (binding.Type != Descriptor_Type_Dynamic_Storage_Buffer &&
	//			binding.Type != Descriptor_Type_Dynamic_Uniform_Buffer)
	//		{
	//			continue;
	//		}
	//		dynamicOffsets.Push(binding.Offset[currentFrameIndex]);
	//	}

	//	vkCmdBindDescriptorSets(pDevice->GetCommandBuffer(),
	//		pDevice->GetPipelineBindPoint(pRenderPass->Type),
	//		pRenderPass->pPipeline->LayoutHnd,
	//		pSet->Slot,
	//		1,
	//		&pSet->Hnd[currentFrameIndex],
	//		static_cast<uint32>(dynamicOffsets.Length()),
	//		dynamicOffsets.First()
	//	);
	//}

	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.renderPass = pRenderPass->RenderPassHnd;
	beginInfo.framebuffer = pRenderPass->Framebuffer.Hnd[nextSwapchainIndex];
	beginInfo.renderArea.offset = { static_cast<int32>(pRenderPass->Pos.x), static_cast<int32>(pRenderPass->Pos.y) };
	beginInfo.renderArea.extent = { pRenderPass->Extent.Width, pRenderPass->Extent.Height };
	beginInfo.clearValueCount = static_cast<uint32>(clearValues.Length());
	beginInfo.pClearValues = clearValues.First();

	vkCmdBeginRenderPass(pDevice->GetCommandBuffer(), &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	pRenderPass->Bound = true;
}

void IRenderSystem::EndRenderPass(Ref<SRenderPass> pRenderPass)
{
	VkCommandBuffer cmd = pDevice->GetCommandBuffer();
	const IRenderDevice::VulkanQueue& gfxQueue = pDevice->GetGraphicsQueue();

	vkCmdEndRenderPass(cmd);

	VkImageSubresourceRange range = {};
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	range.baseMipLevel = 0;
	range.baseArrayLayer = 1;
	range.levelCount = 1;
	range.layerCount = 1;

	for (auto [hnd, img] : pRenderPass->ColorOutputs)
	{
		pDevice->ImageBarrier(
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
		pDevice->ImageBarrier(
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

void IRenderSystem::RecordDrawCommand(const DrawCommand& Command)
{
	const VkCommandBuffer& cmd = pDevice->GetCommandBuffer();
	if (!Command.NumIndices)
	{
		vkCmdDraw(cmd, Command.NumVertices, Command.InstanceCount, Command.VertexOffset, Command.InstanceOffset);
		return;
	}
	vkCmdDrawIndexed(cmd, Command.NumIndices, Command.InstanceCount, Command.IndexOffset, Command.VertexOffset, Command.InstanceOffset);
}

void IRenderSystem::PrepareDrawCommands()
{
	const uint32 currentFrameIndex = pDevice->GetCurrentFrameIndex();
	StaticArray<math::mat4, DrawManager::MaxDrawablesCount> transforms;

	uint32 baseOffset = DrawManager::MaxDrawablesCount * currentFrameIndex;
	uint32 firstInstance = 0;

	for (auto& [key, instancedEntries] : DrawManager::_InstancedDraws)
	{
		DrawManager::EntryContainer& nonInstancedEntries = *DrawManager::_NonInstancedDraws.Get(key);

		for (DrawManager::InstanceEntry& entry : instancedEntries)
		{
			auto& transformRange = entry.TransformRange;
			while (transformRange.pBegin)
			{
				transforms.Push(Move(entry.TransformRange.pBegin->Data));
				transformRange.pBegin = transformRange.pBegin->Next;
			}

			auto& drawCmds = entry.DrawRange;
			while (drawCmds.pBegin)
			{
				drawCmds.pBegin->Data.InstanceOffset = baseOffset + firstInstance;
				DrawCommands.Push(Move(drawCmds.pBegin->Data));
			}
			firstInstance = static_cast<uint32>(transforms.Length());
		}

		for (DrawManager::InstanceEntry& entry : nonInstancedEntries)
		{
			auto& transformRange = entry.TransformRange;
			transforms.Push(Move(entry.TransformRange.pBegin->Data));

			auto& drawCmds = entry.DrawRange;
			while (drawCmds.pBegin)
			{
				drawCmds.pBegin->Data.InstanceOffset = baseOffset + firstInstance;
				DrawCommands.Push(Move(drawCmds.pBegin->Data));
			}
			firstInstance = static_cast<uint32>(transforms.Length());
		}
	}

	uint8* pData = reinterpret_cast<uint8*>(InstanceBuffer.Value->pData + baseOffset);
	IMemory::Memcpy(pData, transforms.First(), DrawManager::_NumDrawables * sizeof(math::mat4));
}

void IRenderSystem::Clear()
{
	DrawManager::_DrawCommandAllocator.FlushMemory();
	DrawManager::_TransformAllocator.FlushMemory();

	for (auto& pair : DrawManager::_InstancedDraws)
	{
		pair.Value.Empty();
	}

	for (auto& pair : DrawManager::_NonInstancedDraws)
	{
		pair.Value.Empty();
	}

	//DrawManager::_InstancedDraws.Empty();
	//DrawManager::_NonInstancedDraws.Empty();

	BindableManager::_BindableAllocator.FlushMemory();
	
	for (auto& pair : BindableManager::_Bindables)
	{
		pair.Value = {};
	}
	//BindableManager::_Bindables.Empty();

	DrawCommands.Empty();
	pFrameGraph->ResetRenderPassBindState();
}

IRenderSystem::IRenderSystem(EngineImpl& InEngine, Handle<ISystem> Hnd) :
	Engine(InEngine),
	pDevice(nullptr),
	pStore(nullptr),
	pStaging(nullptr),
	pFrameGraph(nullptr),
	Hnd(Hnd),
	VertexBuffer(),
	IndexBuffer(),
	InstanceBuffer()
{}

IRenderSystem::~IRenderSystem() {}

/**
* NOTE(Ygsm):
* Objects marked with !!! should have their configurations stored in a config file.
*/

void IRenderSystem::OnInit()
{
	g_RenderSystemHandle = Hnd;

	/* !!! */
	g_RenderSystemAllocator.Initialize(KILOBYTES(32));
	
	/* !!! */
	DrawManager::_DrawCommandAllocator.Initialize(sizeof(DrawCommand) * DrawManager::MaxDrawablesCount);
	/* !!! */
	DrawManager::_TransformAllocator.Initialize(sizeof(math::mat4) * DrawManager::MaxDrawablesCount);

	/* !!! */
	BindableManager::_BindableAllocator.Initialize(KILOBYTES(16));

	/* !!! */
	DescriptorUpdate::_Buffers.Reserve(16);
	/* !!! */
	DescriptorUpdate::_Images.Reserve(16);

	pStore = IAllocator::New<IDeviceStore>(g_RenderSystemAllocator, g_RenderSystemAllocator);

	pDevice = IAllocator::New<IRenderDevice>(g_RenderSystemAllocator);
	pDevice->Initialize(this->Engine);

	pStaging->Initialize(MEGABYTES(256));
	pFrameGraph->Initialize(Engine.GetWindowInformation().Extent);

	/* !!! */
	// Vertex buffer ...
	BufferAllocateInfo allocInfo = {};
	allocInfo.Locality = Buffer_Locality_Gpu;
	allocInfo.Size = MEGABYTES(256);
	allocInfo.Type.Assign((1 << Buffer_Type_Vertex) || (1 << Buffer_Type_Transfer_Dst));

	Handle<SMemoryBuffer> hnd = AllocateNewBuffer(allocInfo);
	VertexBuffer = DefaultBuffer(hnd, pStore->GetBuffer(hnd));
	BuildBuffer(hnd);

	// Index buffer ...
	allocInfo.Type.Assign((1 << Buffer_Type_Index) || (1 << Buffer_Type_Transfer_Dst));
	hnd = AllocateNewBuffer(allocInfo);
	IndexBuffer = DefaultBuffer(hnd, pStore->GetBuffer(hnd));
	BuildBuffer(hnd);


	// Instance buffer ...
	allocInfo.Type.Assign((1 << Buffer_Type_Vertex) || (1 << Buffer_Type_Transfer_Dst));
	allocInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
	allocInfo.Size = MEGABYTES(64);
	hnd = AllocateNewBuffer(allocInfo);
	InstanceBuffer = DefaultBuffer(hnd, pStore->GetBuffer(hnd));
	BuildBuffer(hnd);
}

void IRenderSystem::OnUpdate()
{
	// NOTE(Ygsm):
	// Let the application call the window resizing function for the renderer.

	UpdateDescriptorSetInQueue();
	PrepareDrawCommands();

	pDevice->BeginFrame();

	if (!pDevice->RenderThisFrame())
	{
		Clear();
		return;
	}

	BindBuffers();
	
	for (DrawCommand& drawCommand : DrawCommands)
	{
		BeginRenderPass(drawCommand.pRenderPass);
		BindBindablesForRenderpass(drawCommand.pRenderPass->Hnd);
		RecordDrawCommand(drawCommand);
		EndRenderPass(drawCommand.pRenderPass);
	}

	pDevice->EndFrame();
	Clear();
}

void IRenderSystem::OnTerminate()
{
	// Destroy default buffers ...
	DestroyBuffer(VertexBuffer.Key);
	DestroyBuffer(IndexBuffer.Key);
	DestroyBuffer(InstanceBuffer.Key);

	g_RenderSystemAllocator.Terminate();
	pDevice->Terminate();
}

Handle<SDescriptorPool> IRenderSystem::CreateDescriptorPool()
{
	size_t id = g_Uid();
	SDescriptorPool* pPool = pStore->NewDescriptorPool(id);
	if (!pPool) { return INVALID_HANDLE; }
	return id;
}

bool IRenderSystem::DescriptorPoolAddSizeType(Handle<SDescriptorPool> Hnd, SDescriptorPool::Size Type)
{
	SDescriptorPool* pPool = pStore->GetDescriptorPool(Hnd);
	if (pPool) 
	{ 
		VKT_ASSERT(false && "Pool with handle does not exist.");
		return false; 
	}
	VKT_ASSERT((pPool->Sizes.Length() != MAX_DESCRIPTOR_POOL_TYPE_SIZE) && "Maximum pool type size reached.");
	pPool->Sizes.Push(Type);
	return true;
}

bool IRenderSystem::BuildDescriptorPool(Handle<SDescriptorPool> Hnd)
{
	using PoolSizeContainer = StaticArray<VkDescriptorPoolSize, MAX_DESCRIPTOR_POOL_TYPE_SIZE>;

	uint32 maxSets = 0;
	SDescriptorPool* pPool = pStore->GetDescriptorPool(Hnd);
	if (!pPool)
	{
		VKT_ASSERT(false && "Pool with handle does not exist.");
		return false;
	}

	PoolSizeContainer poolSizes;

	for (SDescriptorPool::Size& size : pPool->Sizes)
	{
		poolSizes.Push({ 
			pDevice->GetDescriptorType(size.Type), 
			size.Count 
		});
		maxSets += size.Count;
	}

	VkDescriptorPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.maxSets = maxSets;
	info.pPoolSizes = poolSizes.First();
	info.poolSizeCount = static_cast<uint32>(poolSizes.Length());

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkCreateDescriptorPool(pDevice->GetDevice(), &info, nullptr, &pPool->Hnd[i]);
	}

	return true;
}

bool IRenderSystem::DestroyDescriptorPool(Handle<SDescriptorPool> Hnd)
{
	SDescriptorPool* pPool = pStore->GetDescriptorPool(Hnd);
	if (!pPool)
	{
		VKT_ASSERT(false && "Pool with handle does not exist.");
		return false;
	}
	vkDeviceWaitIdle(pDevice->GetDevice());
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyDescriptorPool(pDevice->GetDevice(), pPool->Hnd[i], nullptr);
	}
	pStore->DeleteDescriptorPool(Hnd);
	return true;
}

Handle<SDescriptorSetLayout> IRenderSystem::CreateDescriptorSetLayout()
{
	size_t id = g_Uid();
	SDescriptorSetLayout* pSetLayout = pStore->NewDescriptorSetLayout(id);
	if (!pSetLayout) { return INVALID_HANDLE; }
	return id;
}

bool IRenderSystem::DescriptorSetLayoutAddBinding(const DescriptorSetLayoutBindingInfo& BindInfo)
{
	SDescriptorSetLayout* pSetLayout = pStore->GetDescriptorSetLayout(BindInfo.LayoutHnd);
	if (!pSetLayout)
	{
		VKT_ASSERT(false && "Layout with handle does not exist.");
		return false;
	}

	SDescriptorSetLayout::Binding binding = {};
	binding.BindingSlot = BindInfo.BindingSlot;
	binding.DescriptorCount = BindInfo.DescriptorCount;
	binding.Type = BindInfo.Type;
	binding.ShaderStages = BindInfo.ShaderStages;

	if (BindInfo.BufferHnd != INVALID_HANDLE)
	{
		SMemoryBuffer* pBuffer = pStore->GetBuffer(BindInfo.BufferHnd);
		if (!pBuffer)
		{
			VKT_ASSERT(false && "Buffer with handle does not exist.");
			return false;
		}
		binding.pBuffer = pBuffer;
	}

	pSetLayout->Bindings.Push(Move(binding));

	return true;
}

bool IRenderSystem::BuildDescriptorSetLayout(Handle<SDescriptorSetLayout> Hnd)
{
	using BindingsContainer = StaticArray<VkDescriptorSetLayoutBinding, MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS>;

	SDescriptorSetLayout* pSetLayout = pStore->GetDescriptorSetLayout(Hnd);
	if (!pSetLayout)
	{
		VKT_ASSERT(false && "Layout with handle does not exist.");
		return false;
	}

	BindingsContainer layoutBindings;

	for (const SDescriptorSetLayout::Binding& b : pSetLayout->Bindings)
	{
		VkDescriptorSetLayoutBinding binding = {};
		binding.binding = b.BindingSlot;
		binding.descriptorCount = b.DescriptorCount;
		binding.descriptorType = pDevice->GetDescriptorType(b.Type);

		if (b.ShaderStages.Has(Shader_Type_Vertex))
		{
			binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
		}

		if (b.ShaderStages.Has(Shader_Type_Fragment))
		{
			binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		layoutBindings.Push(binding);
	}

	VkDescriptorSetLayoutCreateInfo create = {};
	create.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	create.bindingCount = static_cast<uint32>(layoutBindings.Length());
	create.pBindings = layoutBindings.First();

	if (vkCreateDescriptorSetLayout(pDevice->GetDevice(), &create, nullptr, &pSetLayout->Hnd) != VK_SUCCESS)
	{
		VKT_ASSERT(false && "Failed to create descriptor set layout on GPU");
	}

	return true;
}

bool IRenderSystem::DestroyDescriptorSetLayout(Handle<SDescriptorSetLayout> Hnd)
{
	SDescriptorSetLayout* pSetLayout = pStore->GetDescriptorSetLayout(Hnd);
	if (!pSetLayout)
	{
		VKT_ASSERT(false && "Layout with handle does not exist.");
		return false;
	}
	vkDeviceWaitIdle(pDevice->GetDevice());
	vkDestroyDescriptorSetLayout(pDevice->GetDevice(), pSetLayout->Hnd, nullptr);
	pStore->DeleteDescriptorSetLayout(Hnd);

	return true;
}

Handle<SDescriptorSet> IRenderSystem::CreateDescriptorSet(const DescriptorSetAllocateInfo& AllocInfo)
{
	SDescriptorPool* pPool = pStore->GetDescriptorPool(AllocInfo.PoolHnd);
	SDescriptorSetLayout* pLayout = pStore->GetDescriptorSetLayout(AllocInfo.LayoutHnd);
	if (!pPool || !pLayout) { return INVALID_HANDLE; }

	size_t id = g_Uid();
	SDescriptorSet* pSet = pStore->NewDescriptorSet(id);
	if (!pSet) { return INVALID_HANDLE; }

	pSet->pPool = pPool;
	pSet->pLayout = pLayout;
	pSet->Slot = AllocInfo.Slot;

	return id;
}

bool IRenderSystem::DescriptorSetUpdateBuffer(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, Handle<SMemoryBuffer> BufferHnd)
{
	SDescriptorSet* pSet = pStore->GetDescriptorSet(Hnd);
	SMemoryBuffer* pBuffer = pStore->GetBuffer(BufferHnd);

	if (!pSet || pBuffer) { return false; }

	SDescriptorSetLayout::Binding* pBinding = nullptr;
	for (SDescriptorSetLayout::Binding& b : pSet->pLayout->Bindings)
	{
		if (b.BindingSlot != BindingSlot) { continue; }
		pBinding = &b;
		break;
	}

	if (!pBinding) { return false; }

	if (pBinding->Type == Descriptor_Type_Sampler ||
		pBinding->Type == Descriptor_Type_Input_Attachment ||
		pBinding->Type == Descriptor_Type_Sampled_Image)
	{
		return false;
	}

	DescriptorUpdate::_Buffers[DescriptorUpdate::Key(pSet, pBinding, DescriptorUpdate::CalcKey(Hnd, BindingSlot))].Push(pBuffer);

	return true;
}

bool IRenderSystem::DescriptorSetUpdateTexture(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, Handle<SImage> ImageHnd)
{
	SDescriptorSet* pSet = pStore->GetDescriptorSet(Hnd);
	SImage* pImage = pStore->GetImage(ImageHnd);

	if (!pSet || pImage) { return false; }

	SDescriptorSetLayout::Binding* pBinding = nullptr;
	for (SDescriptorSetLayout::Binding& b : pSet->pLayout->Bindings)
	{
		if (b.BindingSlot != BindingSlot) { continue; }
		pBinding = &b;
		break;
	}

	if (!pBinding) { return false; }

	if (pBinding->Type != Descriptor_Type_Sampler ||
		pBinding->Type != Descriptor_Type_Input_Attachment ||
		pBinding->Type != Descriptor_Type_Sampled_Image)
	{
		return false;
	}

	DescriptorUpdate::_Images[DescriptorUpdate::Key(pSet, pBinding, DescriptorUpdate::CalcKey(Hnd, BindingSlot))].Push(pImage);

	return true;
}

bool IRenderSystem::BuildDescriptorSet(Handle<SDescriptorSet> Hnd)
{
	SDescriptorSet* pSet = pStore->GetDescriptorSet(Hnd);
	if (!pSet)
	{
		VKT_ASSERT(false && "Set with handle does not exist.");
		return false;
	}

	if (pSet->pPool->Hnd[0] == VK_NULL_HANDLE || 
		pSet->pLayout->Hnd == VK_NULL_HANDLE)
	{
		VKT_ASSERT(false && "Descriptor pool or set layout has not been constructed.");
		return false;
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &pSet->pLayout->Hnd;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		allocInfo.descriptorPool = pSet->pPool->Hnd[i];
		vkAllocateDescriptorSets(pDevice->GetDevice(), &allocInfo, &pSet->Hnd[i]);
	}

	return true;
}

bool IRenderSystem::DescriptorSetFlushBindingOffset(Handle<SDescriptorSet> Hnd)
{
	SDescriptorSet* pSet = pStore->GetDescriptorSet(Hnd);
	if (!pSet)
	{
		VKT_ASSERT(false && "Set with handle does not exist.");
		return false;
	}

	for (SDescriptorSetLayout::Binding& b : pSet->pLayout->Bindings)
	{
		b.Offset[pDevice->GetCurrentFrameIndex()] = 0;
	}

	return true;
}

bool IRenderSystem::DestroyDescriptorSet(Handle<SDescriptorSet> Hnd)
{
	SDescriptorSet* pSet = pStore->GetDescriptorSet(Hnd);
	if (!pSet)
	{
		VKT_ASSERT(false && "Set with handle does not exist.");
		return false;
	}

	vkDeviceWaitIdle(pDevice->GetDevice());
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkFreeDescriptorSets(pDevice->GetDevice(), pSet->pPool->Hnd[i], 1, &pSet->Hnd[i]);
	}
	pStore->DeleteDescriptorSet(Hnd);

	return true;
}

void IRenderSystem::UpdateDescriptorSetInQueue()
{
	StaticArray<VkDescriptorBufferInfo, MAX_DESCRIPTOR_BINDING_UPDATES> buffers;
	StaticArray<VkDescriptorImageInfo, MAX_DESCRIPTOR_BINDING_UPDATES> images;

	// Update buffers ....
	for (auto& [key, container] : DescriptorUpdate::_Buffers)
	{
		SDescriptorSet* pSet = key.pSet;
		SDescriptorSetLayout::Binding* pBinding = key.pBinding;

		for (SMemoryBuffer* buffer : container)
		{
			buffers.Push({
				buffer->Hnd,
				buffer->Offset,
				buffer->Size
			});
		}

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = pBinding->BindingSlot;
		write.descriptorCount = static_cast<uint32>(buffers.Length());
		write.descriptorType = pDevice->GetDescriptorType(pBinding->Type);
		write.pBufferInfo = buffers.First();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			write.dstSet = pSet->Hnd[i];
			vkUpdateDescriptorSets(pDevice->GetDevice(), 1, &write, 0, nullptr);
		}
	}

	// Update images ....
	for (auto& [key, container] : DescriptorUpdate::_Images)
	{
		SDescriptorSet* pSet = key.pSet;
		SDescriptorSetLayout::Binding* pBinding = key.pBinding;

		for (SImage* image : container)
		{
			images.Push({
				image->pSampler->Hnd,
				image->ImgViewHnd,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			});
		}

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = pBinding->BindingSlot;
		write.descriptorCount = static_cast<uint32>(images.Length());
		write.descriptorType = pDevice->GetDescriptorType(pBinding->Type);
		write.pImageInfo = images.First();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			write.dstSet = pSet->Hnd[i];
			vkUpdateDescriptorSets(pDevice->GetDevice(), 1, &write, 0, nullptr);
		}
	}
}

Handle<SMemoryBuffer> IRenderSystem::AllocateNewBuffer(const BufferAllocateInfo& AllocInfo)
{
	size_t id = g_Uid();
	SMemoryBuffer* pBuffer = pStore->NewBuffer(id);
	if (!pBuffer) { return INVALID_HANDLE; }

	pBuffer->Locality = AllocInfo.Locality;
	pBuffer->Size = AllocInfo.Size;
	pBuffer->Type = AllocInfo.Type;
	pBuffer->pData = nullptr;

	return id;
}

bool IRenderSystem::BuildBuffer(Handle<SMemoryBuffer> Hnd)
{
	SMemoryBuffer* pBuffer = pStore->GetBuffer(Hnd);
	if (!pBuffer) { return false; }

	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = pBuffer->Size;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	for (uint32 i = 0; i < Buffer_Type_Max; i++)
	{
		if (!pBuffer->Type.Has(i)) { continue; }
		info.usage |= pDevice->GetBufferUsage(i);
	}

	VmaAllocationCreateInfo alloc = {};
	alloc.usage = pDevice->GetMemoryUsage(pBuffer->Locality);

	if (vmaCreateBuffer(
		pDevice->GetAllocator(),
		&info,
		&alloc,
		&pBuffer->Hnd,
		&pBuffer->Allocation,
		nullptr
	))
	{
		return false;
	}

	if (pBuffer->Locality != Buffer_Locality_Gpu)
	{
		vmaMapMemory(pDevice->GetAllocator(), pBuffer->Allocation, reinterpret_cast<void**>(pBuffer->pData));
		vmaUnmapMemory(pDevice->GetAllocator(), pBuffer->Allocation);
	}

	return true;
}

bool IRenderSystem::DestroyBuffer(Handle<SMemoryBuffer> Hnd)
{
	SMemoryBuffer* pBuffer = pStore->GetBuffer(Hnd);
	if (!pBuffer) { return false; }

	vkDeviceWaitIdle(pDevice->GetDevice());
	vmaDestroyBuffer(pDevice->GetAllocator(), pBuffer->Hnd, pBuffer->Allocation);
	pStore->DeleteBuffer(Hnd);

	return true;
}

Handle<SImage> IRenderSystem::CreateImage(const ImageCreateInfo& CreateInfo)
{
	size_t id = (CreateInfo.Identifier == -1) ?  g_Uid() : CreateInfo.Identifier;
	SImage* pImg = pStore->NewImage(id);
	if (!pImg) { return INVALID_HANDLE; }

	pImg->Width = CreateInfo.Width;
	pImg->Height = CreateInfo.Height;
	pImg->Channels = CreateInfo.Channels;
	pImg->Type = CreateInfo.Type;

	return id;
}

bool IRenderSystem::BuildImage(Handle<SImage> Hnd)
{
	SImage* pImg = pStore->GetImage(Hnd);
	if (!pImg) { return false; }

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	switch (pImg->Channels)
	{
	case 1:
		format = VK_FORMAT_R8_SRGB;
		break;
	case 2:
		format = VK_FORMAT_R8G8_SRGB;
		break;
	default:
		break;
	}

	if (pImg->Usage.Has(Image_Usage_Depth_Stencil_Attachment))
	{
		format = VK_FORMAT_D24_UNORM_S8_UINT;
	}

	VkImageUsageFlags usage = 0;
	for (uint32 i = 0; i < Image_Usage_Max; i++)
	{
		if (!pImg->Usage.Has(i)) { continue; }
		usage |= pDevice->GetImageUsage(i);
	}

	VkImageCreateInfo img = {};
	img.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	img.imageType = pDevice->GetImageType(pImg->Type);
	img.format = format;
	img.samples = VK_SAMPLE_COUNT_1_BIT;
	img.extent = { pImg->Width, pImg->Height, 1 };
	img.tiling = VK_IMAGE_TILING_OPTIMAL;
	img.mipLevels = 1;
	img.usage = usage;
	img.arrayLayers = 1;
	img.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	img.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImageViewCreateInfo imgView = {};
	imgView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imgView.format = format;
	imgView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgView.subresourceRange.baseMipLevel = 0;
	imgView.subresourceRange.levelCount = 1;
	imgView.subresourceRange.baseArrayLayer = 0;
	imgView.subresourceRange.layerCount = 1;
	imgView.viewType = pDevice->GetImageViewType(pImg->Type);

	if (pImg->Usage.Has(Image_Usage_Depth_Stencil_Attachment))
	{
		imgView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	VmaAllocationCreateInfo alloc = {};
	alloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vkCreateImageView(pDevice->GetDevice(), &imgView, nullptr, &pImg->ImgViewHnd);
	vmaCreateImage(pDevice->GetAllocator(), &img, &alloc, &pImg->ImgHnd, &pImg->Allocation, nullptr);

	return true;
}

bool IRenderSystem::DestroyImage(Handle<SImage> Hnd)
{
	SImage* pImg = pStore->GetImage(Hnd);
	if (!pImg) { return false; }

	vkDeviceWaitIdle(pDevice->GetDevice());
	vkDestroyImageView(pDevice->GetDevice(), pImg->ImgViewHnd, nullptr);
	vmaDestroyImage(pDevice->GetAllocator(), pImg->ImgHnd, pImg->Allocation);

	pStore->DeleteImage(Hnd);

	return true;
}

IStagingManager& IRenderSystem::GetStagingManager() const
{
	return *pStaging;
}

IFrameGraph& IRenderSystem::GetFrameGraph() const
{
	return *pFrameGraph;
}

Handle<SShader> IRenderSystem::CreateShader(const ShaderCreateInfo& CreateInfo)
{
	size_t id = g_Uid();
	Ref<SShader> pShader = pStore->NewShader(id);

	if (!pShader) { return INVALID_HANDLE; }
	pShader->Type = CreateInfo.Type;

	if (!pDevice->ToSpirV(CreateInfo.pCode, pShader->SpirV, pShader->Type)) 
	{ 
		return INVALID_HANDLE; 
	}

	PreprocessShader(pShader);

	return id;
}

bool IRenderSystem::BuildShader(Handle<SShader> Hnd)
{
	Ref<SShader> pShader = pStore->GetShader(Hnd);
	if (!pShader) { return false; }
	if (!pShader->SpirV.Length()) { return false; } // Impossible but just to be safe.

	VkShaderModuleCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.codeSize = pShader->SpirV.Length() * sizeof(uint32);
	info.pCode = pShader->SpirV.First();

	// NOTE(Ygsm):
	// Should do this to all build functions ...
	if (vkCreateShaderModule(pDevice->GetDevice(), &info, nullptr, &pShader->Hnd) != VK_SUCCESS)
	{
		return false;
	}

	return true;
}

bool IRenderSystem::DestroyShader(Handle<SShader> Hnd)
{
	Ref<SShader> pShader = pStore->GetShader(Hnd);
	if (!pShader) { return false; }

	vkDeviceWaitIdle(pDevice->GetDevice());
	vkDestroyShaderModule(pDevice->GetDevice(), pShader->Hnd, nullptr);

	pStore->DeleteShader(Hnd);

	return true;
}

Handle<SPipeline> IRenderSystem::CreatePipeline(const PipelineCreateInfo& CreateInfo)
{
	size_t id = g_Uid();
	Ref<SPipeline> pPipeline = pStore->NewPipeline(id);
	if (!pPipeline) { return INVALID_HANDLE; }

	Ref<SShader> pVertexShader = pStore->GetShader(CreateInfo.VertexShaderHnd);
	Ref<SShader> pFragmentShader = pStore->GetShader(CreateInfo.FragmentShaderHnd);
	Ref<SShader> pGeometryShader = pStore->GetShader(CreateInfo.GeometryShaderHnd);
	Ref<SShader> pComputeShader = pStore->GetShader(CreateInfo.ComputeShaderHnd);

	// NOTE(Ygsm):
	// Need to include valid pipeline check i.e if shaders supplied are enough.

	if (pVertexShader	) pPipeline->pVertexShader	 = pVertexShader;
	if (pFragmentShader	) pPipeline->pFragmentShader = pFragmentShader;
	if (pGeometryShader	) pPipeline->pGeometryShader = pGeometryShader;
	if (pComputeShader	) pPipeline->pComputeShader  = pComputeShader;

	pPipeline->ColorOutputCount = CreateInfo.ColorOutputCount;
	pPipeline->Samples = CreateInfo.Samples;
	pPipeline->Topology = CreateInfo.Topology;
	pPipeline->CullMode = CreateInfo.CullMode;
	pPipeline->PolygonalMode = CreateInfo.PolygonalMode;
	pPipeline->FrontFace = CreateInfo.FrontFace;
	pPipeline->HasDepthStencil = CreateInfo.HasDepthStencil;
	pPipeline->Viewport = CreateInfo.Viewport;
	pPipeline->Scissor = CreateInfo.Scissor;
	pPipeline->BindPoint = CreateInfo.BindPoint;

	return id;
}

bool IRenderSystem::PipelineAddVertexInputBinding(Handle<SPipeline> Hnd, SPipeline::VertexInputBinding Input)
{
	Ref<SPipeline> pPipeline = pStore->GetPipeline(Hnd);
	if (!pPipeline) { return false; }

	pPipeline->VertexBindings.Push(Input);

	return true;
}

bool IRenderSystem::PipelineAddRenderPass(Handle<SPipeline> Hnd, Handle<SRenderPass> RenderPass)
{
	Ref<SPipeline> pPipeline = pStore->GetPipeline(Hnd);
	Ref<SRenderPass> pRenderPass = pStore->GetRenderPass(RenderPass);

	if (!pPipeline || !pRenderPass) { return false; }

	pPipeline->pRenderPass = pRenderPass;

	return true;
}

bool IRenderSystem::PipelineAddDescriptorSetLayout(Handle<SPipeline> Hnd, Handle<SDescriptorSetLayout> Layout)
{
	Ref<SPipeline> pPipeline = pStore->GetPipeline(Hnd);
	Ref<SDescriptorSetLayout> pLayout = pStore->GetDescriptorSetLayout(Layout);

	if (!pPipeline || !pLayout)
	{
		return false;
	}

	if (pPipeline->Layouts.Length() == MAX_PIPELINE_DESCRIPTOR_LAYOUT)
	{
		return false;
	}

	pLayout->pPipeline = pPipeline;
	pPipeline->Layouts.Push(pLayout);

	return true;
}

bool IRenderSystem::BuildGraphicsPipeline(Handle<SPipeline> Hnd)
{
	Ref<SPipeline> pPipeline = pStore->GetPipeline(Hnd);
	if (!pPipeline) { return false; }
	if (!pPipeline->pVertexShader && !pPipeline->pFragmentShader) { return false; }

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};

	Array<VkPipelineShaderStageCreateInfo> stages;
	Array<VkVertexInputBindingDescription> bindingDescriptions;
	Array<VkVertexInputAttributeDescription> attributeDescriptions;

	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.pName = "main";

	if (pPipeline->pVertexShader)
	{

		shaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStage.module = pPipeline->pVertexShader->Hnd;
		stages.Push(shaderStage);
	}

	if (pPipeline->pFragmentShader)
	{
		shaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStage.module = pPipeline->pFragmentShader->Hnd;
		stages.Push(shaderStage);
	}

	for (SPipeline::VertexInputBinding& bind : pPipeline->VertexBindings)
	{
		VkVertexInputBindingDescription description = {};
		description.binding = bind.Binding;
		description.stride = bind.Stride;
		description.inputRate = pDevice->GetVertexInputRate(bind.Type);
		bindingDescriptions.Push(Move(description));

		if (!pPipeline->pVertexShader) { continue; }

		VkFormat format;
		uint32 stride = 0, binding = 0;

		for (uint32 i = bind.From; i <= bind.To; i++)
		{
			ShaderAttrib& attrib = pPipeline->pVertexShader->Attributes[i];
			switch (attrib.Format)
			{
			case Shader_Attrib_Type_Vec2:
				format = VK_FORMAT_R32G32_SFLOAT;
				break;
			case Shader_Attrib_Type_Vec3:
				format = VK_FORMAT_R32G32B32_SFLOAT;
				break;
			case Shader_Attrib_Type_Vec4:
			default:
				format = VK_FORMAT_R32G32B32A32_SFLOAT;
				break;
			}

			attrib.Offset = stride;
			stride += CalcStrideForFormat(attrib.Format);
			binding = bind.Binding;

			VkVertexInputAttributeDescription attribDescription = {};
			attribDescription.location = attrib.Location;
			attribDescription.binding = binding;
			attribDescription.format = format;
			attribDescription.offset = attrib.Offset;
			attributeDescriptions.Push(Move(attribDescription));
		}
	}

	VkPipelineVertexInputStateCreateInfo vertexState = {};
	vertexState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexState.pVertexBindingDescriptions = bindingDescriptions.First();
	vertexState.pVertexAttributeDescriptions = attributeDescriptions.First();
	vertexState.vertexBindingDescriptionCount = static_cast<uint32>(bindingDescriptions.Length());
	vertexState.vertexAttributeDescriptionCount = static_cast<uint32>(pPipeline->pVertexShader->Attributes.Length());

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = pDevice->GetPrimitiveTopology(pPipeline->Topology);
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = pPipeline->Viewport.Width;
	viewport.height = pPipeline->Viewport.Height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { pPipeline->Scissor.Offset.x, pPipeline->Scissor.Offset.y };
	scissor.extent = { pPipeline->Scissor.Extent.Width, pPipeline->Scissor.Extent.Height };

	VkPipelineViewportStateCreateInfo viewportCreate = {};
	viewportCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCreate.pViewports = &viewport;
	viewportCreate.viewportCount = 1;
	viewportCreate.pScissors = &scissor;
	viewportCreate.scissorCount = 1;

	VkDynamicState dynamicStates[1] = { VK_DYNAMIC_STATE_VIEWPORT };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 1;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineRasterizationStateCreateInfo rasterState = {};
	rasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterState.polygonMode = pDevice->GetPolygonMode(pPipeline->PolygonalMode);
	rasterState.cullMode = pDevice->GetCullMode(pPipeline->CullMode);
	rasterState.frontFace = pDevice->GetFrontFaceMode(pPipeline->FrontFace);
	rasterState.lineWidth = 1.0f;
	//rasterStateCreateInfo.depthClampEnable = VK_FALSE; // Will be true for shadow rendering for some reason I do not know yet...

	VkPipelineMultisampleStateCreateInfo multisampleCreate = {};
	multisampleCreate.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreate.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleCreate.sampleShadingEnable = VK_FALSE;
	//multisampleCreate.minSampleShading		= 1.0f;
	multisampleCreate.alphaToCoverageEnable = VK_FALSE;
	multisampleCreate.alphaToOneEnable = VK_FALSE;

	Array<VkPipelineColorBlendAttachmentState> colorBlendStates;
	VkPipelineColorBlendAttachmentState blendState = {};
	blendState.blendEnable = VK_TRUE;
	blendState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendState.colorBlendOp = VK_BLEND_OP_ADD;
	blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendState.alphaBlendOp = VK_BLEND_OP_ADD;
	blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	for (uint32 i = 0; i < pPipeline->ColorOutputCount; i++)
	{
		colorBlendStates.Push(blendState);
	}

	VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
	colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendInfo.attachmentCount = static_cast<uint32>(colorBlendStates.Length());
	colorBlendInfo.pAttachments = colorBlendStates.First();
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;

	StaticArray<VkDescriptorSetLayout, MAX_PIPELINE_DESCRIPTOR_LAYOUT> descriptorLayouts;
	for (Ref<SDescriptorSetLayout> pSetLayout : pPipeline->Layouts)
	{
		descriptorLayouts.Push(pSetLayout->Hnd);
	}

	VkPushConstantRange pushConstant = {};
	pushConstant.offset = 0;
	pushConstant.size = pDevice->GetPhysicalDeviceProperties().limits.maxPushConstantsSize;
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.pPushConstantRanges = &pushConstant;
	layoutCreateInfo.pushConstantRangeCount = 1;

	if (descriptorLayouts.Length())
	{
		layoutCreateInfo.pSetLayouts = descriptorLayouts.First();
		layoutCreateInfo.setLayoutCount = static_cast<uint32>(descriptorLayouts.Length());
	}

	if (vkCreatePipelineLayout(
		pDevice->GetDevice(),
		&layoutCreateInfo,
		nullptr,
		&pPipeline->LayoutHnd) != VK_SUCCESS)
	{
		VKT_ASSERT(false && "Could not create pipeline layout");
		return false;
	}

	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilCreateInfo.minDepthBounds = 0.0f;
	depthStencilCreateInfo.maxDepthBounds = 1.0f;
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;

	//if (pPipeline->HasDepthStencil)
	//{
	//	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	//	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;

	//	//
	//	// TODO(Ygsm):
	//	// Need to revise depth stencil configuration in the pipeline.
	//	//

	//	//depthStencilCreateInfo.stencilTestEnable = VK_TRUE;
	//	//depthStencilCreateInfo.front = {};
	//	//depthStencilCreateInfo.back = {};
	//}

	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = static_cast<uint32>(stages.Length());
	pipelineCreateInfo.pStages = stages.First();
	pipelineCreateInfo.pVertexInputState = &vertexState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &viewportCreate;
	pipelineCreateInfo.pRasterizationState = &rasterState;
	pipelineCreateInfo.pMultisampleState = &multisampleCreate;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.pColorBlendState = &colorBlendInfo;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	pipelineCreateInfo.layout = pPipeline->LayoutHnd;
	pipelineCreateInfo.renderPass = pPipeline->pRenderPass->RenderPassHnd;
	pipelineCreateInfo.subpass = 0;							// TODO(Ygsm): Study more about this!
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// TODO(Ygsm): Study more about this!
	pipelineCreateInfo.basePipelineIndex = -1;				// TODO(Ygsm): Study more about this!

	if (pPipeline->HasDepthStencil)
	{
		depthStencilCreateInfo.depthTestEnable = VK_TRUE;
		depthStencilCreateInfo.depthWriteEnable = VK_TRUE;

		pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	}

	if (vkCreateGraphicsPipelines(
		pDevice->GetDevice(), 
		VK_NULL_HANDLE, 
		1, 
		&pipelineCreateInfo, 
		nullptr, 
		&pPipeline->Hnd) != VK_SUCCESS)
	{
		VKT_ASSERT("Could not create graphics pipeline!" && false);
		return false;
	}

	return true;
}

bool IRenderSystem::DestroyPipeline(Handle<SPipeline> Hnd)
{
	Ref<SPipeline> pPipeline = pStore->GetPipeline(Hnd);
	if (!pPipeline) { return false; }

	if (pPipeline->Hnd == VK_NULL_HANDLE) { return false; }

	vkDeviceWaitIdle(pDevice->GetDevice());
	vkDestroyPipelineLayout(pDevice->GetDevice(), pPipeline->LayoutHnd, nullptr);
	vkDestroyPipeline(pDevice->GetDevice(), pPipeline->Hnd, nullptr);

	pStore->DeletePipeline(Hnd);

	return true;
}

bool IRenderSystem::BindDescriptorSet(Handle<SDescriptorSet> Hnd, Handle<SRenderPass> RenderpassHnd)
{
	Ref<SDescriptorSet> pSet = pStore->GetDescriptorSet(Hnd);
	Ref<SRenderPass> pRenderpass = pStore->GetRenderPass(RenderpassHnd);
	VKT_ASSERT(pSet && pRenderpass && "Invalid handle(s) supplied");

	if (!pSet || pRenderpass) 
	{ 
		return false; 
	}

	ForwardNode<SBindable>* bindableNode = IAllocator::New<ForwardNode<SBindable>>(BindableManager::_BindableAllocator);
	BindableManager::BindableRange& range = BindableManager::_Bindables[RenderpassHnd];

	bindableNode->Data.Type = EBindableType::Bindable_Type_Descriptor_Set;
	bindableNode->Data.pSet = pSet;

	if (!range.pBegin && !range.pEnd) 
	{
		range.pBegin = bindableNode;
		range.pEnd = bindableNode;
	}

	range.pEnd->Next = bindableNode;
	range.pEnd = bindableNode;

	return true;
}

bool IRenderSystem::BindPipeline(Handle<SPipeline> Hnd, Handle<SRenderPass> RenderpassHnd)
{
	Ref<SPipeline> pPipeline = pStore->GetPipeline(Hnd);
	Ref<SRenderPass> pRenderpass = pStore->GetRenderPass(RenderpassHnd);
	VKT_ASSERT(pPipeline && pRenderpass && "Invalid handle(s) supplied");

	if (!pPipeline || pRenderpass)
	{
		return false;
	}

	ForwardNode<SBindable>* bindableNode = IAllocator::New<ForwardNode<SBindable>>(BindableManager::_BindableAllocator);
	BindableManager::BindableRange& range = BindableManager::_Bindables[RenderpassHnd];

	bindableNode->Data.Type = EBindableType::Bindable_Type_Pipeline;
	bindableNode->Data.pPipeline = pPipeline;

	if (!range.pBegin && !range.pEnd)
	{
		range.pBegin = bindableNode;
		range.pEnd = bindableNode;
	}

	range.pEnd->Next = bindableNode;
	range.pEnd = bindableNode;

	return true;
}

bool IRenderSystem::Draw(const DrawInfo& Info)
{
	Ref<SRenderPass> pRenderPass = pStore->GetRenderPass(Info.Renderpass);
	VKT_ASSERT(pRenderPass && "RenderPass Handle is invalid!");
	if (!pRenderPass) { return false; }

	VKT_ASSERT(!Info.DrawableCount && "Drawable count can not be zero!");
	if (!Info.DrawableCount) { return false; }

	VKT_ASSERT((DrawManager::_NumDrawables >= DrawManager::MaxDrawablesCount) && "Maximum ammount of drawables reached!");
	if (DrawManager::_NumDrawables >= DrawManager::MaxDrawablesCount) { return false; }

	DrawManager::_NumDrawables++;

	DrawManager::EntryContainer* entries = &DrawManager::_InstancedDraws[Info.Renderpass];
	if (!Info.Instanced)
	{
		entries = &DrawManager::_NonInstancedDraws[Info.Renderpass];
	}

	ForwardNode<math::mat4>* transform = IAllocator::New<ForwardNode<math::mat4>>(DrawManager::_TransformAllocator);
	transform->Data = Info.Transform;

	if (Info.Instanced)
	{
		DrawManager::InstanceEntry* entry = nullptr;
		for (DrawManager::InstanceEntry& e : *entries)
		{
			if (e.Id != Info.Id) { continue; }
			entry = &e;
			break;
		}

		if (entry)
		{
			entry->TransformRange.pEnd->Next = transform;
			entry->TransformRange.pEnd = transform;

			ForwardNode<DrawCommand>* drawCmdNode = entry->DrawRange.pBegin;
			while (drawCmdNode)
			{
				drawCmdNode->Data.InstanceCount++;
				drawCmdNode = drawCmdNode->Next;
			}
			return true;
		}
	}


	DrawManager::InstanceEntry& entry = entries->Insert(DrawManager::InstanceEntry());
	entry.Id = Info.Id;

	entry.TransformRange.pBegin = transform;
	entry.TransformRange.pEnd = transform;

	for (uint32 i = 0; i < Info.DrawableCount; i++)
	{
		ForwardNode<DrawCommand>* drawCmdNode = IAllocator::New<ForwardNode<DrawCommand>>(DrawManager::_DrawCommandAllocator);
		if (!entry.DrawRange.pBegin && !entry.DrawRange.pEnd)
		{
			entry.DrawRange.pBegin = drawCmdNode;
			entry.DrawRange.pEnd = drawCmdNode;
		}

		entry.DrawRange.pEnd->Next = drawCmdNode;
		entry.DrawRange.pEnd = drawCmdNode;

		DrawCommand& cmd = drawCmdNode->Data;
		cmd.VertexOffset = Info.pVertexInformation[i].VertexOffset;
		cmd.NumVertices = Info.pVertexInformation[i].NumVertices;
		cmd.IndexOffset = Info.pIndexInformation[i].IndexOffset;
		cmd.NumIndices = Info.pIndexInformation[i].NumIndices;
		cmd.pRenderPass = pRenderPass;
		cmd.InstanceCount = 1;
	}

	return true;
}

const uint32 IRenderSystem::GetMaxDrawablesCount() const
{
	return DrawManager::MaxDrawablesCount;
}

const uint32 IRenderSystem::GetDrawableCount() const
{
	return DrawManager::_NumDrawables;
}

//bool IRenderSystem::BlitToDefault(Handle<SImage> Hnd)
//{
//	Ref<SImage> pImg = pStore->GetImage(Hnd);
//	if (!pImg) { return false; }
//
//	VkImage src = pImg->ImgHnd;
//	VkImage dst = pDevice->GetNextImageInSwapchain();
//
//	VkImageSubresourceRange range = {};
//	range.levelCount = 1;
//	range.layerCount = 1;
//	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//
//	VkImageMemoryBarrier barrier = {};
//	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
//	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//	barrier.srcQueueFamilyIndex = pDevice->GetGraphicsQueue().FamilyIndex;
//	barrier.dstQueueFamilyIndex = pDevice->GetPresentationQueue().FamilyIndex;
//	barrier.image = dst;
//	barrier.subresourceRange = range;
//
//	VkOffset3D srcOffset = { pImg->Width, pImg->Height, 1 };
//	VkOffset3D dstOffset = { pDevice->GetSwapchain().Extent.width, pDevice->GetSwapchain().Extent.height, 1 };
//
//	VkImageBlit region = {};
//	region.srcOffsets[1] = srcOffset;
//	region.dstOffsets[1] = dstOffset;
//	region.srcSubresource.mipLevel = 0;
//	region.srcSubresource.layerCount = 1;
//	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//	region.dstSubresource.mipLevel = 0;
//	region.dstSubresource.layerCount = 1;
//	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//
//	vkCmdPipelineBarrier(
//		pDevice->GetCommandBuffer(),
//		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//		VK_PIPELINE_STAGE_TRANSFER_BIT,
//		0,
//		0,
//		nullptr,
//		0,
//		nullptr,
//		1,
//		&barrier
//	);
//
//	vkCmdBlitImage(
//		pDevice->GetCommandBuffer(),
//		src,
//		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//		dst,
//		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//		1,
//		&region,
//		VK_FILTER_LINEAR
//	);
//
//	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
//	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
//	vkCmdPipelineBarrier(
//		pDevice->GetCommandBuffer(),
//		VK_PIPELINE_STAGE_TRANSFER_BIT,
//		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
//		0,
//		0,
//		nullptr,
//		0,
//		nullptr,
//		1,
//		&barrier
//	);
//
//	return true;
//}

void IRenderSystem::FlushVertexBuffer()
{
	auto [hnd, pBuffer] = VertexBuffer;
	pBuffer->Offset = 0;
	IMemory::Memzero(pBuffer->pData, pBuffer->Size);
}

void IRenderSystem::FlushIndexBuffer()
{
	auto [hnd, pBuffer] = IndexBuffer;
	pBuffer->Offset = 0;
	IMemory::Memzero(pBuffer->pData, pBuffer->Size);
}

void IRenderSystem::FlushInstanceBuffer()
{
	auto [hnd, pBuffer] = InstanceBuffer;
	pBuffer->Offset = 0;
	IMemory::Memzero(pBuffer->pData, pBuffer->Size);
}

Handle<ISystem> IRenderSystem::GetSystemHandle()
{
	return g_RenderSystemHandle;
}

namespace ao
{
	IRenderSystem& FetchRenderSystem()
	{
		EngineImpl& engine = FetchEngineCtx();
		Handle<ISystem> handle = IRenderSystem::GetSystemHandle();
		SystemInterface* system = engine.GetRegisteredSystem(System_Engine_Type, handle);
		return *(reinterpret_cast<IRenderSystem*>(system));
	}
}

IRenderSystem::DescriptorUpdate::Key::Key() :
	pSet{}, pBinding{}, _Key(0)
{}

IRenderSystem::DescriptorUpdate::Key::Key(	SDescriptorSet* InSet,
											SDescriptorSetLayout::Binding* InBinding,
											size_t InKey ) :
	pSet(InSet), pBinding(InBinding), _Key(InKey)
{}

IRenderSystem::DescriptorUpdate::Key::~Key()
{
	pSet = nullptr;
	pBinding = nullptr;
	_Key = -1;
}

size_t* IRenderSystem::DescriptorUpdate::Key::First()
{
	return &_Key;
}

const size_t* IRenderSystem::DescriptorUpdate::Key::First() const
{
	return &_Key;
}

size_t IRenderSystem::DescriptorUpdate::Key::Length() const 
{
	return sizeof(size_t);
}

size_t IRenderSystem::DescriptorUpdate::CalcKey(Handle<SDescriptorSet> SetHnd, uint32 Binding)
{
	size_t binding = static_cast<size_t>(Binding);
	return (static_cast<size_t>(SetHnd) << binding) | binding;
}