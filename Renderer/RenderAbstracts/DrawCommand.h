#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACTS_DRAWABLE
#define LEARNVK_RENDERER_RENDER_ABSTRACTS_DRAWABLE

#include "Library/Containers/Map.h"
#include "Assets/GPUHandles.h"

class Model;

struct DrawCommand
{
	uint32 Id;
	uint32 NumVertices;
	uint32 NumIndices;
	uint32 VertexOffset;
	uint32 IndexOffset;
	Handle<HBuffer> Vbo;
	Handle<HBuffer> Ebo;
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACTS_DRAWABLE