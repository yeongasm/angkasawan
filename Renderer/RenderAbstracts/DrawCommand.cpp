#include "DrawCommand.h"
#include "API/Context.h"
#include "RenderAbstracts/FrameGraph.h"
#include "Assets/Assets.h"
#include "MaterialManager.h"

void IRDrawManager::AddDrawCommandList(uint32 PassId)
{
	auto& instanceEntries = InstanceDraws.Insert(PassId, {});
	instanceEntries.Reserve(128);
	auto& nonInstanceEntries = NonInstanceDraws.Insert(PassId, {});
	nonInstanceEntries.Reserve(128);
	auto& finalCmds = FinalDrawCommands.Insert(PassId, {});
	finalCmds.Reserve(1024);
}

void IRDrawManager::BindBuffers()
{
	const uint32 currentFrame = gpu::CurrentFrameIndex();
	gpu::BindVertexBuffer(*VertexBuffer, VertexFirstBinding, 0);
	gpu::BindInstanceBuffer(*InstanceBuffer, InstanceFirstBinding, 0);
	gpu::BindIndexBuffer(*IndexBuffer, 0);
}

IRDrawManager::IRDrawManager(LinearAllocator& InAllocator, IRFrameGraph& InFrameGraph, IRAssetManager& InAssetManager) :
	InstanceDraws(),
	NonInstanceDraws(),
	FinalDrawCommands(),
	Allocator(InAllocator),
	FrameGraph(InFrameGraph),
	AssetManager(InAssetManager),
	InstanceBuffer(nullptr),
	Offsets{0},
	NumOfDrawables(0),
	MaxDrawables(0),
	VertexFirstBinding(0),
	InstanceFirstBinding(0)
{}

IRDrawManager::~IRDrawManager()
{
	InstanceDraws.Release();
	NonInstanceDraws.Release();
	FinalDrawCommands.Release();
}

bool IRDrawManager::InitializeDrawManager(const DrawManagerInitInfo& AllocInfo)
{
	if (!AllocInfo.MaxDrawCount  ||
		!AllocInfo.pMemAllocInfo ||
		!AllocInfo.pMemManager)
	{
		return false;
	}

	const size_t mat4Size = gpu::PadSizeToAlignedSize(sizeof(math::mat4));
	MaxDrawables = AllocInfo.MaxDrawCount;

	size_t totalSize = mat4Size * MaxDrawables;
	AllocInfo.pMemAllocInfo->Size = totalSize * MAX_FRAMES_IN_FLIGHT;
	InstanceBuffer = reinterpret_cast<SRMemoryBuffer*>(Allocator.Malloc(sizeof(SRMemoryBuffer)));
	IMemory::InitializeObject(InstanceBuffer);
	IMemory::Memcpy(InstanceBuffer, AllocInfo.pMemAllocInfo, sizeof(SRMemoryBufferBase));

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		Offsets[i] = static_cast<uint32>(totalSize * i);
	}

	InstanceBuffer->Locality = Buffer_Locality_Cpu_To_Gpu;

	if (!gpu::CreateBuffer(*InstanceBuffer))
	{
		// Need to figure out a way to free the allocator ... 
		return false;
	}

	VertexBuffer = AllocInfo.pMemManager->GetBuffer(AllocInfo.VtxBufHnd);
	IndexBuffer = AllocInfo.pMemManager->GetBuffer(AllocInfo.IdxBufHnd);
	VertexFirstBinding = AllocInfo.VertexInputBinding;
	InstanceFirstBinding = AllocInfo.InstanceInputBinding;

	return true;
}

bool IRDrawManager::DrawModel(const ModelDrawInfo& DrawInfo)
{
	if (!DrawInfo.pModel /*||
		DrawInfo.PipelineHnd == INVALID_HANDLE*/)
	{
		return false;
	}

	//Model* model = AssetManager.GetModelWithHandle(DrawInfo.ModelHnd);
	Model* model = DrawInfo.pModel;

	if (!model) { return false; }
	if (!model->Length()) { return false; }

	if (NumOfDrawables >= MaxDrawables)
	{
		VKT_ASSERT(false && "Maximum number of draw calls allowed reached!");
		return false;
	}

	// Will throw an assertion if render pass with id does not exist.
	FrameGraph.GetRenderPass(DrawInfo.RenderpassId);

	// Drawables represent the total number of model transforms in this manager.
	NumOfDrawables++;
	InstanceEntries* entries = (DrawInfo.Instanced) ? &InstanceDraws[DrawInfo.RenderpassId] : &NonInstanceDraws[DrawInfo.RenderpassId];

	if (DrawInfo.Instanced)
	{
		SInstanceEntry* e = nullptr;
		for (SInstanceEntry& entry : *entries)
		{
			if (entry.Id != model->Id) { continue; }
			e = &entry;
			break;
		}

		if (e)
		{
			e->ModelTransform.Push(DrawInfo.Transform);
			for (DrawCommand& cmd : e->DrawCommands)
			{
				cmd.InstanceCount++;
			}
			return true;
		}
	}

	SInstanceEntry& entry = entries->Insert(SInstanceEntry());

	entry.ModelTransform.Push(DrawInfo.Transform);
	entry.Id = model->Id;
	
	uint32 i = 0;

	for (Mesh* mesh : *model)
	{
		DrawCommand& cmd = entry.DrawCommands.Insert(DrawCommand());
		cmd.VertexOffset = mesh->VtxOffset;
		cmd.IndexOffset = mesh->IdxOffset;
		cmd.NumVertices = mesh->NumOfVertices;
		cmd.NumIndices = mesh->NumOfIndices;
		cmd.InstanceCount = 1;
		cmd.PipelineHandle = DrawInfo.PipelineHandle;
		cmd.DescriptorSetHandle = DrawInfo.DescriptorSetHandle;

		// TODO(Ygsm):
		// Short fix ... need to figure out a better way.
		if (i != DrawInfo.NumMaterials)
		{
			cmd.PushConstantHandle = DrawInfo.pMaterial[i]->PushConstantHandle;
			i++;
		}
	}

	return true;
}

uint32 IRDrawManager::GetMaxDrawableCount() const
{
	return MaxDrawables;
}

void IRDrawManager::FinalizeInstances()
{
	if (!InstanceDraws.Length() && !NonInstanceDraws.Length())
	{
		return;
	}

	const size_t mat4Size = gpu::PadSizeToAlignedSize(sizeof(math::mat4));
	const uint32 currentFrame = gpu::CurrentFrameIndex();
	static Array<math::mat4> modelTransforms;

	uint32 baseOffset = MaxDrawables * currentFrame;
	uint32 firstInstance = 0;

	for (auto& [key, entry] : InstanceDraws)
	{
		DrawCommands* finalDrawCmds = &FinalDrawCommands[key];
		for (SInstanceEntry& entry : entry)
		{
			for (DrawCommand& command : entry.DrawCommands)
			{
				command.InstanceOffset = baseOffset + firstInstance;
				finalDrawCmds->Push(command);
			}
			firstInstance = static_cast<uint32>(modelTransforms.Append(entry.ModelTransform));
		}
	}

	for (auto& [key, entry] : NonInstanceDraws)
	{
		for (SInstanceEntry& entry : entry)
		{
			DrawCommands* finalDrawCmds = &FinalDrawCommands[key];
			for (DrawCommand& command : entry.DrawCommands)
			{
				command.InstanceOffset = baseOffset + firstInstance;
				finalDrawCmds->Push(command);
			}
			firstInstance = static_cast<uint32>(modelTransforms.Append(entry.ModelTransform));
		}
	}

	gpu::CopyToBuffer(*InstanceBuffer, modelTransforms.First(), NumOfDrawables * mat4Size, Offsets[currentFrame]);
	modelTransforms.Empty();
}

void IRDrawManager::Flush()
{
	for (auto& pair : InstanceDraws)
	{
		pair.Value.Empty();
	}

	for (auto& pair : NonInstanceDraws)
	{
		pair.Value.Empty();
	}

	for (auto& pair : FinalDrawCommands)
	{
		pair.Value.Empty();
	}

	NumOfDrawables = 0;
}

void IRDrawManager::Destroy()
{
	for (auto& pair : InstanceDraws)
	{
		pair.Value.Release();
	}

	for (auto& pair : NonInstanceDraws)
	{
		pair.Value.Release();
	}

	gpu::DestroyBuffer(*InstanceBuffer);
}

