#include "DrawCommand.h"
#include "API/Context.h"
#include "RenderAbstracts/FrameGraph.h"

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

IRDrawManager::IRDrawManager(LinearAllocator& InAllocator, IRFrameGraph& InFrameGraph) :
	InstanceDraws(),
	NonInstanceDraws(),
	FinalDrawCommands(),
	Allocator(InAllocator),
	FrameGraph(InFrameGraph),
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
	FMemory::InitializeObject(InstanceBuffer);
	FMemory::Memcpy(InstanceBuffer, AllocInfo.pMemAllocInfo, sizeof(SRMemoryBufferBase));

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

	return true;
}

bool IRDrawManager::DrawModel(Model& ToDraw, const math::mat4& Transform, uint32 PassId, bool Instanced)
{
	if (!ToDraw.Length())
	{
		return false;
	}

	if (NumOfDrawables >= MaxDrawables)
	{
		VKT_ASSERT(false && "Maximum number of draw calls allowed reached!");
		return false;
	}

	// Will throw an assertion if render pass with id does not exist.
	FrameGraph.GetRenderPass(PassId);

	// Drawables represent the total number of model transforms in this manager.
	NumOfDrawables++;
	InstanceEntries* entries = (Instanced) ? &InstanceDraws[PassId] : &NonInstanceDraws[PassId];

	if (Instanced)
	{
		SInstanceEntry* e = nullptr;
		for (SInstanceEntry& entry : *entries)
		{
			if (entry.Id != ToDraw.Id) { continue; }
			e = &entry;
			break;
		}

		if (e)
		{
			e->ModelTransform.Push(Transform);
			for (DrawCommand& cmd : e->DrawCommands)
			{
				cmd.InstanceCount++;
			}
			return true;
		}
	}

	SInstanceEntry& entry = entries->Insert(SInstanceEntry());

	entry.ModelTransform.Push(Transform);
	entry.Id = ToDraw.Id;
	
	for (Mesh* mesh : ToDraw)
	{
		DrawCommand& cmd = entry.DrawCommands.Insert(DrawCommand());
		//cmd.Id = PassId;
		cmd.VertexOffset = mesh->VtxOffset;
		cmd.IndexOffset = mesh->IdxOffset;
		cmd.NumVertices = mesh->NumOfVertices;
		cmd.NumIndices = mesh->NumOfIndices;
		cmd.InstanceCount = 1;
	}

	return true;
}

uint32 IRDrawManager::GetMaxDrawableCount() const
{
	return MaxDrawables;
}

void IRDrawManager::SetVertexInputFirstBinding(uint32 Binding)
{
	VertexFirstBinding = Binding;
}

void IRDrawManager::SetInstanceInputFirstBinding(uint32 Binding)
{
	InstanceFirstBinding = Binding;
}

void IRDrawManager::FinalizeInstances()
{
	const size_t mat4Size = gpu::PadSizeToAlignedSize(sizeof(math::mat4));
	const uint32 currentFrame = gpu::CurrentFrameIndex();
	static Array<math::mat4> modelTransforms;

	uint32 baseOffset = MaxDrawables * currentFrame;
	uint32 firstInstance = 0;

	for (auto& pair : InstanceDraws)
	{
		DrawCommands* finalDrawCmds = &FinalDrawCommands[pair.Key];
		for (SInstanceEntry& entry : pair.Value)
		{
			for (DrawCommand& command : entry.DrawCommands)
			{
				command.InstanceOffset = baseOffset + firstInstance;
				finalDrawCmds->Push(command);
			}
			firstInstance = static_cast<uint32>(modelTransforms.Append(entry.ModelTransform));
		}
	}

	for (auto& pair : NonInstanceDraws)
	{
		for (SInstanceEntry& entry : pair.Value)
		{
			DrawCommands* finalDrawCmds = &FinalDrawCommands[pair.Key];
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
		//for (SInstanceEntry& entry : pair.Value)
		//{
		//	entry.ModelTransform.Empty();
		//	entry.DrawCommands.Empty();
		//}
	}

	for (auto& pair : NonInstanceDraws)
	{
		pair.Value.Empty();
		//for (SInstanceEntry& entry : pair.Value)
		//{
		//	entry.ModelTransform.Empty();
		//	entry.DrawCommands.Empty();
		//}
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

