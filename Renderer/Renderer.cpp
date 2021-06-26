#include "Renderer.h"
#include "API/Device.h"
#include "Library/Math/Operations.h"
#include "Library/Containers/Node.h"
#include "Library/Algorithms/QuickSort.h"
#include "Library/Random/Xoroshiro.h"
#include "API/Vk/Src/spirv_reflect.h"
#include "SubSystem/Time/ScopedTimer.h"

Handle<ISystem> g_RenderSystemHandle;
LinearAllocator g_RenderSystemAllocator;
Xoroshiro64 g_Uid(OS::GetPerfCounter());

//LinearAllocator IRenderSystem::DrawManager::_DrawCommandAllocator = {};
//LinearAllocator IRenderSystem::DrawManager::_TransformAllocator = {};
//uint32 IRenderSystem::DrawManager::_NumDrawables = 0;
//
//Map<size_t, IRenderSystem::DrawManager::EntryContainer> IRenderSystem::DrawManager::_InstancedDraws = {};
//Map<size_t, IRenderSystem::DrawManager::EntryContainer> IRenderSystem::DrawManager::_NonInstancedDraws = {};

LinearAllocator IRenderSystem::BindableManager::_BindableAllocator = {};
Map<size_t, IRenderSystem::BindableManager::BindableRange> IRenderSystem::BindableManager::_Bindables = {};
IRenderSystem::BindableManager::BindableRange IRenderSystem::BindableManager::_GlobalBindables = {};

struct DescriptorUpdate
{
  struct Key
  {
    Ref<SDescriptorSet> pSet;
    Ref<SDescriptorSetLayout::Binding> pBinding;
    size_t _Key;
    uint32 _FirstImage;

    Key() : pSet{}, pBinding{}, _Key{ 0 }, _FirstImage{ 0 } {}
    Key(Ref<SDescriptorSet> InSet, Ref<SDescriptorSetLayout::Binding> InBinding, size_t InKey) :
      pSet{ InSet }, pBinding{ InBinding }, _Key{ InKey }, _FirstImage{ 0 } {}
    ~Key()
    {
      pSet.~Ref();
      pBinding.~Ref();
      _Key = _FirstImage = 0;
    }

    bool operator==(const Key& Rhs) { return _Key == Rhs._Key; }
    bool operator!=(const Key& Rhs) { return _Key != Rhs._Key; }

    /**
    * NOTE(Ygsm):
    * These functions are needed to make hashing custom structs work.
    */
    size_t*       First()         { return &_Key; }
    const size_t* First()   const { return &_Key; }
    size_t        Length()  const { return sizeof(size_t); }
  };

  template <typename UpdateObject>
  using Container = Map<Key, UpdateObject, XxHash<Key>>;

  static size_t CalculateKey(Handle<SDescriptorSet> SetHnd, uint32 Binding)
  {
    size_t binding = static_cast<size_t>(Binding);
    return (static_cast<size_t>(SetHnd) << binding) | binding;
  }
};

DescriptorUpdate::Container<Array<VkDescriptorImageInfo>> g_DescriptorImageInfos;
DescriptorUpdate::Container<VkDescriptorBufferInfo>       g_DescriptorBufferInfos;

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

void IRenderSystem::IterateBindableRange(BindableManager::BindableRange& Range)
{
	ForwardNode<SBindable>* node = Range.pBegin;
	while (node)
	{
		BindBindable(node->Data);
		node = node->Next;
	}
}

void IRenderSystem::BindBindable(SBindable& Bindable)
{
	if (Bindable.Bound) { return; }

	VkCommandBuffer cmd = pDevice->GetCommandBuffer();
	const uint32 index = pDevice->GetCurrentFrameIndex();
	switch (Bindable.Type)
	{
	case EBindableType::Bindable_Type_Descriptor_Set: 
		{
      uint32* pOffset = nullptr;
      uint32 offsetCount = 0;
			StaticArray<uint32, MAX_DESCRIPTOR_BINDING_UPDATES> offsets;
			for (SDescriptorSetLayout::Binding& binding : Bindable.pSet->pLayout->Bindings)
			{
				if (binding.Type == Descriptor_Type_Dynamic_Storage_Buffer ||
					binding.Type == Descriptor_Type_Dynamic_Uniform_Buffer)
				{
					offsets.Push(static_cast<uint32>(binding.Offset[index]));
				}
			}
      pOffset = offsets.First();
      offsetCount = static_cast<uint32>(offsets.Length());
			vkCmdBindDescriptorSets(
				cmd,
				pDevice->GetPipelineBindPoint(Bindable.pSet->pLayout->pPipeline->BindPoint),
				Bindable.pSet->pLayout->pPipeline->LayoutHnd,
				Bindable.pSet->Slot,
				1,
				&Bindable.pSet->Hnd[index],
				offsetCount,
				pOffset
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
	IterateBindableRange(*range);
}

void IRenderSystem::DynamicStateSetup(Ref<SRenderPass> pRenderPass)
{
	VkCommandBuffer cmd = pDevice->GetCommandBuffer();
	VkViewport viewport = {};
	viewport.x = static_cast<float32>(pRenderPass->Pos.x);
	viewport.y = static_cast<float32>(pRenderPass->Pos.y);
	viewport.width = static_cast<float32>(pRenderPass->Extent.Width);
	viewport.height = static_cast<float32>(pRenderPass->Extent.Height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D rect = {};
	rect.offset.x = pRenderPass->Pos.x;
	rect.offset.y = pRenderPass->Pos.y;
	rect.extent.width = pRenderPass->Extent.Width;
	rect.extent.height = pRenderPass->Extent.Height;
	vkCmdSetScissor(cmd, 0, 1, &rect);
}

void IRenderSystem::BindBuffers()
{
	VkCommandBuffer cmd = pDevice->GetCommandBuffer();
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &VertexBuffer->Hnd, &offset);
	vkCmdBindVertexBuffers(cmd, 1, 1, &InstanceBuffer->Hnd, &offset);
	vkCmdBindIndexBuffer(cmd, IndexBuffer->Hnd, 0, VK_INDEX_TYPE_UINT32);

	IterateBindableRange(BindableManager::_GlobalBindables);
}

void IRenderSystem::BeginRenderPass(Ref<SRenderPass> pRenderPass)
{
	using ClearValues = StaticArray<VkClearValue, MAX_RENDERPASS_ATTACHMENT_COUNT>;
	using DynamicOffsets = StaticArray<uint32, MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS>;

	ClearValues clearValues;
	DynamicOffsets dynamicOffsets;
	StaticArray<VkImageView, 10> attImgViews;

	bool hasDepthStencil = false;
	const uint32 nextSwapchainIndex = pDevice->GetNextSwapchainImageIndex();
	const uint32 currentFrameIndex = pDevice->GetCurrentFrameIndex();

	size_t numClearValues = pRenderPass->ColorOutputs.Length();

	if (!pRenderPass->Flags.Has(RenderPass_Bit_No_Color_Render))
	{
		numClearValues++;
		attImgViews.Push(pRenderPass->pOwner->ColorImage.pImg->ImgViewHnd);
	}

	for (auto& [hnd, pImg] : pRenderPass->ColorOutputs)
	{
		attImgViews.Push(pImg->ImgViewHnd);
	}

	if (pRenderPass->Flags.Has(RenderPass_Bit_DepthStencil_Output) &&
		pRenderPass->DepthStencilOutput.Hnd != INVALID_HANDLE)
	{
		numClearValues++;
		hasDepthStencil = true;
		attImgViews.Push(pRenderPass->DepthStencilOutput.pImg->ImgViewHnd);
	}
	else if (!pRenderPass->Flags.Has(RenderPass_Bit_No_DepthStencil_Render))
	{
		numClearValues++;
		hasDepthStencil = true;
		attImgViews.Push(pRenderPass->pOwner->DepthStencilImage.pImg->ImgViewHnd);
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

	VkRenderPassAttachmentBeginInfo attInfo = {};
	attInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO;
	attInfo.attachmentCount = static_cast<uint32>(attImgViews.Length());
	attInfo.pAttachments = attImgViews.First();

	VkRenderPassBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	beginInfo.pNext = &attInfo;
	beginInfo.renderPass = pRenderPass->RenderPassHnd;
	//beginInfo.framebuffer = pRenderPass->Framebuffer[nextSwapchainIndex];
	beginInfo.framebuffer = pRenderPass->Framebuffer;
	beginInfo.renderArea.offset = { pRenderPass->Pos.x, pRenderPass->Pos.y };
	beginInfo.renderArea.extent = { pRenderPass->Extent.Width, pRenderPass->Extent.Height };
	beginInfo.clearValueCount = static_cast<uint32>(clearValues.Length());
	beginInfo.pClearValues = clearValues.First();

	vkCmdBeginRenderPass(pDevice->GetCommandBuffer(), &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
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

void IRenderSystem::BindPushConstant(const DrawCommand& Command)
{
  const VkCommandBuffer& cmd = pDevice->GetCommandBuffer();
  vkCmdPushConstants(
    cmd,
    Command.pPipeline->LayoutHnd,
    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    0,
    128,
    &Command.Constants
  );
}

//void IRenderSystem::PrepareDrawCommands()
//{
//	constexpr size_t mat4Size = sizeof(math::mat4);
//	const uint32 currentFrameIndex = pDevice->GetCurrentFrameIndex();
//	//StaticArray<math::mat4, 500> transforms;
//
//	const uint32 baseOffset = DrawManager::MaxDrawablesCount * currentFrameIndex;
//	uint32 updateCount = 0;
//	uint32 firstInstance = 0;
//
//	for (auto& [renderPass, instancedEntries] : DrawManager::_InstancedDraws)
//	{
//		DrawManager::EntryContainer& nonInstancedEntries = *DrawManager::_NonInstancedDraws.Get(renderPass);
//
//		for (DrawManager::InstanceEntry& entry : instancedEntries)
//		{
//			auto& transformRange = entry.TransformRange;
//			while (transformRange.pBegin)
//			{
//				const size_t pos = static_cast<size_t>(baseOffset) + static_cast<size_t>(updateCount);
//				CopyToBuffer(
//					InstanceBuffer.Value, 
//					&transformRange.pBegin->Data, 
//					mat4Size,
//					pos * mat4Size
//				);
//				updateCount++;
//				transformRange.pBegin = transformRange.pBegin->Next;
//			}
//
//			auto& drawCmds = entry.DrawRange;
//			ForwardNode<DrawCommand>* node = drawCmds.pBegin;
//			DrawManager::DrawCommandRange& range = Drawables[renderPass];
//
//			if (!range.pBegin && !range.pEnd)
//			{
//				range.pBegin = drawCmds.pBegin;
//				range.pEnd = drawCmds.pEnd;
//			}
//
//			while (node)
//			{
//				node->Data.InstanceOffset = baseOffset + firstInstance;
//				node = node->Next;
//			}
//
//			if (range.pEnd != drawCmds.pEnd)
//			{
//				range.pEnd->Next = drawCmds.pBegin;
//				range.pEnd = drawCmds.pEnd;
//			}
//			firstInstance = updateCount;
//		}
//
//		//for (DrawManager::InstanceEntry& entry : nonInstancedEntries)
//		//{
//		//	auto& transformRange = entry.TransformRange;
//		//	transforms.Push(Move(entry.TransformRange.pBegin->Data));
//
//		//	auto& drawCmds = entry.DrawRange;
//
//		//	DrawManager::DrawCommandRange& range = Drawables[renderPass];
//
//		//	if (!range.pBegin && !range.pEnd)
//		//	{
//		//		range.pBegin = drawCmds.pBegin;
//		//		range.pEnd = drawCmds.pBegin;
//		//	}
//
//		//	while (drawCmds.pBegin)
//		//	{
//		//		drawCmds.pBegin->Data.InstanceOffset = baseOffset + firstInstance;
//		//		range.pEnd->Next = drawCmds.pBegin;
//		//		range.pEnd = drawCmds.pBegin;
//		//		drawCmds.pBegin = drawCmds.pBegin->Next;
//		//	}
//		//	firstInstance = static_cast<uint32>(transforms.Length());
//		//}
//	}
//}

void IRenderSystem::MakeTransferToGpu()
{
	if (!pStaging->MakeTransfers.Value()) { return; }

	VkCommandBuffer cmd = pDevice->AllocateCommandBuffer(
		pDevice->GetGraphicsCommandPool(), 
		VK_COMMAND_BUFFER_LEVEL_PRIMARY, 
		1
	);
	if (cmd == VK_NULL_HANDLE) { return; }
	pDevice->BeginCommandBuffer(cmd, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

  if (pStaging->MakeTransfers.Has(Ownership_Transfer_Type_Vertex_Buffer))
  {
	  pDevice->BufferBarrier(
		  cmd,
		  VertexBuffer->Hnd,
		  VertexBuffer->Size,
		  0,
		  VK_ACCESS_TRANSFER_WRITE_BIT,
		  VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
		  VK_PIPELINE_STAGE_TRANSFER_BIT,
		  VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
		  pDevice->GetGraphicsQueue().FamilyIndex,
		  pDevice->GetGraphicsQueue().FamilyIndex
	  );
  }

  if (pStaging->MakeTransfers.Has(Ownership_Transfer_Type_Index_Buffer))
  {
    pDevice->BufferBarrier(
      cmd,
      IndexBuffer->Hnd,
      IndexBuffer->Size,
      0,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_ACCESS_INDEX_READ_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
      pDevice->GetGraphicsQueue().FamilyIndex,
      pDevice->GetGraphicsQueue().FamilyIndex
    );
  }

  if (pStaging->MakeTransfers.Has(Ownership_Transfer_Type_Buffer_Or_Image))
  {
    for (const auto& transfer : pStaging->GetOwnershipTransfers())
    {
      if (transfer.Type == Staging_Upload_Type_Buffer)
      {
        pDevice->BufferBarrier(
          cmd,
          transfer.pBuffer->Hnd,
          transfer.pBuffer->Size,
          0,
          VK_ACCESS_TRANSFER_WRITE_BIT,
          VK_ACCESS_MEMORY_READ_BIT,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
          pDevice->GetGraphicsQueue().FamilyIndex,
          pDevice->GetGraphicsQueue().FamilyIndex
        );
      }

      if (transfer.Type == Staging_Upload_Type_Image)
      {
        VkImageSubresourceRange range = {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = transfer.pImage->MipLevels;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        pDevice->ImageBarrier(
          cmd,
          transfer.pImage->ImgHnd,
          &range,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
          pDevice->GetGraphicsQueue().FamilyIndex,
          pDevice->GetGraphicsQueue().FamilyIndex,
          VK_ACCESS_TRANSFER_WRITE_BIT,
          VK_ACCESS_SHADER_READ_BIT
        );
      }
    }
  }

	VkSubmitInfo submit = {};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	pDevice->EndCommandBuffer(cmd);
	vkQueueSubmit(
		pDevice->GetGraphicsQueue().Hnd,
		1,
		&submit,
		VK_NULL_HANDLE
	);
	vkQueueWaitIdle(pDevice->GetGraphicsQueue().Hnd);
	pDevice->MoveToZombieList(cmd, pDevice->GetGraphicsCommandPool());
  pStaging->MakeTransfers.Assign(0);
}

void IRenderSystem::Clear()
{
  for (Array<DrawCommand>& drawCommands : Drawables)
  {
    drawCommands.Empty();
  }
  FlushInstanceBuffer();

  g_DescriptorBufferInfos.Empty();
  g_DescriptorImageInfos.Empty();

	for (auto& pair : BindableManager::_Bindables)
	{
		pair.Value = {};
	}
	BindableManager::_BindableAllocator.FlushMemory();
}

uint64 IRenderSystem::GenHashForImageSampler(const ImageSamplerState& State)
{
	constexpr uint64 bitsInBytes = 8;
	uint64 addmu = static_cast<uint64>(State.AddressModeU) << (bitsInBytes * 0);
	uint64 addmv = static_cast<uint64>(State.AddressModeV) << (bitsInBytes * 1);
	uint64 addmw = static_cast<uint64>(State.AddressModeW) << (bitsInBytes * 2);
	uint64 ani = static_cast<uint64>(State.AnisotropyLvl) << (bitsInBytes * 3);
	uint64 minf = static_cast<uint64>(State.MinFilter) << (bitsInBytes * 4);
	uint64 magf = static_cast<uint64>(State.MagFilter) << (bitsInBytes * 5);
	uint64 cmp = static_cast<uint64>(State.CompareOp) << (bitsInBytes * 6);
	uint64 hash = addmu | addmv | addmw | ani | minf | magf | cmp;
	XXHash64(&hash, sizeof(uint64), &hash);
	return hash;
}

void IRenderSystem::BlitToDefault()
{
	const auto& [hnd, pImg] = pFrameGraph->ColorImage;

	VkImage src = pImg->ImgHnd;
	VkImage dst = pDevice->GetNextImageInSwapchain();

	VkImageSubresourceRange range = {};
	range.levelCount = 1;
	range.layerCount = 1;
	range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = pDevice->GetGraphicsQueue().FamilyIndex;
	barrier.dstQueueFamilyIndex = pDevice->GetPresentationQueue().FamilyIndex;
	barrier.image = dst;
	barrier.subresourceRange = range;

	const IRenderDevice::VulkanSwapchain& swapchain = pDevice->GetSwapchain();

	VkOffset3D srcOffset = { static_cast<int32>(pImg->Width), static_cast<int32>(pImg->Height), 1 };
	VkOffset3D dstOffset = { static_cast<int32>(swapchain.Extent.width), static_cast<int32>(swapchain.Extent.height), 1 };

	VkImageBlit region = {};
	region.srcOffsets[1] = srcOffset;
	region.dstOffsets[1] = dstOffset;
	region.srcSubresource.mipLevel = 0;
	region.srcSubresource.layerCount = 1;
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.mipLevel = 0;
	region.dstSubresource.layerCount = 1;
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	vkCmdPipelineBarrier(
		pDevice->GetCommandBuffer(),
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier
	);

	vkCmdBlitImage(
		pDevice->GetCommandBuffer(),
		src,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		dst,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region,
		VK_FILTER_LINEAR
	);

	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	vkCmdPipelineBarrier(
		pDevice->GetCommandBuffer(),
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier
	);
}

void IRenderSystem::CopyToBuffer(Ref<SMemoryBuffer> pBuffer, void* Data, size_t Size, size_t Offset, bool Update)
{
	uint8* data = reinterpret_cast<uint8*>(pBuffer->pData + Offset);
	IMemory::Memcpy(data, Data, Size);
	if (Update) { pBuffer->Offset += Size; }
}

uint32 IRenderSystem::GetImageUsageFlags(Ref<SImage> pImg)
{
	VkImageUsageFlags usage = 0;
	for (uint32 i = 0; i < Image_Usage_Max; i++)
	{
		if (!pImg->Usage.Has(i)) { continue; }
		usage |= pDevice->GetImageUsage(i);
	}
	return usage;
}

uint32 IRenderSystem::GetImageFormat(Ref<SImage> pImg)
{
	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	if (pImg->Usage.Has(Image_Usage_Depth_Stencil_Attachment))
	{
		format = VK_FORMAT_D24_UNORM_S8_UINT;
	}
	return format;
}

IRenderSystem::IRenderSystem(EngineImpl& InEngine, Handle<ISystem> Hnd) :
	Engine(InEngine),
  pDevice{},
  pStore{},
	pStaging{},
	pFrameGraph{},
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
	constexpr size_t drawCmdAllocatorSize = sizeof(DrawCommand) * MAX_DRAWABLE_COUNT;
	constexpr size_t transformAllocatorSize = sizeof(math::mat4) * MAX_DRAWABLE_COUNT;

	g_RenderSystemHandle = Hnd;

	/* !!! */
	g_RenderSystemAllocator.Initialize(KILOBYTES(32));
	
	/* !!! */
	//DrawManager::_DrawCommandAllocator.Initialize(drawCmdAllocatorSize);
	/* !!! */
	//DrawManager::_TransformAllocator.Initialize(transformAllocatorSize);

	/* !!! */
	BindableManager::_BindableAllocator.Initialize(KILOBYTES(16));
	BindableManager::_Bindables.Reserve(MAX_FRAMEGRAPH_PASS_COUNT);
	Drawables.Reserve(MAX_FRAMEGRAPH_PASS_COUNT);

	/* !!! */
  g_DescriptorImageInfos.Reserve(16);
  g_DescriptorBufferInfos.Reserve(16);
	/* !!! */

	pStore = IAllocator::New<IDeviceStore>(g_RenderSystemAllocator);

	pDevice = IAllocator::New<IRenderDevice>(g_RenderSystemAllocator);
	pDevice->Initialize(this->Engine);

	pStaging = IAllocator::New<IStagingManager>(g_RenderSystemAllocator, *this);
	pStaging->Initialize();

	pFrameGraph = IAllocator::New<IFrameGraph>(g_RenderSystemAllocator, *this);
	//pFrameGraph->Initialize(Engine.GetWindowInformation().Extent);

	/* !!! */
	// Vertex buffer ...
	BufferAllocateInfo allocInfo = {};
	allocInfo.Locality = Buffer_Locality_Gpu;
	allocInfo.Size = MEGABYTES(256);
	allocInfo.Type.Assign((1 << Buffer_Type_Vertex) | (1 << Buffer_Type_Transfer_Dst));

	Handle<SMemoryBuffer> hnd = AllocateNewBuffer(allocInfo);
	VertexBuffer = DefaultBuffer(hnd, pStore->GetBuffer(hnd));
	BuildBuffer(hnd);

	// Index buffer ...
	allocInfo.Type.Assign((1 << Buffer_Type_Index) | (1 << Buffer_Type_Transfer_Dst));
	hnd = AllocateNewBuffer(allocInfo);
	IndexBuffer = DefaultBuffer(hnd, pStore->GetBuffer(hnd));
	BuildBuffer(hnd);

	// Instance buffer ...
	allocInfo.Type.Assign((1 << Buffer_Type_Vertex) | (1 << Buffer_Type_Transfer_Dst));
	allocInfo.Locality = Buffer_Locality_Cpu_To_Gpu;
	allocInfo.Size =  MEGABYTES(64);
	hnd = AllocateNewBuffer(allocInfo);
	InstanceBuffer = DefaultBuffer(hnd, pStore->GetBuffer(hnd));
	BuildBuffer(hnd);
}

void IRenderSystem::OnUpdate()
{
	if (Engine.HasWindowSizeChanged())
	{
		const Extent2D& extent = Engine.GetWindowInformation().Extent;
		pDevice->OnWindowResize(extent.Width, extent.Height);
	}

	MakeTransferToGpu();
	UpdateDescriptorSetInQueue();
	
	pDevice->BeginFrame();

	if (!pDevice->RenderThisFrame())
	{
		Clear();
		return;
	}
	BindBuffers();
	for (auto& [hnd, pRenderPass] : pFrameGraph->RenderPasses)
	{

		BeginRenderPass(pRenderPass);
		BindBindablesForRenderpass(hnd);
		DynamicStateSetup(pRenderPass);
    for (const DrawCommand& command : Drawables[pRenderPass->IndexForDraw])
    {
      BindPushConstant(command);
      RecordDrawCommand(command);
    }
		EndRenderPass(pRenderPass);
	}
	BlitToDefault();
	pDevice->EndFrame();
	Clear();
}

void IRenderSystem::OnTerminate()
{
	pStaging->Terminate();
	pFrameGraph->Terminate();

  Drawables.Release();

	DestroyBuffer(VertexBuffer);
	DestroyBuffer(IndexBuffer);
	DestroyBuffer(InstanceBuffer);

	pDevice->Terminate();
	g_RenderSystemAllocator.Terminate();
}

Handle<SDescriptorPool> IRenderSystem::CreateDescriptorPool()
{
	size_t id = g_Uid();
	Ref<SDescriptorPool> pPool = pStore->NewDescriptorPool(id);
	if (!pPool) { return INVALID_HANDLE; }
	return id;
}

bool IRenderSystem::DescriptorPoolAddSizeType(Handle<SDescriptorPool> Hnd, SDescriptorPool::Size Type)
{
	Ref<SDescriptorPool> pPool = pStore->GetDescriptorPool(Hnd);
	if (!pPool) 
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
	Ref<SDescriptorPool> pPool = pStore->GetDescriptorPool(Hnd);
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
	info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	info.maxSets = maxSets;
	info.pPoolSizes = poolSizes.First();
	info.poolSizeCount = static_cast<uint32>(poolSizes.Length());

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkCreateDescriptorPool(pDevice->GetDevice(), &info, nullptr, &pPool->Hnd[i]);
	}

	return true;
}

bool IRenderSystem::DestroyDescriptorPool(Handle<SDescriptorPool>& Hnd)
{
	Ref<SDescriptorPool> pPool = pStore->GetDescriptorPool(Hnd);
	if (!pPool)
	{
		VKT_ASSERT(false && "Pool with handle does not exist.");
		return false;
	}
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		pDevice->MoveToZombieList(pPool->Hnd[i], IRenderDevice::EHandleType::Handle_Type_Descriptor_Pool);
	}
	pStore->DeleteDescriptorPool(Hnd);
	Hnd = INVALID_HANDLE;

	return true;
}

Handle<SDescriptorSetLayout> IRenderSystem::CreateDescriptorSetLayout()
{
	size_t id = g_Uid();
	Ref<SDescriptorSetLayout> pSetLayout = pStore->NewDescriptorSetLayout(id);
	if (!pSetLayout) { return INVALID_HANDLE; }
	return id;
}

bool IRenderSystem::DescriptorSetLayoutAddBinding(const DescriptorSetLayoutBindingInfo& BindInfo)
{
	Ref<SDescriptorSetLayout> pSetLayout = pStore->GetDescriptorSetLayout(BindInfo.LayoutHnd);
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
	binding.Stride = BindInfo.Stride;

	pSetLayout->Bindings.Push(Move(binding));

	return true;
}

bool IRenderSystem::BuildDescriptorSetLayout(Handle<SDescriptorSetLayout> Hnd)
{
	using BindingsContainer = StaticArray<VkDescriptorSetLayoutBinding, MAX_DESCRIPTOR_SET_LAYOUT_BINDINGS>;

	Ref<SDescriptorSetLayout> pSetLayout = pStore->GetDescriptorSetLayout(Hnd);
	if (!pSetLayout)
	{
		VKT_ASSERT(false && "Layout with handle does not exist.");
		return false;
	}

	BindingsContainer layoutBindings;
  Array<VkDescriptorBindingFlags> flags(pSetLayout->Bindings.Length());

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
    flags.Push(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
	}

  VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags = {};
  bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
  bindingFlags.pBindingFlags = flags.First();
  bindingFlags.bindingCount = static_cast<uint32>(flags.Length());

	VkDescriptorSetLayoutCreateInfo create = {};
	create.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  create.pNext = &bindingFlags;
	create.bindingCount = static_cast<uint32>(layoutBindings.Length());
	create.pBindings = layoutBindings.First();

	if (vkCreateDescriptorSetLayout(pDevice->GetDevice(), &create, nullptr, &pSetLayout->Hnd) != VK_SUCCESS)
	{
		VKT_ASSERT(false && "Failed to create descriptor set layout on GPU");
	}

	return true;
}

bool IRenderSystem::DestroyDescriptorSetLayout(Handle<SDescriptorSetLayout>& Hnd)
{
	Ref<SDescriptorSetLayout> pSetLayout = pStore->GetDescriptorSetLayout(Hnd);
	if (!pSetLayout)
	{
		VKT_ASSERT(false && "Layout with handle does not exist.");
		return false;
	}
	pDevice->MoveToZombieList(pSetLayout->Hnd, IRenderDevice::EHandleType::Handle_Type_Descriptor_Set_Layout);
	pStore->DeleteDescriptorSetLayout(Hnd);
	Hnd = INVALID_HANDLE;

	return true;
}

Handle<SDescriptorSet> IRenderSystem::CreateDescriptorSet(const DescriptorSetAllocateInfo& AllocInfo)
{
	Ref<SDescriptorPool> pPool = pStore->GetDescriptorPool(AllocInfo.PoolHnd);
	Ref<SDescriptorSetLayout> pLayout = pStore->GetDescriptorSetLayout(AllocInfo.LayoutHnd);
	if (!pPool || !pLayout) { return INVALID_HANDLE; }

	size_t id = g_Uid();
	Ref<SDescriptorSet> pSet = pStore->NewDescriptorSet(id);
	if (!pSet) { return INVALID_HANDLE; }

	pSet->pPool = pPool;
	pSet->pLayout = pLayout;
	pSet->Slot = AllocInfo.Slot;

	return id;
}

bool IRenderSystem::DescriptorSetMapToBuffer(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, Handle<SMemoryBuffer> BufferHnd, size_t Offset0, size_t Offset1)
{
	Ref<SDescriptorSet> pSet = pStore->GetDescriptorSet(Hnd);
	Ref<SMemoryBuffer> pBuffer = pStore->GetBuffer(BufferHnd);

	if (!pSet || !pBuffer) { return false; }

	Ref<SDescriptorSetLayout::Binding> pBinding;
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

	if (pBuffer->Size < (Offset0 + Offset1)) { return false; }

	if (pBinding->Type == Descriptor_Type_Dynamic_Uniform_Buffer ||
		pBinding->Type == Descriptor_Type_Storage_Buffer)
	{
		pBinding->Offset[0] = Offset0;
		pBinding->Offset[1] = Offset1;
	}

	pBinding->pBuffer = pBuffer;

	DescriptorUpdate::Key key(pSet, pBinding, DescriptorUpdate::CalculateKey(Hnd, BindingSlot));

  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.buffer = pBuffer->Hnd;
  bufferInfo.offset = pBinding->Offset[0];
  bufferInfo.range = pBinding->Stride;

  g_DescriptorBufferInfos[key] = bufferInfo;

	return true;
}

bool IRenderSystem::DescriptorSetMapToImage(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, Handle<SImage>* pImgHnd, uint32 NumImages, Handle<SImageSampler> ImgSamplerHnd, uint32 DstIndex)
{
  if (!NumImages) { return false; }
  Ref<SDescriptorSet> pSet = pStore->GetDescriptorSet(Hnd);
  if (!pSet) { return false; }

  Ref<SDescriptorSetLayout::Binding> pBinding;
  for (SDescriptorSetLayout::Binding& b : pSet->pLayout->Bindings)
  {
    if (b.BindingSlot != BindingSlot) { continue; }
    pBinding = &b;
    break;
  }

  if (!pBinding) { return false; }

  VKT_ASSERT(pBinding->Type == Descriptor_Type_Sampled_Image);
  if (pBinding->Type != Descriptor_Type_Sampled_Image)
  {
    return false;
  }
 
  Ref<SImageSampler> pSampler = pStore->GetImageSampler(ImgSamplerHnd);
  DescriptorUpdate::Key key(pSet, pBinding, DescriptorUpdate::CalculateKey(Hnd, BindingSlot));
  key._FirstImage = DstIndex;

  for (uint32 i = 0; i < NumImages; i++)
  {
    Ref<SImage> pImg = pStore->GetImage(pImgHnd[i]);
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.sampler = pSampler->Hnd;
    imageInfo.imageView = pImg->ImgViewHnd;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    g_DescriptorImageInfos[key].Push(imageInfo);
  }

  return true;
}

bool IRenderSystem::DescriptorSetBindToGlobal(Handle<SDescriptorSet> Hnd)
{
	Ref<SDescriptorSet> pSet = pStore->GetDescriptorSet(Hnd);
	if (!pSet) { return false; }

	ForwardNode<SBindable>* bindableNode = IAllocator::New<ForwardNode<SBindable>>(BindableManager::_BindableAllocator);
	BindableManager::BindableRange& range = BindableManager::_GlobalBindables;

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

bool IRenderSystem::BuildDescriptorSet(Handle<SDescriptorSet> Hnd)
{
	Ref<SDescriptorSet> pSet = pStore->GetDescriptorSet(Hnd);
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
	Ref<SDescriptorSet> pSet = pStore->GetDescriptorSet(Hnd);
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

bool IRenderSystem::DestroyDescriptorSet(Handle<SDescriptorSet>& Hnd)
{
	Ref<SDescriptorSet> pSet = pStore->GetDescriptorSet(Hnd);
	if (!pSet)
	{
		VKT_ASSERT(false && "Set with handle does not exist.");
		return false;
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		pDevice->MoveToZombieList(pSet->Hnd[i], pSet->pPool->Hnd[i]);
	}
	pStore->DeleteDescriptorSet(Hnd);
	Hnd = INVALID_HANDLE;

	return true;
}

bool IRenderSystem::DescriptorSetUpdateDataAtBinding(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, void* Data, size_t Size)
{
	Ref<SDescriptorSet> pSet = pStore->GetDescriptorSet(Hnd);
	const uint32 index = pDevice->GetCurrentFrameIndex();
	if (!pSet) { return false; }

	const SDescriptorSetLayout::Binding* pBinding = nullptr;
	for (const SDescriptorSetLayout::Binding& binding : pSet->pLayout->Bindings)
	{
		if (binding.BindingSlot == BindingSlot)
		{
			pBinding = &binding;
			break;
		}
	}

	if (pBinding->Type == Descriptor_Type_Input_Attachment ||
		pBinding->Type == Descriptor_Type_Sampled_Image ||
		pBinding->Type == Descriptor_Type_Sampler)
	{
		return false;
	}

	if (!pBinding->pBuffer) { return false; }

	CopyToBuffer(pBinding->pBuffer, Data, Size, pBinding->Offset[index]);

	return true;
}

size_t IRenderSystem::PadToAlignedSize(size_t Size)
{
	const size_t minUboAlignment = pDevice->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
	return (Size + minUboAlignment - 1) & ~(minUboAlignment - 1);
}

void IRenderSystem::UpdateDescriptorSetInQueue()
{
	// Update buffers ....
	for (auto& [key, info] : g_DescriptorBufferInfos)
	{
		Ref<SDescriptorSet> pSet = key.pSet;
		Ref<SDescriptorSetLayout::Binding> pBinding = key.pBinding;

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = pBinding->BindingSlot;
		write.descriptorCount = 1;
		write.descriptorType = pDevice->GetDescriptorType(pBinding->Type);
		write.pBufferInfo = &info;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			write.dstSet = pSet->Hnd[i];
			vkUpdateDescriptorSets(pDevice->GetDevice(), 1, &write, 0, nullptr);
		}
	}

	// Update images ....
	for (auto& [key, container] : g_DescriptorImageInfos)
	{
		Ref<SDescriptorSet> pSet = key.pSet;
		Ref<SDescriptorSetLayout::Binding> pBinding = key.pBinding;

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstBinding = pBinding->BindingSlot;
		write.descriptorCount = static_cast<uint32>(container.Length());
		write.descriptorType = pDevice->GetDescriptorType(pBinding->Type);
		write.pImageInfo = container.First();
    write.dstArrayElement = key._FirstImage;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			write.dstSet = pSet->Hnd[i];
			vkUpdateDescriptorSets(pDevice->GetDevice(), 1, &write, 0, nullptr);
		}

    container.Release();
	}
}

Handle<SMemoryBuffer> IRenderSystem::AllocateNewBuffer(const BufferAllocateInfo& AllocInfo)
{
	size_t id = g_Uid();
	Ref<SMemoryBuffer> pBuffer = pStore->NewBuffer(id);
	if (!pBuffer) { return INVALID_HANDLE; }

	pBuffer->Locality = AllocInfo.Locality;
	pBuffer->Size = AllocInfo.Size;
	pBuffer->Type = AllocInfo.Type;
	pBuffer->pData = nullptr;

	return id;
}

bool IRenderSystem::CopyDataToBuffer(Handle<SMemoryBuffer> Hnd, void* Data, size_t Size, size_t Offset)
{
	Ref<SMemoryBuffer> pBuffer = pStore->GetBuffer(Hnd);
	if (!pBuffer) { return false; }
	if (pBuffer->Size < Offset) { return false; }
	CopyToBuffer(pBuffer, Data, Size, Offset);
	return true;
}

bool IRenderSystem::BuildBuffer(Handle<SMemoryBuffer> Hnd)
{
	Ref<SMemoryBuffer> pBuffer = pStore->GetBuffer(Hnd);
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
	) != VK_SUCCESS)
	{
		return false;
	}

	if (pBuffer->Locality != Buffer_Locality_Gpu)
	{
		vmaMapMemory(pDevice->GetAllocator(), pBuffer->Allocation, reinterpret_cast<void**>(&pBuffer->pData));
		vmaUnmapMemory(pDevice->GetAllocator(), pBuffer->Allocation);
	}

	return true;
}

bool IRenderSystem::DestroyBuffer(Handle<SMemoryBuffer>& Hnd)
{
	Ref<SMemoryBuffer> pBuffer = pStore->GetBuffer(Hnd);
	if (!pBuffer) { return false; }

	pDevice->MoveToZombieList(pBuffer->Hnd, IRenderDevice::EHandleType::Handle_Type_Buffer, pBuffer->Allocation);
	pStore->DeleteBuffer(Hnd);
	Hnd = INVALID_HANDLE;

	return true;
}

Handle<SImage> IRenderSystem::CreateImage(uint32 Width, uint32 Height, uint32 Channels, ETextureType Type, bool GenMips)
{
	size_t id = g_Uid();
	Ref<SImage> pImg = pStore->NewImage(id);
	if (!pImg) { return INVALID_HANDLE; }

	pImg->Width = Width;
	pImg->Height = Height;
	pImg->Channels = Channels;
	pImg->Type = Type;
  pImg->Size = (size_t)Width * (size_t)Height * (size_t)Channels;
	pImg->MipMaps = GenMips;
  pImg->Usage.Set(Image_Usage_Transfer_Dst);
  pImg->Usage.Set(Image_Usage_Sampled);

	return id;
}

bool IRenderSystem::BuildImage(Handle<SImage> Hnd)
{
	Ref<SImage> pImg = pStore->GetImage(Hnd);
	if (!pImg) { return false; }

	VkFormat format = static_cast<VkFormat>(GetImageFormat(pImg));
	VkImageUsageFlags usage = GetImageUsageFlags(pImg);

	if (pImg->MipMaps)
	{
		const float32 texWidth = static_cast<float32>(pImg->Width);
		const float32 texHeight = static_cast<float32>(pImg->Height);
		pImg->MipLevels = static_cast<uint32>(math::FFloor(math::FLog2(Max<float32>(texWidth, texHeight)))) + 1;

		if (pImg->MipLevels > MAX_IMAGE_MIP_MAP_LEVEL)
		{
			pImg->MipLevels = MAX_IMAGE_MIP_MAP_LEVEL;
		}
		//pImg->Usage.Set(Image_Usage_Transfer_Dst);
	}

  usage = GetImageUsageFlags(pImg);

	VkImageCreateInfo img = {};
	img.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	img.imageType = pDevice->GetImageType(pImg->Type);
	img.format = format;
	img.samples = VK_SAMPLE_COUNT_1_BIT;
	img.extent = { pImg->Width, pImg->Height, 1 };
	img.tiling = VK_IMAGE_TILING_OPTIMAL;
	img.mipLevels = pImg->MipLevels;
	img.usage = usage;
	img.arrayLayers = 1;
	img.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	img.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo alloc = {};
	alloc.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vmaCreateImage(pDevice->GetAllocator(), &img, &alloc, &pImg->ImgHnd, &pImg->Allocation, nullptr);

	VkImageViewCreateInfo imgView = {};
	imgView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imgView.format = format;
	imgView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgView.subresourceRange.baseMipLevel = 0;
	imgView.subresourceRange.levelCount = pImg->MipLevels;
	imgView.subresourceRange.baseArrayLayer = 0;
	imgView.subresourceRange.layerCount = 1;
	imgView.viewType = pDevice->GetImageViewType(pImg->Type);
	imgView.image = pImg->ImgHnd;

	if (pImg->Usage.Has(Image_Usage_Depth_Stencil_Attachment))
	{
		imgView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	vkCreateImageView(pDevice->GetDevice(), &imgView, nullptr, &pImg->ImgViewHnd);

	return true;
}

bool IRenderSystem::DestroyImage(Handle<SImage>& Hnd)
{
	Ref<SImage> pImg = pStore->GetImage(Hnd);
	if (!pImg) { return false; }

	pDevice->MoveToZombieList(pImg->ImgHnd, IRenderDevice::EHandleType::Handle_Type_Image, pImg->Allocation);
	pDevice->MoveToZombieList(pImg->ImgViewHnd, IRenderDevice::EHandleType::Handle_Type_Image_View);
	pStore->DeleteImage(Hnd);
	Hnd = INVALID_HANDLE;

	return true;
}

Handle<SImageSampler> IRenderSystem::CreateImageSampler(const ImageSamplerCreateInfo& CreateInfo)
{
	size_t id = g_Uid();
	uint64 samplerHash = GenHashForImageSampler(CreateInfo);
	Ref<SImageSampler> pImgSampler = pStore->GetImageSamplerWithHash(samplerHash);
	if (pImgSampler) { return INVALID_HANDLE; }

	pImgSampler = pStore->NewImageSampler(id);
	if (!pImgSampler) { return INVALID_HANDLE; }

	pImgSampler->AddressModeU = CreateInfo.AddressModeU;
	pImgSampler->AddressModeV = CreateInfo.AddressModeV;
	pImgSampler->AddressModeW = CreateInfo.AddressModeW;
	pImgSampler->AnisotropyLvl = CreateInfo.AnisotropyLvl;
	pImgSampler->CompareOp = CreateInfo.CompareOp;
	pImgSampler->MagFilter = CreateInfo.MagFilter;
	pImgSampler->MinFilter = CreateInfo.MinFilter;
	pImgSampler->Hash = samplerHash;

	return id;
}

bool IRenderSystem::BuildImageSampler(Handle<SImageSampler> Hnd)
{
	Ref<SImageSampler> pImgSampler = pStore->GetImageSampler(Hnd);
	if (!pImgSampler) { return false; }

	VkSamplerCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.addressModeU = pDevice->GetSamplerAddressMode(pImgSampler->AddressModeU);
	createInfo.addressModeV = pDevice->GetSamplerAddressMode(pImgSampler->AddressModeV);
	createInfo.addressModeW = pDevice->GetSamplerAddressMode(pImgSampler->AddressModeW);
	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	createInfo.minFilter = pDevice->GetFilter(pImgSampler->MinFilter);
	createInfo.magFilter = pDevice->GetFilter(pImgSampler->MagFilter);

	if (pImgSampler->AnisotropyLvl)
	{
		const float32 hardwareAnisotropyLimit = pDevice->GetPhysicalDeviceProperties().limits.maxSamplerAnisotropy;
		createInfo.anisotropyEnable = VK_TRUE;
		createInfo.maxAnisotropy = (pImgSampler->AnisotropyLvl > hardwareAnisotropyLimit) ? hardwareAnisotropyLimit : pImgSampler->AnisotropyLvl;
	}

	if (pImgSampler->CompareOp)
	{
		createInfo.compareEnable = VK_TRUE;
		createInfo.compareOp = pDevice->GetCompareOp(pImgSampler->CompareOp);
	}

	if (vkCreateSampler(pDevice->GetDevice(), &createInfo, nullptr, &pImgSampler->Hnd) != VK_SUCCESS)
	{
		VKT_ASSERT(false && "Failed to create sampler for image");
		return false;
	}

	return true;
}

bool IRenderSystem::DestroyImageSampler(Handle<SImageSampler>& Hnd)
{
	Ref<SImageSampler> pImgSampler = pStore->GetImageSampler(Hnd);
	if (!pImgSampler) { return false; }

	pDevice->MoveToZombieList(pImgSampler->Hnd, IRenderDevice::EHandleType::Handle_Type_Image_Sampler);
	pStore->DeleteImageSampler(Hnd);
	Hnd = INVALID_HANDLE;

	return true;
}

IStagingManager& IRenderSystem::GetStagingManager()
{
	return *pStaging;
}

IFrameGraph& IRenderSystem::GetFrameGraph()
{
	return *pFrameGraph;
}

Handle<SShader> IRenderSystem::CreateShader(const String& Code, EShaderType Type)
{
	size_t id = g_Uid();
	Ref<SShader> pShader = pStore->NewShader(id);

	if (!pShader) { return INVALID_HANDLE; }
	pShader->Type = Type;

	if (!pDevice->ToSpirV(Code, pShader->SpirV, pShader->Type)) 
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

	pShader->SpirV.Release();

	return true;
}

bool IRenderSystem::DestroyShader(Handle<SShader>& Hnd)
{
	Ref<SShader> pShader = pStore->GetShader(Hnd);
	if (!pShader) { return false; }

	pDevice->MoveToZombieList(pShader->Hnd, IRenderDevice::EHandleType::Handle_Type_Shader);
	pStore->DeleteShader(Hnd);

	Hnd = INVALID_HANDLE;

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
	pPipeline->ColorOutputCount = static_cast<uint32>(pRenderPass->ColorOutputs.Length());

	if (!pRenderPass->Flags.Has(RenderPass_Bit_No_Color_Render))
	{
		pPipeline->ColorOutputCount++;
	}

	if (!pRenderPass->Flags.Has(RenderPass_Bit_No_DepthStencil_Render) ||
		pRenderPass->DepthStencilOutput.Hnd != INVALID_HANDLE)
	{
		pPipeline->HasDepthStencil = true;
	}

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

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = states;

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

bool IRenderSystem::DestroyPipeline(Handle<SPipeline>& Hnd)
{
	Ref<SPipeline> pPipeline = pStore->GetPipeline(Hnd);
	if (!pPipeline) { return false; }

	if (pPipeline->Hnd == VK_NULL_HANDLE) { return false; }

	pDevice->MoveToZombieList(pPipeline->Hnd, IRenderDevice::EHandleType::Handle_Type_Pipeline);
	pDevice->MoveToZombieList(pPipeline->LayoutHnd, IRenderDevice::EHandleType::Handle_Type_Pipeline_Layout);
	pStore->DeletePipeline(Hnd);

	return true;
}

bool IRenderSystem::BindDescriptorSet(Handle<SDescriptorSet> Hnd, Handle<SRenderPass> RenderpassHnd)
{
	Ref<SDescriptorSet> pSet = pStore->GetDescriptorSet(Hnd);
	Ref<SRenderPass> pRenderpass = pStore->GetRenderPass(RenderpassHnd);
	VKT_ASSERT(pSet && pRenderpass && "Invalid handle(s) supplied");

	if (!pSet || !pRenderpass) 
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

	if (!pPipeline || !pRenderpass)
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

void IRenderSystem::Draw(const DrawSubmissionInfo& Info)
{
	Ref<SRenderPass> pRenderPass = pStore->GetRenderPass(Info.RenderPassHnd);
  Ref<SPipeline> pPipeline = pStore->GetPipeline(Info.PipelineHnd);

	VKT_ASSERT(pRenderPass && "RenderPass Handle is invalid!");
	VKT_ASSERT(pPipeline && "Pipeline Handle is invalid!");
	VKT_ASSERT(Info.DrawCount && "Drawable count can not be zero!");

  Array<DrawCommand>& drawCommandsAtPass = Drawables[pRenderPass->IndexForDraw];
  uint32 instanceOffset = static_cast<uint32>(InstanceBuffer->Offset) / static_cast<uint32>(sizeof(math::mat4));
  CopyToBuffer(InstanceBuffer, Info.pTransforms, sizeof(math::mat4) * Info.TransformCount, InstanceBuffer->Offset, true);

  for (uint32 i = 0; i < Info.DrawCount; i++)
  {
    const VertexInformation& vertInfo = Info.pVertexInformation[i];
    const IndexInformation& indexInfo = Info.pIndexInformation[i];

    DrawCommand drawCmd;
    drawCmd.InstanceCount = Info.TransformCount;
    drawCmd.InstanceOffset = instanceOffset;
    drawCmd.VertexOffset = vertInfo.VertexOffset;
    drawCmd.NumVertices = vertInfo.NumVertices;
    drawCmd.IndexOffset = indexInfo.IndexOffset;
    drawCmd.NumIndices = indexInfo.NumIndices;
    drawCmd.pPipeline = pPipeline;

    if (Info.ConstantsCount)
    {
      uint8* pSrc = reinterpret_cast<uint8*>(Info.pConstants) + (Info.ConstantTypeSize * i);
      IMemory::Memcpy(drawCmd.Constants, pSrc, Info.ConstantTypeSize);
      drawCmd.HasPushConstants = true;
    }

    drawCommandsAtPass.Push(Move(drawCmd));
  }
}

//const uint32 IRenderSystem::GetMaxDrawablesCount() const
//{
//	return MAX_DRAWABLE_COUNT;
//}

//const uint32 IRenderSystem::GetDrawableCount() const
//{
//	return DrawManager::_NumDrawables;
//}

const uint32 IRenderSystem::GetCurrentFrameIndex() const
{
	return pDevice->GetCurrentFrameIndex();
}

void IRenderSystem::FlushVertexBuffer()
{
	VertexBuffer->Offset = 0;
	//IMemory::Memzero(VertexBuffer->pData, VertexBuffer->Size);
}

void IRenderSystem::FlushIndexBuffer()
{
  IndexBuffer->Offset = 0;
	//IMemory::Memzero(IndexBuffer->pData, IndexBuffer->Size);
}

void IRenderSystem::FlushInstanceBuffer()
{
	InstanceBuffer->Offset = 0;
	//IMemory::Memzero(InstanceBuffer->pData, InstanceBuffer->Size);
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
