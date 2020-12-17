#include "RenderGroup.h"

RenderGroup::RenderGroup(RenderContext& Context) :
	Vbo(INVALID_HANDLE),
	Ebo(INVALID_HANDLE),
	VertexPool(),
	IndexPool(),
	Context(Context)
{}

RenderGroup::~RenderGroup() {}

bool RenderGroup::Build()
{
	if (VertexPool.Length())
	{
		if (!Context.NewVertexBuffer(Vbo, VertexPool.First(), VertexPool.Length())) { return false; }
	}

	if (IndexPool.Length())
	{
		if (!Context.NewIndexBuffer(Ebo, IndexPool.First(), IndexPool.Length())) { return false; }
	}

	return true;
}

void RenderGroup::Destroy()
{
	Context.DestroyBuffer(Vbo);

	if (Ebo != INVALID_HANDLE)
	{
		Context.DestroyBuffer(Ebo);
	}
}

void RenderGroup::AddMeshToGroup(Mesh& InMesh)
{
	if (InMesh.Group != INVALID_HANDLE) { return; }

	InMesh.VertexOffset = static_cast<uint32>(VertexPool.Length());
	VertexPool.Append(InMesh.Vertices.begin(), InMesh.Vertices.Length());
	InMesh.Vertices.Release();

	if (InMesh.Indices.Length())
	{
		InMesh.IndexOffset = static_cast<uint32>(IndexPool.Length());
		IndexPool.Append(InMesh.Indices.begin(), InMesh.Indices.Length());
		InMesh.Indices.Release();
	}
}