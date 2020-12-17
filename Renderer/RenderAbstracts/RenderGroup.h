#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACTS_RENDER_GROUP_H
#define LEARNVK_RENDERER_RENDER_ABSTRACTS_RENDER_GROUP_H

#include "Library/Containers/Array.h"
#include "Assets/GPUHandles.h"
#include "Assets/Model.h"
#include "API/Context.h"

class RenderGroup
{
public:

	Handle<HBuffer>		Vbo;
	Handle<HBuffer>		Ebo;

private:

	template <typename Type>
	using DataPool = Array<Type, 16>;

	DataPool<Vertex>	VertexPool;
	DataPool<uint32>	IndexPool;
	RenderContext&		Context;

public:

	RenderGroup(RenderContext& Context);
	~RenderGroup();

	bool Build			();
	void Destroy		();
	void AddMeshToGroup	(Mesh& InMesh);

};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACTS_RENDER_GROUP_H