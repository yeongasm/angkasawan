#pragma once
#ifndef LEARNVK_RENDERER_VK_VK_H
#define LEARNVK_RENDERER_VK_VK_H

#if _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VK_USE_PLATFORM_WIN32_KHR
#else
#endif

#include "Src/vulkan.h"
#include "vk_mem_alloc.h"

#ifndef RENDERER_DEBUG_RENDER_DEVICE
#define RENDERER_DEBUG_RENDER_DEVICE 1
#endif // !RENDERER_DEBUG_RENDER_DEVICE

#endif // !LEARNVK_RENDERER_VK_VK_H