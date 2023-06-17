#pragma once
#ifndef FOUNDATION_FOUNDATION_API_H
#define FOUNDATION_FOUNDATION_API_H

#ifdef FOUNDATION_EXPORT
#define FOUNDATION_API __declspec(dllexport)
#else
#define FOUNDATION_API __declspec(dllimport)
#endif

#endif // !FOUNDATION_FOUNDATION_API_H
