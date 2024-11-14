#pragma once
#ifndef SANDBOX_API_H
#define SANDBOX_API_H

#ifdef SANDBOX_EXPORT
#define SANDBOX_API __declspec(dllexport)
#else
#define SANDBOX_API __declspec(dllimport)
#endif

#define EXTERN_C extern "C"

#endif // !SANDBOX_API_H
