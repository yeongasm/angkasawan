#include "PushConstant.h"
#include "API/Context.h"
#include "PipelineManager.h"

IRPushConstantManager::IRPushConstantManager(LinearAllocator& InAllocator, IRPipelineManager& InPipelineManager) :
	Allocator(InAllocator),
	PipelineManager(InPipelineManager),
	PushConstantContainer()
{}

IRPushConstantManager::~IRPushConstantManager()
{
	PushConstantContainer.Release();
}

Handle<SRPushConstant> IRPushConstantManager::CreateNewPushConstant(const PushConstantCreateInfo& CreateInfo)
{
	SRPushConstant* pushConstant = reinterpret_cast<SRPushConstant*>(Allocator.Malloc(sizeof(SRPushConstant)));
	IMemory::InitializeObject(pushConstant);
	pushConstant->Pipeline = PipelineManager.GetGraphicsPipelineWithHandle(CreateInfo.PipelineHandle);
	size_t idx = PushConstantContainer.Push(pushConstant);
	return Handle<SRPushConstant>(idx);
}

void IRPushConstantManager::BindPushConstant(Handle<SRPushConstant> Hnd)
{
	if (PreviousHandle == Hnd || Hnd == INVALID_HANDLE) { return; }
	SRPushConstant* pushConstant = PushConstantContainer[Hnd];
	if (!pushConstant) { return; }
	gpu::SubmitPushConstant(*pushConstant);
	PreviousHandle = Hnd;
}

bool IRPushConstantManager::UpdatePushConstantData(Handle<SRPushConstant> Hnd, void* Data, size_t Size)
{
	if (Hnd == INVALID_HANDLE || !Data || !Size)
	{
		return false;
	}
	SRPushConstant* pushConstant = PushConstantContainer[Hnd];
	if (!pushConstant) { return false; }

	uint8* dst = pushConstant->Data + pushConstant->Offset;
	IMemory::Memcpy(dst, Data, Size);
	pushConstant->Offset += Size;

	return true;
}
