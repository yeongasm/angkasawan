#include <new>
#include "Sync.h"
#include "Library/Memory/Memory.h"

Mutex::Mutex()
{
	IMemory::Memset(Lock, 0, sizeof(Lock));
#if _WIN32
	SRWLOCK* lock = new (&Lock) SRWLOCK();
	InitializeSRWLock(lock);
#else
#endif
}

Mutex::~Mutex()
{
#if _WIN32
	SRWLOCK* lock = reinterpret_cast<SRWLOCK*>(Lock);
	lock->~SRWLOCK();
#else
#endif
}

void Mutex::Acquire()
{
#if _WIN32
	SRWLOCK* lock = reinterpret_cast<SRWLOCK*>(Lock);
	AcquireSRWLockExclusive(lock);
#else
#endif
}

void Mutex::Release()
{
#if _WIN32
	SRWLOCK* lock = reinterpret_cast<SRWLOCK*>(Lock);
	if (TryAcquireSRWLockExclusive(lock))
		ReleaseSRWLockExclusive(lock);
#else
#endif
}

bool SpinLock::TryAcquire()
{
#if _WIN32
	BOOLEAN alreadyLocked = _interlockedbittestandset(&Atomic, 0);
	return alreadyLocked;
#else
#endif
}

void SpinLock::Acquire()
{
	while (TryAcquire())
	{
#if _WIN32
		// SwitchToThread switches execution to another thread for one thread-scheduling time slice.
		SwitchToThread();
#else
#endif
	}
}

void SpinLock::Release()
{
#if _WIN32
	_interlockedbittestandreset(&Atomic, 0);
#else
#endif
}

Semaphore::Semaphore(size_t InitCount, size_t MaxCount)
{
#if _WIN32
	Sem = CreateSemaphoreA(nullptr, static_cast<LONG>(InitCount), static_cast<LONG>(MaxCount), nullptr);
#else
#endif
}

Semaphore::~Semaphore()
{
#if _WIN32
	CloseHandle(Sem);
#else
#endif
}

void Semaphore::Signal()
{
	ReleaseSemaphore(Sem, 1, nullptr);
}

void Semaphore::Wait()
{
	WaitForSingleObject(Sem, INFINITE);
}