#pragma once
#ifndef CORE_HAL_PLATFORM_H
#define CORE_HAL_PLATFORM_H

/**
* NOTE(Afiq):
* The current method of abstracting platform code is non-scalable.
* We might want to break methods inside of PlatformOSInterface to be more use-case specific.
* 
* i.e, Window related functions should be abstracted into platform window etc.
*/

#ifdef _WIN32
#include "win/win_interface.h"
COREBEGIN

using PlatformOSInterface = os::WinOSInterface;
using Dll = os::Dll;

COREEND
#else
#endif


#endif // !CORE_HAL_PLATFORM_H
