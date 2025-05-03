#pragma once
#ifndef API_H
#define API_H

#ifdef LIBRARY_SHARED_LIB
#ifdef LIBRARY_EXPORT
#define LIB_API __declspec(dllexport)
#else
#define LIB_API __declspec(dllimport)
#endif
#else
#define LIB_API
#endif

#endif // !API_H
