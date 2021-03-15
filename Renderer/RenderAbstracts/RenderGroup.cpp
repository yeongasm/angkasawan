#include "RenderGroup.h"
#include "API/Context.h"

size_t IRVertexGroupManager::DoesVertexGroupExist(uint32 Id)
{
	VertexGroup* vertexGroup = nullptr;
	for (size_t i = 0; i < VertexGroups.Length(); i++)
	{
		vertexGroup = VertexGroups[i];
		if (vertexGroup->Id == Id) { return i; }
	}
	return Resource_Not_Exist;
}

IRVertexGroupManager::IRVertexGroupManager(RenderContext& Context, LinearAllocator& InAllocator) :
	Context(Context), Allocator(InAllocator), VertexGroups{}
{}

IRVertexGroupManager::~IRVertexGroupManager() {}

//void IRVertexGroupManager::Initialize(const VertexGroupManagerConfiguration& Config)
//{
//	VertexGroups.Reserve(Config.GroupReserveCount);
//}

Handle<VertexGroup> IRVertexGroupManager::CreateVertexGroup(const VertexGroupCreateInfo& CreateInfo)
{
	if (DoesVertexGroupExist(CreateInfo.Id) != Resource_Not_Exist)
	{
		return INVALID_HANDLE;
	}

	VertexGroup* group = reinterpret_cast<VertexGroup*>(Allocator.Malloc(sizeof(VertexGroup)));
	FMemory::InitializeObject(group);
	
	group->Id = CreateInfo.Id;
	group->VertexPool.Reserve(CreateInfo.VertexPoolReserveCount);
	group->IndexPool.Reserve(CreateInfo.IndexPoolReserveCount);

	size_t index = VertexGroups.Push(group);

	return Handle<VertexGroup>(index);
}


Handle<VertexGroup> IRVertexGroupManager::GetVertexGroupHandleWithId(uint32 Id)
{
	return Handle<VertexGroup>(DoesVertexGroupExist(Id));
}


VertexGroup* IRVertexGroupManager::GetVertexGroup(Handle<VertexGroup> Hnd)
{
	VKT_ASSERT(Hnd != INVALID_HANDLE && "Invalid handle provided into function");
	if (Hnd == INVALID_HANDLE)
	{
		return nullptr;
	}
	return VertexGroups[Hnd];
}


bool IRVertexGroupManager::AddModelToVertexGroup(Model& InModel, Handle<VertexGroup> GroupHandle)
{
	VertexGroup* group = GetVertexGroup(GroupHandle);
	
	if (!group)
	{
		return false;
	}

	for (Mesh* mesh : InModel)
	{
		mesh->VertexOffset = static_cast<uint32>(group->VertexPool.Length());
		group->VertexPool.Append(mesh->Vertices.begin(), mesh->Vertices.Length());

		if (mesh->Indices.Length())
		{
			//mesh->IndexOffset = static_cast<uint32>(group->IndexPool.Length());
			for (uint32& index : mesh->Indices)
			{
				index += mesh->VertexOffset;
			}
			group->IndexPool.Append(mesh->Indices.begin(), mesh->Indices.Length());
		}

		mesh->Group = group->Id;
		mesh->NumOfVertices = static_cast<uint32>(mesh->Vertices.Length());
		mesh->NumOfIndices = static_cast<uint32>(mesh->Indices.Length());

		mesh->Vertices.Release();
		mesh->Indices.Release();
	}

	return true;
}


//bool IRVertexGroupManager::Build(Handle<VertexGroup> Hnd)
//{
//	VertexGroup* group = GetVertexGroup(Hnd);
//	if (!group)
//	{
//		return false;
//	}
//
//	Context.NewVertexBuffer(group->Vbo, group->VertexPool.First(), group->VertexPool.Length());
//
//	if (group->IndexPool.Length())
//	{
//		Context.NewIndexBuffer(group->Ebo, group->IndexPool.First(), group->IndexPool.Length());
//	}
//
//	return true;
//}


//bool IRVertexGroupManager::Destroy(Handle<VertexGroup> Hnd)
//{
//	VertexGroup* group = GetVertexGroup(Hnd);
//	if (!group)
//	{
//		return false;
//	}
//
//	Context.DestroyBuffer(group->Vbo);
//	Context.DestroyBuffer(group->Ebo);
//	group->VertexPool.Release();
//	group->IndexPool.Release();
//	VertexGroups.PopAt(Hnd);
//
//	return true;
//}


void IRVertexGroupManager::Build()
{
	for (VertexGroup* group : VertexGroups)
	{
		//Context.NewBuffer(group->Vbo, group->VertexPool.First(), sizeof(Vertex), group->VertexPool.Length());
		if (group->IndexPool.Length())
		{
			//Context.NewIndexBuffer(group->Ebo, group->IndexPool.First(), group->IndexPool.Length());
		}
	}
}


void IRVertexGroupManager::Destroy()
{
	for (VertexGroup* group : VertexGroups)
	{
		Context.DestroyBuffer(group->Vbo);
		if (group->IndexPool.Length())
		{
			Context.DestroyBuffer(group->Ebo);
		}
		group->VertexPool.Release();
		group->IndexPool.Release();
	}
	VertexGroups.Release();
}


//uint32 RenderGroup::GetId() const
//{
//	return Id;
//}
//
//Handle<HBuffer> RenderGroup::GetVertexBuffer() const
//{
//	return Vbo;
//}
//
//Handle<HBuffer> RenderGroup::GetIndexBuffer() const
//{
//	return Ebo;
//}
//
//bool RenderGroup::IsBuilt() const
//{
//	return Built;
//}
//
//bool RenderGroup::Build(bool Release)
//{
//	if (VertexPool.Length())
//	{
//		if (!Context.NewVertexBuffer(Vbo, VertexPool.First(), VertexPool.Length())) { return false; }
//	}
//
//	if (IndexPool.Length())
//	{
//		if (!Context.NewIndexBuffer(Ebo, IndexPool.First(), IndexPool.Length())) { return false; }
//	}
//
//	if (Release)
//	{
//		VertexPool.Release();
//		IndexPool.Release();
//	}
//
//	Built = true;
//
//	return true;
//}
//
//void RenderGroup::Destroy()
//{
//	Context.DestroyBuffer(Vbo);
//
//	if (Ebo != INVALID_HANDLE)
//	{
//		Context.DestroyBuffer(Ebo);
//	}
//}
//
//void RenderGroup::ReserveVertexPool(size_t Capacity)
//{
//	VertexPool.Reserve(Capacity);
//}
//
//void RenderGroup::ReserveIndexPool(size_t Capacity)
//{
//	IndexPool.Reserve(Capacity);
//}
//
//void RenderGroup::AddModelToGroup(Model& InModel, bool Release)
//{
//	for (Mesh* mesh : InModel)
//	{
//		AddMeshToGroup(*mesh, Release);
//	}
//}
//
//void RenderGroup::AddMeshToGroup(Mesh& InMesh, bool Release)
//{
//	if (InMesh.Group != INVALID_HANDLE) { return; }
//
//	InMesh.VertexOffset = static_cast<uint32>(VertexPool.Length());
//	VertexPool.Append(InMesh.Vertices.begin(), InMesh.Vertices.Length());
//
//	if (InMesh.Indices.Length())
//	{
//		InMesh.IndexOffset = static_cast<uint32>(IndexPool.Length());
//		for (uint32& index : InMesh.Indices)
//		{
//			index += InMesh.IndexOffset;
//		}
//		IndexPool.Append(InMesh.Indices.begin(), InMesh.Indices.Length());
//	}
//
//	if (Release)
//	{
//		//InMesh.Vertices.Release();
//		//InMesh.Indices.Release();
//	}
//
//	InMesh.Group = Id;
//}