#pragma once
#ifndef LEARNVK_RENDERER_API_GRAPHICS_DRIVER
#define LEARNVK_RENDERER_API_GRAPHICS_DRIVER

#ifdef VULKAN_RENDERER
#include "Vk/VulkanDriver.h"
using GraphicsDriver = vk::VulkanDriver;
#endif

#endif // !LEARNVK_RENDERER_API_GRAPHICS_DRIVER