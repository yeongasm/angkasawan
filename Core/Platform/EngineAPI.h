#pragma once

#ifdef CORE_EXPORT
#define ENGINE_API _declspec(dllexport)
#else
#define ENGINE_API _declspec(dllimport)
#endif

// NOTE:
// Taken from nem0's Lumix Engine.
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
// this is disabled because VS19 16.5.0 has false positives :(
#pragma warning(disable : 4724)
#if _MSC_VER == 1900 
#pragma warning(disable : 4091)
#endif