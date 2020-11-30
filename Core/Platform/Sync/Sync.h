#pragma once
#ifndef LEARNVK_PLATFORM_SYNC_SYNC_H
#define LEARNVK_PLATFORM_SYNC_SYNC_H

#include "../EngineAPI.h"
#include "../Minimal.h"

class alignas(8) ENGINE_API Mutex
{
public:
	Mutex();
	~Mutex();
	DELETE_COPY_AND_MOVE(Mutex)

	void Acquire();
	void Release();
private:
	uint8 Lock[8];
};

class alignas(8) ENGINE_API SpinLock
{
public:

	SpinLock() = default;
	~SpinLock() = default;
	DELETE_COPY_AND_MOVE(SpinLock)

	bool TryAcquire();
	void Acquire();
	void Release();
private:
#if _WIN32
	LONG Atomic;
#else
	// To be implemeted ...
#endif
};

template <typename LockType>
class alignas(8) ENGINE_API ScopedLock
{
private:
	using Lock_t = LockType;
public:

	ScopedLock(Lock_t& Lock) : Lock(Lock) { this->Lock->Acquire(); }
	~ScopedLock() { this->Lock->Release(); }

	DELETE_COPY_AND_MOVE(ScopedLock)
private:
	Lock_t* Lock;
};

class ENGINE_API Semaphore
{
public:

	Semaphore(size_t InitCount, size_t MaxCount);
	~Semaphore();

	DELETE_COPY_AND_MOVE(Semaphore)

	void Signal();
	void Wait();

private:
	void* Sem;
};

#endif // !LEARNVK_PLATFORM_SYNC_SYNC_H