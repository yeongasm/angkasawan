#pragma once
#ifndef LEARNVK_RENDERER_PLATFORM_API
#define LEARNVK_RENDERER_PLATFORM_API

#ifdef RENDERER_EXPORT
#define RENDERER_API _declspec(dllexport)
#else
#define RENDERER_API _declspec(dllimport)
#endif // RENDERER_EXPORT


#endif // !LEARNVK_RENDERER_PLATFORM_API