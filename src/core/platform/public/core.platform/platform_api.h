#pragma once
#ifndef PLATFORM_PLATFORM_API_H
#define PLATFORM_PLATFORM_API_H

#ifdef PLATFORM_SHARED_LIB
#ifdef PLATFORM_EXPORT
#define PLATFORM_API __declspec(dllexport)
#else
#define PLATFORM_API __declspec(dllimport)
#endif
#else
#define PLATFORM_API
#endif

#endif // !PLATFORM_PLATFORM_API_H
