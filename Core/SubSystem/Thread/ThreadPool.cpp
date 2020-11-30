#include <thread>
#include "ThreadPool.h"

#if _WIN32
DWORD WINAPI ThreadPool::ExecuteThread(LPVOID Arg)
{
	WkThread* worker = reinterpret_cast<WkThread*>(Arg);
	
	while (true)
	{
		worker->State = WorkerState::Idle;
		//if (worker->System->AppTerminated)
		//{
		//	break;
		//}

		if (!worker->Jobs.Length())
		{
			// Torn between SwitchToThread and Sleep(1).
			// Need to do more research.
			//::Sleep(1);
			::SwitchToThread();
			continue;
		}

		worker->State = WorkerState::Running;
		worker->Lock.Acquire();
		Job task = worker->Jobs.Front();
		worker->Jobs.Deque();
		worker->Lock.Release();

		// Execute job ...
		task.Callback(task.Argument);
	}

	return 0;
}
#else
#endif