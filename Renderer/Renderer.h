#pragma once
#ifndef LEARNVK_RENDERER_RENDER_SYSTEM_H
#define LEARNVK_RENDERER_RENDER_SYSTEM_H

#include "RenderPlatform/API.h"
#include "Engine/Interface.h"
#include "API/Device.h"
#include "SubSystem/Resource/Handle.h"
#include "API/RendererFlagBits.h"
#include "RenderAbstracts/Primitives.h"

class IStagingManager;

class RENDERER_API IRenderSystem : public SystemInterface
{
private:

	friend class IStagingManager;

	struct DescriptorUpdate
	{
		struct Key
		{
			SDescriptorSet* pSet;
			SDescriptorSetLayout::Binding* pBinding;
			size_t _Key;

			Key(SDescriptorSet* InSet, SDescriptorSetLayout::Binding* InBinding, size_t InKey);
			~Key();

			/**
			* NOTE(Ygsm):
			* These functions are needed to make hashing custom structs work.
			*/
			size_t First();
			size_t Length();
		};

		template <typename UpdateObject>
		using Container = Map<Key, StaticArray<UpdateObject, MAX_DESCRIPTOR_BINDING_UPDATES>, XxHash<Key>>;

		static Container<SImage*> _Images;
		static Container<SMemoryBuffer*> _Buffers;
		
		static size_t CalcKey(Handle<SDescriptorSet> SetHnd, uint32 Binding);
	};

	using DefaultBuffer = Pair<Handle<SMemoryBuffer>, SMemoryBuffer*>;

	EngineImpl& Engine;
	IRenderDevice* Device;
	IDeviceStore* Store;
	IStagingManager* Staging;
	Handle<ISystem>	Hnd;

	// NOTE(Ygsm):
	// A better solution would be to allocate memory in pages!
	// Instead of linear memory.
	DefaultBuffer VertexBuffer;
	DefaultBuffer IndexBuffer;
	DefaultBuffer InstanceBuffer;

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

	Handle<SDescriptorSet> CreateDescriptorSet(const DescriptorSetAllocateInfo& AllocInfo);
	bool DescriptorSetUpdateBuffer(Handle<SDescriptorSet> Hnd, uint32 Binding, Handle<SMemoryBuffer> BufferHnd);
	bool DescriptorSetUpdateTexture(Handle<SDescriptorSet> Hnd, uint32 Binding, Handle<SImage> ImageHnd);
	bool BuildDescriptorSet(Handle<SDescriptorSet> Hnd);
	bool DescriptorSetFlushBindingOffset(Handle<SDescriptorSet> Hnd);
	bool DestroyDescriptorSet(Handle<SDescriptorSet> Hnd);

	void UpdateDescriptorSetInQueue();

	Handle<SMemoryBuffer> AllocateNewBuffer(const BufferAllocateInfo& AllocInfo);
	bool BuildBuffer(Handle<SMemoryBuffer> Hnd);
	bool DestroyBuffer(Handle<SMemoryBuffer> Hnd);
	
	IStagingManager& GetStagingManager() const;

	void FlushVertexBuffer();
	void FlushIndexBuffer();
	void FlushInstanceBuffer();

	/**
	* NOTE(Ygsm):
	* Updating descriptor sets should be done at the start of every frame and the update buffer should clear after everything is updated.
	*/

	static Handle<ISystem> GetSystemHandle();
};

namespace ao
{
	RENDERER_API IRenderSystem& FetchRenderSystem();
}

#endif // !LEARNVK_RENDERER_RENDER_SYSTEM_H