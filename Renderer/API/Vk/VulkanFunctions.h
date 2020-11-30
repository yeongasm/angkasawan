#pragma once
#ifndef LEARNVK_RENDERER_API_VK_VULKAN_FUNCTIONS
#define LEARNVK_RENDERER_API_VK_VULKAN_FUNCTIONS

#include "Vk.h"

namespace vk
{
#define VK_EXPORTED_FUNCTION(Func) extern PFN_##Func Func;
#define VK_GLOBAL_LEVEL_FUNCTION(Func) extern PFN_##Func Func;
#define VK_INSTANCE_LEVEL_FUNCTION(Func) extern PFN_##Func Func;
#define VK_DEVICE_LEVEL_FUNCTION(Func) extern PFN_##Func Func;
#include "VkFuncDecl.inl"
}

#endif // !LEARNVK_RENDERER_API_VK_VULKAN_FUNCTIONS