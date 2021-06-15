#pragma once
#ifndef LEARNVK_SANDBOX_SANDBOX_H
#define LEARNVK_SANDBOX_SANDBOX_H

#ifdef SANDBOX_DLL_EXPORT
#define SANDBOX_API __declspec(dllexport)
#else
#define SANDBOX_API __declspec(dllimport)
#endif // !SANDBOX_DLL_EXPORT

extern "C"
{
	SANDBOX_API void OnDllLoad(void* Param);
	SANDBOX_API void OnDllUnload(void* Param);
}

#endif // !LEARNVK_SANDBOX_SANDBOX_H