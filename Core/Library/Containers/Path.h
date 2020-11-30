#pragma once
#ifndef LEARNVK_LIBRARY_CONTAINERS_PATH
#define LEARNVK_LIBRARY_CONTAINERS_PATH

#include "Library/Memory/Memory.h"
#include "Library/Algorithms/Hash.h"
#include "Library/Templates/Templates.h"
#include "Library/Containers/String.h"

class FilePath
{
public:

	using Type		= int8;
	using ConstType = const int8;

	FilePath() : Path{}, Hash(0), Len(0) {}
	~FilePath() { *Path = '\0'; Hash = 0; Len = 0; }

	FilePath(const char* Path)				{ ConstructPathString(Path); }
	FilePath(const char* Path, uint32 Hash) { ConstructPathString(Path, false); this->Hash = Hash; }

	FilePath(const FilePath& Rhs)	{ *this = Rhs; }
	FilePath(FilePath&& Rhs)		{ *this = Move(Rhs); }

	FilePath& operator=(const FilePath& Rhs)
	{
		if (this != &Rhs)
		{
			CopyFilePath(Rhs);
			Len = Rhs.Len;
			Hash = Rhs.Hash;
		}
		return *this;
	}

	FilePath& operator=(FilePath& Rhs)
	{
		if (this != &Rhs)
		{
			CopyFilePath(Rhs);
			Len = Rhs.Len;
			Hash = Rhs.Hash;
			new (&Rhs) FilePath();
		}
		return *this;
	}

	FilePath& operator=(const char* Path) 
	{ 
		ConstructPathString(Path); 
		return *this;
	}

	bool operator== (const FilePath& Rhs) const { return Hash == Rhs.Hash; }
	bool operator!= (const FilePath& Rhs) const { return Hash != Rhs.Hash; }

	Type*		First	()		  { return Path; }
	ConstType*	First	()	const { return Path; }
	size_t		Length	()	const { return Len; }
	uint32		GetHash	()	const { return Hash; }
	const char* C_Str	()	const { return Path; }
	const char* Filename()	const
	{
		char slash = '/';
		size_t i = 0;
//#if _WIN32
//		slash = '\\';
//#endif
		size_t tail = Len - 1;
		while (tail)
		{
			if (Path[tail] == slash)
			{
				i = tail + 1;
				break;
			}
			tail--;
		}
		return Path + i;
	}

	const char* Extension() const
	{
		char period = '.';
		size_t i = 0;
		size_t tail = Len - 1;
		while (tail)
		{
			if (Path[tail] == period)
			{
				i = tail + 1;
				break;
			}
			tail--;
		}
		return Path + i;
	}

private:
	int8	Path[256];
	uint32	Hash;
	uint32	Len;

	void CopyFilePath(const FilePath& Rhs)
	{
		if (!Rhs.Len) return;

		for (size_t i = 0; i < Rhs.Len; i++)
		{
			Path[i] = Rhs.Path[i];
		}
		Path[Rhs.Len] = '\0';
	}

	void ConstructPathString(const char* Source, bool Hashify = true)
	{
		Len = static_cast<uint32>(strlen(Source));
		VKT_ASSERT(Len < 256);
		for (size_t i = 0; i < Len; i++)
		{
			Path[i] = Source[i];
		}
		Path[Len] = '\0';
		if (Hashify)
		{
			MurmurHash<FilePath> hashFunc;
			Hash = hashFunc(*this);
		}
	}
};

#endif // !LEARNVK_LIBRARY_CONTAINERS_PATH