#pragma once
#ifndef RENDERER_RENDERER_API_H
#define RENDERER_RENDERER_API_H

#ifdef RHI_EXPORT
#define RHI_API __declspec(dllexport)
#else
#define RHI_API __declspec(dllimport)
#endif

#endif // !RENDERER_RENDERER_API_H
