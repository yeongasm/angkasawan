#define VOLK_IMPLEMENTATION
#include <volk.h>

#pragma warning(push)
#pragma warning(disable:4005)
#pragma warning(disable:4189)
#pragma warning(disable:4127)
#pragma warning(disable:4324)

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VK_NO_PROTOTYPES
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#pragma warning(pop)