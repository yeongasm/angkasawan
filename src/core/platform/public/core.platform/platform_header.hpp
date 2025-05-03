#pragma once
#ifndef LIB_PLATFORM_HEADER_H
#define LIB_PLATFORM_HEADER_H
/**
* DISCLAIMER!!!
* 
* DO NOT INCLUDE THIS HEADER FILE IN OTHER HEADER FILES. IT SHOULD ONLY BE INCLUDED IN CPP FILES!
* We do not want operating system specific types to leak out.
*/
#ifdef _WIN64
#ifndef INCLUDE_WINDOWS_H
#define INCLUDE_WINDOWS_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#endif	//!INCLUDE_WINDOWS_H
#elif __APPLE__
// TODO(afiq):
#elif __linux__
// TODO(afiq):
#endif

#endif // !LIB_PLATFORM_HEADER_H
