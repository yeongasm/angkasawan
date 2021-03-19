#pragma once
#ifndef LEARNVK_RENDERER_API_GPU_HANDLES_H
#define LEARNVK_RENDERER_API_GPU_HANDLES_H

#include "SubSystem/Resource/Handle.h"

#define DEFINE_GPU_HANDLE(Type)	\
struct H##Type {}

DEFINE_GPU_HANDLE(Shader);
DEFINE_GPU_HANDLE(Pipeline);
DEFINE_GPU_HANDLE(Image);
DEFINE_GPU_HANDLE(Renderpass);
DEFINE_GPU_HANDLE(Framebuffer);
DEFINE_GPU_HANDLE(Buffer);
DEFINE_GPU_HANDLE(SetPool);
DEFINE_GPU_HANDLE(SetLayout);
DEFINE_GPU_HANDLE(Set);
DEFINE_GPU_HANDLE(Sampler);

#endif // !LEARNVK_RENDERER_API_GPU_HANDLES_H