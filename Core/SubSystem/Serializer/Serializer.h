#pragma once
#ifndef ANGKASA1_CORE_SUB_SYSTEM_SERIALIZER_SERIALIZER_H
#define ANGKASA1_CORE_SUB_SYSTEM_SERIALIZER_SERIALIZER_H

#include "Platform/EngineAPI.h"
#include "Library/Templates/Types.h"
#include "Library/Templates/Templates.h"
#include "Library/Allocators/BaseAllocator.h"

class Ifstream;

struct FloatInt
{
	union
	{
		int32 Int = 0;
		float32 Float;
	};
};

struct FloatInt64
{
	union
	{
		int64 Int = 0;
		float64 Float;
	};
};

struct FileHeader
{
	uint32 Magic;
	uint32 Version;
	uint64 TotalSize;
};

struct ENGINE_API StreamHeader
{
	uint16	Magic;
	uint8	Tag;

	StreamHeader();
	StreamHeader(uint16 Magic, uint8 Tag);
};

class ENGINE_API IMemoryStream
{
protected:
	uint8* Data;
	size_t Pos;
	size_t Capacity;
	
	void OverflowCheck(size_t Size) const;
public:
	IMemoryStream();
	IMemoryStream(uint8* Data, size_t Size);
	~IMemoryStream();

	void Assign(uint8* Data, size_t Size);
	void Rewind();
	void Skip(size_t SizeInBytes);
	void SetPos(size_t PosInBytes);
	size_t Offset() const;
	size_t Size() const;
	uint8* GetData();
	const uint8* GetConstData() const;
};

class ENGINE_API WriteMemoryStream : public IMemoryStream
{
private:
	IAllocator* Allocator = nullptr;
public:
	using IMemoryStream::IMemoryStream;
	WriteMemoryStream(IAllocator& Allocator, size_t Size);
	~WriteMemoryStream();

	void Flush();
	void Write(void* Src, size_t Size);
};

class ENGINE_API ReadMemoryStream : public IMemoryStream
{
private:
	IAllocator* Allocator = nullptr;
public:
	using IMemoryStream::IMemoryStream;
	ReadMemoryStream(IAllocator& Allocator, size_t Size);
	~ReadMemoryStream();

	ReadMemoryStream& operator<< (const Ifstream& Stream);

	void Read(void* Dst, size_t Size);
};

class ENGINE_API Serializer
{
private:
	WriteMemoryStream& Blob;
public:

	Serializer(WriteMemoryStream& Blob);
	~Serializer();

	void Write(void* Data, size_t Size);
	void WriteByte(int8 Val);
	void WriteUnsignedByte(uint8 Val);
	void WriteShort(int16 Val);
	void WriteUnsignedShort(uint16 Val);
	void WriteInt(int32 Val);
	void WriteUnsignedInt(uint32 Val);
	void WriteInt64(int64 Val);
	void WriteUnsignedInt64(int64 Val);
	void WriteFloat(float32 Val);
	void WriteDouble(float64 Val);
};

class ENGINE_API Deserializer
{
private:
	ReadMemoryStream& Blob;
public:

	Deserializer(ReadMemoryStream& Blob);
	~Deserializer();

	void Read(void* Data, size_t Size);
	void ReadByte(int8& Val);
	void ReadUnsigedByte(uint8& Val);
	void ReadShort(int16& Val);
	void ReadUnsignedShort(uint16& Val);
	void ReadInt(int32& Val);
	void ReadUnsignedInt(uint32& Val);
	void ReadInt64(int64& Val);
	void ReadUnsignedInt64(uint64& Val);
	void ReadFloat(float32& Val);
	void ReadDouble(float64& Val);
};

#endif // !ANGKASA1_CORE_SUB_SYSTEM_SERIALIZER_SERIALIZER_H