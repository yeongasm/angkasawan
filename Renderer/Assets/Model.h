#pragma once
#ifndef LEARNVK_RENDERER_MODEL
#define LEARNVK_RENDERER_MODEL

#include "Library/Math/Vec3.h"

struct Vertex
{
	vec3 Position;
	vec3 Normal;
	vec3 Tangent;
	vec3 Bitangent;
	vec2 TexCoord;
};

struct Mesh
{

};

#endif // !LEARNVK_RENDERER_MODEL