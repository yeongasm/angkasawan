#ifndef OS_HEADER_PLATFORM_H
#define OS_HEADER_PLATFORM_H

#ifdef _WIN32
#ifndef INCLUDE_WINDOWS_H
#define INCLUDE_WINDOWS_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#endif	//!INCLUDE_WINDOWS_H
#else
// Other OS header files here.
#endif

#endif // !OS_HEADER_PLATFORM_H
