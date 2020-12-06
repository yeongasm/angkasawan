#pragma once
#ifndef LEANVK_RENDERER_RENDER_ABSTRACTS_H
#define LEANVK_RENDERER_RENDER_ABSTRACTS_H

#include "Library/Math/Mat4.h"
#include "Library/Containers/Map.h"
#include "FrameGraph/FrameGraph.h"
#include "DrawCommand.h"


struct CommandBuffer
{
	mat4				Projection;
	mat4				View;

	// This is wrong! DrawCommands should go by stages depending on the order of the renderpass.
	Array<DrawCommand>	RenderCommands;
};

#endif // !LEANVK_RENDERER_RENDER_ABSTRACTS_H