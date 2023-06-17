#pragma once
#ifndef RENDERER_RENDERER_API_H
#define RENDERER_RENDERER_API_H

#ifdef RENDERER_EXPORT
#define RENDERER_API __declspec(dllexport)
#else
#define RENDERER_API __declspec(dllimport)
#endif

#define EXTERN_C extern "C"

#endif // !RENDERER_RENDERER_API_H
