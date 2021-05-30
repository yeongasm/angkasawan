#pragma once
#ifndef LEARNVK_RENDERER_RENDER_SYSTEM_H
#define LEARNVK_RENDERER_RENDER_SYSTEM_H

#include "RenderPlatform/API.h"
#include "Engine/Interface.h"
#include "API/Device.h"
#include "API/RendererFlagBits.h"
#include "Library/Containers/Ref.h"
#include "Library/Containers/Node.h"
#include "Library/Math/Matrix.h"
#include "SubSystem/Resource/Handle.h"
#include "RenderAbstracts/Primitives.h"

class IStagingManager;
class IFrameGraph;

struct VertexInformation
{
	uint32 VertexOffset;
	uint32 NumVertices;
};

struct IndexInformation
{
	uint32 IndexOffset;
	uint32 NumIndices;
};

struct DrawInfo
{
	math::mat4			Transform;
	Handle<SRenderPass> Renderpass;
	VertexInformation*	pVertexInformation;
	//uint32				VertexInformationCount;
	IndexInformation*	pIndexInformation;
	uint32				DrawableCount; // Num of meshes in the model.
	//uint32				IndexInformationCount;
	uint32				Id;
	bool				Instanced = true;
};

// NOTE(Ygsm):
// There needs to be a capability for objects to be soft deleted.
class RENDERER_API IRenderSystem : public SystemInterface
{
private:

	friend class IStagingManager;
	friend class IFrameGraph;

	struct DescriptorUpdate
	{
		struct Key
		{
			SDescriptorSet* pSet;
			SDescriptorSetLayout::Binding* pBinding;
			size_t _Key;

			Key();
			Key(SDescriptorSet* InSet, SDescriptorSetLayout::Binding* InBinding, size_t InKey);
			~Key();

			bool operator==(const Key& Rhs) { return _Key == Rhs._Key; }
			bool operator!=(const Key& Rhs) { return _Key != Rhs._Key; }

			/**
			* NOTE(Ygsm):
			* These functions are needed to make hashing custom structs work.
			*/
			size_t* First();
			const size_t* First() const;
			size_t Length() const;
		};

		template <typename UpdateObject>
		using Container = Map<Key, StaticArray<UpdateObject, MAX_DESCRIPTOR_BINDING_UPDATES>, XxHash<Key>>;

		static Container<SImage*> _Images;
		static Container<SMemoryBuffer*> _Buffers;
		
		static size_t CalcKey(Handle<SDescriptorSet> SetHnd, uint32 Binding);
	};
	
	struct DrawManager
	{
		struct DrawCommandRange
		{
			ForwardNode<DrawCommand>* pBegin;
			ForwardNode<DrawCommand>* pEnd;
		};

		struct TransformRange
		{
			ForwardNode<math::mat4>* pBegin;
			ForwardNode<math::mat4>* pEnd;
		};

		struct InstanceEntry
		{
			uint32 Id;
			TransformRange TransformRange;
			DrawCommandRange DrawRange;
		};

		using EntryContainer = StaticArray<InstanceEntry, MAX_INSTANCING_COUNT>;

		static constexpr uint32 MaxDrawablesCount = MAX_DRAWABLE_COUNT;

		// NOTE(Ygsm): Might swap these out for a static array in the future but we'll go with this for now.
		static LinearAllocator _DrawCommandAllocator;
		static LinearAllocator _TransformAllocator;
		static Map<size_t, EntryContainer> _InstancedDraws;
		static Map<size_t, EntryContainer> _NonInstancedDraws;
		static uint32 _NumDrawables;
	};
	
	struct BindableManager
	{
		struct BindableRange
		{
			ForwardNode<SBindable>* pBegin;
			ForwardNode<SBindable>* pEnd;
		};
		// NOTE(Ygsm): Might swap this out for a static array in the future but we'll go with this for now.
		static LinearAllocator _BindableAllocator;
		static Map<size_t, BindableRange> _Bindables;
	};

	using DefaultBuffer = Pair<Handle<SMemoryBuffer>, SMemoryBuffer*>;

	EngineImpl& Engine;
	IRenderDevice* pDevice;
	IDeviceStore* pStore;
	IStagingManager* pStaging;
	IFrameGraph* pFrameGraph;
	Handle<ISystem>	Hnd;

	DefaultBuffer VertexBuffer;
	DefaultBuffer IndexBuffer;
	DefaultBuffer InstanceBuffer;

	Array<DrawCommand> DrawCommands;

	uint32 CalcStrideForFormat(EShaderAttribFormat Format);
	void PreprocessShader(Ref<SShader> pShader);
	//void GetShaderDescriptorLayoutInformation(Ref<SShader> pShader);

	void BindBindable(SBindable& Bindable);
	void BindBindablesForRenderpass(Handle<SRenderPass> Hnd);

	void BindBuffers();
	void BeginRenderPass(Ref<SRenderPass> pRenderPass);
	void EndRenderPass(Ref<SRenderPass> pRenderPass);

	void RecordDrawCommand(const DrawCommand& Command);
	void PrepareDrawCommands();

	void Clear();

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
	bool DescriptorSetUpdateBuffer(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, Handle<SMemoryBuffer> BufferHnd);
	bool DescriptorSetUpdateTexture(Handle<SDescriptorSet> Hnd, uint32 BindingSlot, Handle<SImage> ImageHnd);
	bool BuildDescriptorSet(Handle<SDescriptorSet> Hnd);
	bool DescriptorSetFlushBindingOffset(Handle<SDescriptorSet> Hnd);
	bool DestroyDescriptorSet(Handle<SDescriptorSet> Hnd);

	void UpdateDescriptorSetInQueue();

	Handle<SMemoryBuffer> AllocateNewBuffer(const BufferAllocateInfo& AllocInfo);
	bool BuildBuffer(Handle<SMemoryBuffer> Hnd);
	bool DestroyBuffer(Handle<SMemoryBuffer> Hnd);
	
	Handle<SImage> CreateImage(const ImageCreateInfo& CreateInfo);
	bool BuildImage(Handle<SImage> Hnd);
	bool DestroyImage(Handle<SImage> Hnd);

	IStagingManager& GetStagingManager() const;
	IFrameGraph& GetFrameGraph() const;

	Handle<SShader> CreateShader(const ShaderCreateInfo& CreateInfo);
	bool BuildShader(Handle<SShader> Hnd);
	bool DestroyShader(Handle<SShader> Hnd);

	Handle<SPipeline> CreatePipeline(const PipelineCreateInfo& CreateInfo);
	bool PipelineAddVertexInputBinding(Handle<SPipeline> Hnd, SPipeline::VertexInputBinding Input);
	bool PipelineAddRenderPass(Handle<SPipeline> Hnd, Handle<SRenderPass> RenderPass);
	bool PipelineAddDescriptorSetLayout(Handle<SPipeline> Hnd, Handle<SDescriptorSetLayout> Layout);
	bool BuildGraphicsPipeline(Handle<SPipeline> Hnd);
	bool DestroyPipeline(Handle<SPipeline> Hnd);

	bool BindDescriptorSet(Handle<SDescriptorSet> Hnd, Handle<SRenderPass> RenderpassHnd);
	bool BindPipeline(Handle<SPipeline> Hnd, Handle<SRenderPass> RenderpassHnd);

	bool Draw(const DrawInfo& Info);
	const uint32 GetMaxDrawablesCount() const;
	const uint32 GetDrawableCount() const;

	//bool BlitToDefault(Handle<SImage> Hnd);

	void FlushVertexBuffer();
	void FlushIndexBuffer();
	void FlushInstanceBuffer();

	static Handle<ISystem> GetSystemHandle();
};

namespace ao
{
	RENDERER_API IRenderSystem& FetchRenderSystem();
}

#endif // !LEARNVK_RENDERER_RENDER_SYSTEM_H