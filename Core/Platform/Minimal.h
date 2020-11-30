#pragma once
#ifndef LEARNVK_PLATFORM
#define LEARNVK_PLATFORM

/**
* This is where platform specific code will go.
*/
#if _WIN32
#include "OS/Win.h"
#else
#endif

#define FLOAT_EPSILON	0.0001f
#define DOUBLE_EPSILON	0.0001

using WndHandle		= OS::WindowHandle;
using DllModule		= OS::DllModule;
using DllInstance	= OS::DllHandle;

using ApplicationInterface = OS::Interface;

#endif // !LEARNVK_PLATFORM