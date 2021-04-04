#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACT_TEXTURE_MEMORY_H
#define LEARNVK_RENDERER_RENDER_ABSTRACT_TEXTURE_MEMORY_H

#include "Library/Containers/Array.h"
#include "RenderPlatform/API.h"
#include "Primitives.h"

class RENDERER_API IRTextureMemoryManager
{
private:
	Array<SRTextureTransferContext> Transfers;
public:

	IRTextureMemoryManager();
	~IRTextureMemoryManager();

	DELETE_COPY_AND_MOVE(IRTextureMemoryManager)

	bool UploadTexture(Texture* InTexture, void* Data, bool Immediate = true);
	void TransferToGPU();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACT_TEXTURE_MEMORY_H