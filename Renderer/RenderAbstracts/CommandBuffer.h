#pragma once
#ifndef LEANVK_RENDERER_RENDER_ABSTRACTS_H
#define LEANVK_RENDERER_RENDER_ABSTRACTS_H

#include "Library/Math/Mat4.h"
#include "Library/Containers/Map.h"
#include "FrameGraph/FrameGraph.h"

struct Drawable
{
	Handle<HVertexBuffer>	Vbo;
	Handle<HIndexBuffer>	Ebo;
	uint32					VertexCount;
	uint32					IndexCount;
};

struct DrawCommand : Drawable
{
	Handle<RenderPass> PassHandle;
};

// Need to add command buffer recording here ... or should it be done in the renderer.

struct CommandBuffer
{
	using Drawables = Array<Drawable>;

	mat4 Projection;
	mat4 View;
	Map<uint32, Drawables> RenderCommands;
};


#endif // !LEANVK_RENDERER_RENDER_ABSTRACTS_H