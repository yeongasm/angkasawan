#pragma once
#ifndef LEARNVK_RENDERER_MODEL
#define LEARNVK_RENDERER_MODEL

#include "Library/Containers/Array.h"
#include "Library/Containers/Path.h"
#include "Library/Math/Math.h"
#include "GPUHandles.h"

using vec3 = math::vec3;

struct Vertex
{
	vec3 Position;
	vec3 Color;
	//vec3 Normal;
	//vec3 Tangent;
	//vec3 Bitangent;
	//vec2 TexCoord;
};

using VertexData	= Array<Vertex>;
using IndexData		= Array<uint32>;

struct ModelCreateInfo
{
	String128 Name;
};

struct ModelImportInfo : public ModelCreateInfo
{
	FilePath	Path;
};

/**
* NOTE(Ygsm):
* In the future, the mesh class would not need to be derived from MeshCreateInfo.
* We can immediately create the vertex buffer or push the mesh into a vertex group.
*/
struct MeshCreateInfo
{
	VertexData	Vertices;
	IndexData	Indices;
};

struct VertexGroup;

struct Mesh : public MeshCreateInfo
{
	Handle<VertexGroup> Group;
	uint32 VertexOffset;
	uint32 IndexOffset;
	uint32 NumOfVertices;
	uint32 NumOfIndices;
};

class Model : private Array<Mesh*>
{
private:
	using Super = Array<Mesh*>;
public:
	String128 Name;

	using Super::Push;
	using Super::PopAt;
	using Super::Length;
	using Super::begin;
	using Super::end;
	using Super::operator[];
};

#endif // !LEARNVK_RENDERER_MODEL