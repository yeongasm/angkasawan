#pragma once
#ifndef LEARNVK_RENDERER_API_GPU_HANDLES_H
#define LEARNVK_RENDERER_API_GPU_HANDLES_H

#include "SubSystem/Resource/Handle.h"

#define DEFINE_GPU_HANDLE(Type)	\
struct H##Type {}

DEFINE_GPU_HANDLE(Shader);
DEFINE_GPU_HANDLE(Pipeline);
DEFINE_GPU_HANDLE(Image);
DEFINE_GPU_HANDLE(FramePass);
DEFINE_GPU_HANDLE(VertexBuffer);
DEFINE_GPU_HANDLE(IndexBuffer);

#endif // !LEARNVK_RENDERER_API_GPU_HANDLES_H