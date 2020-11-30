#pragma once
#ifndef LEARNVK_RENDERER_RENDER_ABSTACTS_DRAW_COMMAND_H
#define LEARNVK_RENDERER_RENDER_ABSTACTS_DRAW_COMMAND_H

#include "FrameGraph/FrameGraph.h"

struct DrawCommand
{
	RenderPass* BindedPass;
};

#endif // !LEARNVK_RENDERER_RENDER_ABSTACTS_DRAW_COMMAND_H