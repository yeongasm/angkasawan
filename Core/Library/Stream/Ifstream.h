#pragma once
#ifndef LEARNVK_LIBRARY_STREAM_IFSTREAM
#define LEARNVK_LIBRARY_STREAM_IFSTREAM

#include "Platform/Minimal.h"
#include "Platform/EngineAPI.h"

class ENGINE_API Ifstream
{
public:
	Ifstream();
	~Ifstream();

	Ifstream(const Ifstream& Rhs);
	Ifstream(Ifstream&& Rhs);

	Ifstream& operator=(const Ifstream& Rhs);
	Ifstream& operator=(Ifstream&& Rhs);

	bool	Open	(const char* Path);
	bool	Read	(void* Buf, size_t Size);
	bool	Close	();
	bool	IsValid	() const;
	size_t	Size	() const;
private:
	FILE*	Stream;
	size_t	Len;
};

#endif // !LEARNVK_LIBRARY_STREAM_IFSTREAM