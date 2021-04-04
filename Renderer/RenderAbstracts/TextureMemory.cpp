#include "TextureMemory.h"
#include "API/Context.h"

IRTextureMemoryManager::IRTextureMemoryManager() :
	Transfers()
{}

IRTextureMemoryManager::~IRTextureMemoryManager()
{
	Transfers.Release();
}

bool IRTextureMemoryManager::UploadTexture(Texture* InTexture, void* Data, bool Immediate)
{
	if (!InTexture) { return false; }

	SRMemoryBuffer buffer = {};
	buffer.Size = InTexture->Size;
	buffer.Locality = Buffer_Locality_Cpu;
	buffer.Type.Set(Buffer_Type_Transfer_Src);

	if (!gpu::CreateBuffer(buffer, Data, InTexture->Size))
	{
		return false;
	}

	auto& transfer = Transfers.Insert(SRTextureTransferContext());

	transfer.Buffer = Move(buffer);
	transfer.Texture = InTexture;

	if (Immediate) { TransferToGPU(); }

	return true;
}

void IRTextureMemoryManager::TransferToGPU()
{
	gpu::BeginTransfer();
	for (SRTextureTransferContext& transfer : Transfers)
	{
		gpu::TransferTexture(transfer);
	}
	gpu::EndTransfer(true);
	gpu::BeginOwnershipTransfer();
	for (SRTextureTransferContext& transfer : Transfers)
	{
		gpu::TransferTextureOwnership(*transfer.Texture);
	}
	gpu::EndOwnershipTransfer();

	for (SRTextureTransferContext& txCtx : Transfers)
	{
		gpu::DestroyBuffer(txCtx.Buffer);
	}
	Transfers.Empty();
}
