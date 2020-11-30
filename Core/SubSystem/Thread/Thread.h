#pragma once
#ifndef LEARNVK_SUBSYTEM_THREAD_H
#define LEARNVK_SUBSYTEM_THREAD_H

#include "Platform/Minimal.h"
#include "Platform/Sync/Sync.h"
#include "Library/Containers/Queue.h"


using ThreadId = size_t;

struct alignas(64) ENGINE_API Job
{
	using Func = void(*)(void*);
	using Args = void*;

	Func Callback;
	Args Argument;
	Job* Parent;
	ThreadId OwnerThread;
};

using JobFunction = Job::Func;

struct WorkerState
{
	enum
	{
		Idle,
		Running
	};
};

/**
* Make the worker thread struct fill out a single cache line.
*
* NOTE(Ygsm):
* We'll work with a single job queue for now.
* Perhaps in the furture when it is really needed, we'll implement separate queues for different job priorities.
*/
struct alignas(64) WkThread
{
	Queue<Job>	Jobs	= {};
	SpinLock	Lock	= {};
	ThreadId	Id		= {};
	size_t		State	= {};
	struct ThreadPool* System = {};

#if _WIN32
	HANDLE	ThreadHandle = {};
#else
#endif
};

#endif // !LEARNVK_SUBSYTEM_THREAD_H