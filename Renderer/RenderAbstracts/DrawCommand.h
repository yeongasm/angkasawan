#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTRACTS_DRAWABLE
#define LEARNVK_RENDERER_RENDER_ABSTRACTS_DRAWABLE

#include "Library/Containers/Map.h"
#include "Assets/GPUHandles.h"

class RenderContext;
class Model;

struct DrawCommand
{
	uint32 Id;
	uint32 VertexOffset;
	uint32 NumVertices;
	uint32 IndexOffset;
	uint32 NumIndices;
	Handle<HBuffer> Vbo;
	Handle<HBuffer> Ebo;
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTRACTS_DRAWABLE