#pragma once
#ifndef LEARNVK_RENDERER_RENDER_SYSTEM_H
#define LEARNVK_RENDERER_RENDER_SYSTEM_H

#include "RenderPlatform/API.h"
#include "Engine/Interface.h"
#include "API/Device.h"
#include "SubSystem/Resource/Handle.h"
#include "API/RendererFlagBits.h"

struct SDescriptorPool;
struct SDescriptorPool::Size;
struct SDescriptorSetLayout;
struct SDesriptorSet;
struct SDescriptorSetInstance;
struct DescriptorPoolCreateInfo;
struct DescriptorSetLayoutBindingInfo;
struct STexture;
struct SImageSampler;

class RENDERER_API IRenderSystem : public SystemInterface
{
private:

	struct DescriptorSetUpdateContext
	{
		SDescriptorSet* pSet;
		uint32 Binding;
		SMemoryBuffer** pBuffers;
		uint32 NumBuffers;
		STexture** pTextures;
		uint32 NumTextures;
	};

	using DescriptorUpdateContainer = Array<DescriptorSetUpdateContext>;

	EngineImpl& Engine;
	IRenderDevice* Device;
	IDeviceStore* Store;

	DescriptorUpdateContainer DescriptorUpdates;

	Handle<ISystem>	Hnd;

	bool DescriptorSetUpdateBufferBinding(SDescriptorSet* pSet, uint32 Binding, SMemoryBuffer** pBuffer, size_t Count);
	bool DescriptorSetUpdateImageBinding(SDescriptorSet* pSet, uint32 Binding, STexture** pTexture, size_t Count);

public:

	IRenderSystem(EngineImpl& InEngine, Handle<ISystem> Hnd);
	~IRenderSystem();

	DELETE_COPY_AND_MOVE(IRenderSystem)

	void OnInit			() override;
	void OnUpdate		() override;
	void OnTerminate	() override;

	Handle<SDescriptorPool> CreateDescriptorPool();
	bool DescriptorPoolAddSizeType(Handle<SDescriptorPool> Hnd, SDescriptorPool::Size Type);
	bool BuildDescriptorPool(Handle<SDescriptorPool> Hnd);
	bool DestroyDescriptorPool(Handle<SDescriptorPool> Hnd);

	Handle<SDescriptorSetLayout> CreateDescriptorSetLayout();
	bool DescriptorSetLayoutAddBinding(const DescriptorSetLayoutBindingInfo& BindInfo);
	bool BuildDescriptorSetLayout(Handle<SDescriptorSetLayout> Hnd);
	bool DestroyDescriptorSetLayout(Handle<SDescriptorSetLayout> Hnd);

	Handle<SDescriptorSet> CreateDescriptorSet();
	bool DescriptorSetUpdateData(Handle<SDescriptorSet> Hnd, uint32 Binding, void* Data, size_t Size);
	bool BuildDescriptorSet(Handle<SDescriptorSet> Hnd);
	bool DescriptorSetFlushBindingOffset(Handle<SDescriptorSet> Hnd);
	bool DestroyDescriptorSet(Handle<SDescriptorSet> Hnd);
	
	bool FlushDescriptorSetBindingOffsets();

	static Handle<ISystem> GetSystemHandle();
};

namespace ao
{
	RENDERER_API IRenderSystem& FetchRenderSystem();
}

#endif // !LEARNVK_RENDERER_RENDER_SYSTEM_H