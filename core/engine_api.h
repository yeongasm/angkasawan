#pragma once
#ifndef CORE_ENGINE_API_H
#define CORE_ENGINE_API_H

#ifdef CORE_EXPORT
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif

#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#pragma warning(disable : 4146)

#endif // !CORE_ENGINE_API_H
