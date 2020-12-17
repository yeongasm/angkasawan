#pragma once
#ifndef LEARNVK_RENDERER_MODEL
#define LEARNVK_RENDERER_MODEL

#include "Library/Containers/Array.h"
#include "Library/Math/Vec3.h"
#include "GPUHandles.h"

struct Vertex
{
	vec3 Position;
	vec3 Color;
	//vec3 Normal;
	//vec3 Tangent;
	//vec3 Bitangent;
	//vec2 TexCoord;
};

struct ModelCreateInfo
{
	String128 Name;
};

struct ModelImportInfo : public ModelCreateInfo
{
	FilePath	Path;
};

struct MeshCreateInfo
{
	Array<Vertex> Vertices;
	Array<uint32> Indices;
};

struct Mesh : public MeshCreateInfo
{
	Handle<HVertexBuffer>	Vbo;
	Handle<HIndexBuffer>	Ebo;
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