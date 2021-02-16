#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACTS_RENDER_GROUP_H
#define LEARNVK_RENDERER_RENDER_ABSTRACTS_RENDER_GROUP_H

#include "Library/Containers/Array.h"
#include "Library/Allocators/LinearAllocator.h"
#include "RenderPlatform/API.h"
#include "Assets/GPUHandles.h"
#include "Assets/Model.h"

class RenderContext;
class Model;

struct VertexGroupCreateInfo
{
	size_t VertexPoolReserveCount;
	size_t IndexPoolReserveCount;
	uint32 Id;
};

struct VertexGroup
{
	Handle<HBuffer>	Vbo;
	Handle<HBuffer>	Ebo;
	VertexData		VertexPool;
	IndexData		IndexPool;
	uint32			Id;
	bool			IsCompiled;
};

struct VertexGroupManagerConfiguration
{
	size_t GroupReserveCount;
};

class RENDERER_API IRVertexGroupManager
{
private:

	enum ExistEnum : size_t 
	{
		Resource_Not_Exist = -1
	};

	Array<VertexGroup*> VertexGroups;
	RenderContext& Context;
	LinearAllocator& Allocator;

	friend class RenderSystem;

	size_t DoesVertexGroupExist(uint32 Id);

public:

	IRVertexGroupManager(RenderContext& Context, LinearAllocator& InAllocator);
	~IRVertexGroupManager();

	DELETE_COPY_AND_MOVE(IRVertexGroupManager)

	void Initialize(const VertexGroupManagerConfiguration& Config);

	Handle<VertexGroup> CreateVertexGroup			(const VertexGroupCreateInfo& CreateInfo);
	Handle<VertexGroup> GetVertexGroupHandleWithId	(uint32 Id);
	VertexGroup*		GetVertexGroup				(Handle<VertexGroup> Hnd);

	bool				AddModelToVertexGroup		(Model& InModel, Handle<VertexGroup> GroupHandle);

	//bool Build(Handle<VertexGroup> Hnd);
	//bool Destroy(Handle<VertexGroup> Hnd);

	void Build();
	void Destroy();
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACTS_RENDER_GROUP_H