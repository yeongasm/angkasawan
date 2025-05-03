#define VOLK_IMPLEMENTATION
#include <volk.h>

#ifndef __clang__
#pragma warning(push)
#pragma warning(disable:4005)
#pragma warning(disable:4100)
#pragma warning(disable:4189)
#pragma warning(disable:4127)
#pragma warning(disable:4324)
#endif 

#define VK_NO_PROTOTYPES
#define VMA_IMPLEMENTATION

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <vk_mem_alloc.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifndef __clang__
#pragma warning(pop)
#endif