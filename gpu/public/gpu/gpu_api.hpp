#pragma once
#ifndef GPU_API_H
#define GPU_API_H

#ifdef GPU_SHARED_LIB
#if GPU_EXPORT
#define GPU_API __declspec(dllexport)
#else
#define GPU_API __declspec(dllimport)
#endif
#else
#define GPU_API
#endif

#endif // !GPU_API_H
