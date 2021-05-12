#pragma once
#ifndef LEARNVK_SUBSYSTEM_THREAD_THREADPOOL
#define LEARNVK_SUBSYSTEM_THREAD_THREADPOOL

#include "Library/Containers/Array.h"
#include "Thread.h"


struct ThreadPool
{
	SpinLock		 JobLock = {};
	Array<Job, 10>   JobPool = {};
	Array<WkThread*> Workers = {};
	size_t NextPushedThreadId = {};
	bool AppTerminated = false;

	void Initialize(size_t WorkerCount)
	{
#if _WIN32
		if (!WorkerCount) return;

		Workers.Reserve(WorkerCount);
		for (size_t i = 0; i < WorkerCount; i++)
		{
			WkThread* worker = reinterpret_cast<WkThread*>(IMemory::Malloc(sizeof(WkThread)));
			IMemory::InitializeObject(worker);
			worker->ThreadHandle = ::CreateThread(nullptr, 0, ThreadPool::ExecuteThread, worker, CREATE_SUSPENDED, nullptr);
			if (worker->ThreadHandle)
			{
				worker->Id = i;
				worker->System = this;
				::SetThreadAffinityMask(worker->ThreadHandle, 1ULL << i);
				::SetThreadPriority(worker->ThreadHandle, THREAD_PRIORITY_HIGHEST);
				bool success = ::ResumeThread(worker->ThreadHandle);
				if (success)
				{
					Workers.Push(worker);
					continue;
				}
				CloseHandle(worker->ThreadHandle);
				IMemory::Free(worker);
			}
		}
#else
#endif
	}

	void Shutdown()
	{
#if _WIN32
		for (size_t i = 0; i < Workers.Length(); i++)
		{
			WkThread* worker = Workers[i];
			::TerminateThread(worker->ThreadHandle, 0);
			::CloseHandle(worker->ThreadHandle);
			IMemory::Free(worker);
		}
#else
#endif
	}

	Job& CreateNewJob()
	{
		JobLock.Acquire();
		Job& job = JobPool.Insert(Job());
		JobLock.Release();
		return job;
	}

	void PushJob(Job& Task)
	{
		if (NextPushedThreadId > Workers.Length())
		{
			NextPushedThreadId = 0;
		}

		WkThread* worker = Workers[NextPushedThreadId];
		worker->Lock.Acquire();
		Task.OwnerThread = worker->Id;
		worker->Jobs.Enqueue(Task);
		worker->Lock.Release();
		NextPushedThreadId++;
	}

private:
#if _WIN32
	static DWORD WINAPI ExecuteThread(LPVOID Arg);
#else
#endif
};

#endif // !LEARNVK_SUBSYSTEM_THREAD_THREADPOOL