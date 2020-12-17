#include "Vk.h"

#define VK_EXPORTED_FUNCTION(Func) PFN_##Func Func;
#define VK_GLOBAL_LEVEL_FUNCTION(Func) PFN_##Func Func;
#define VK_INSTANCE_LEVEL_FUNCTION(Func) PFN_##Func Func;
#define VK_DEVICE_LEVEL_FUNCTION(Func) PFN_##Func Func;
#include "VkFuncDecl.inl"
