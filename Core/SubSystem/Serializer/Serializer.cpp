#include "Serializer.h"
#include "Library/Memory/Memory.h"

StreamHeader::StreamHeader() :
	Magic(0), Tag(0)
{}

StreamHeader::StreamHeader(uint16 Magic, uint8 Tag) :
	Magic(Magic), Tag(Tag)
{}

void IMemoryStream::OverflowCheck(size_t Size) const
{
	VKT_ASSERT(Pos + Size <= Capacity);
}

IMemoryStream::IMemoryStream() :
	Data(nullptr), Pos(0), Capacity(0)
{}

IMemoryStream::IMemoryStream(uint8 * Data, size_t Size) :
	Data(Data), Pos(0), Capacity(Size)
{}

IMemoryStream::~IMemoryStream() {}

void IMemoryStream::Assign(uint8* Data, size_t Size)
{
	this->Data = Data;
	Capacity = Size;
	Pos = 0;
}

void IMemoryStream::Rewind()
{
	Pos = 0;
}

void IMemoryStream::Skip(size_t SizeInBytes)
{
	Pos += SizeInBytes;
}

void IMemoryStream::SetPos(size_t PosInBytes)
{
	VKT_ASSERT(PosInBytes < Capacity);
	Pos = PosInBytes;
}

size_t IMemoryStream::Offset() const
{
	return Pos;
}

size_t IMemoryStream::Size() const
{
	return Capacity;
}

uint8* IMemoryStream::GetData()
{
	return Data;
}

const uint8* IMemoryStream::GetConstData() const
{
	return Data;
}

WriteMemoryStream::WriteMemoryStream(astl::IAllocator& Allocator, size_t Size) :
	Allocator(&Allocator)
{
	uint8* data = reinterpret_cast<uint8*>(this->Allocator->Malloc(Size));
	Assign(data, Size);
}

WriteMemoryStream::~WriteMemoryStream()
{
	if (Allocator)
	{
		Allocator->Free(GetData());
		new (this) WriteMemoryStream();
	}
}

void WriteMemoryStream::Flush()
{
	astl::IMemory::Memzero(GetData(), Size());
}

void WriteMemoryStream::Write(void* Src, size_t Size)
{
	OverflowCheck(Size);
	astl::IMemory::Memcpy(GetData() + Pos, Src, Size);
	Skip(Size);
}

ReadMemoryStream::ReadMemoryStream(astl::IAllocator& Allocator, size_t Size) :
	Allocator(&Allocator)
{
	uint8* data = reinterpret_cast<uint8*>(this->Allocator->Malloc(Size));
	Assign(Data, Size);
}

ReadMemoryStream::~ReadMemoryStream() 
{
	if (Allocator)
	{
		Allocator->Free(GetData());
		new (this) ReadMemoryStream();
	}
}

ReadMemoryStream& ReadMemoryStream::operator<<(const astl::Ifstream& Stream)
{
	if (Stream.IsValid())
	{
		Stream.Read(GetData(), Size());
	}
	return *this;
}

void ReadMemoryStream::Read(void* Dst, size_t Size)
{
	OverflowCheck(Size);
	astl::IMemory::Memcpy(Dst, GetData() + Pos, Size);
	Skip(Size);
}

Serializer::Serializer(WriteMemoryStream& Blob) :
	Blob(Blob)
{}

Serializer::~Serializer() {}

void Serializer::WriteByte(int8 Val)
{
	Write(&Val, sizeof(int8));
}

void Serializer::WriteUnsignedByte(uint8 Val)
{
	Write(&Val, sizeof(uint8));
}

void Serializer::WriteShort(int16 Val)
{
	Write(&Val, sizeof(int16));
}

void Serializer::WriteUnsignedShort(uint16 Val)
{
	Write(&Val, sizeof(uint16));
}

void Serializer::WriteInt(int32 Val)
{
	Write(&Val, sizeof(int32));
}

void Serializer::WriteUnsignedInt(uint32 Val)
{
	Write(&Val, sizeof(uint32));
}

void Serializer::WriteInt64(int64 Val)
{
	Write(&Val, sizeof(int64));
}

void Serializer::WriteUnsignedInt64(int64 Val)
{
	Write(&Val, sizeof(int64));
}

void Serializer::WriteFloat(float32 Val)
{
	astl::FloatInt src;
	src.Float = Val;
	Write(&src.Int, sizeof(float32));
}

void Serializer::WriteDouble(float64 Val)
{
	astl::FloatInt64 src;
	src.Float = Val;
	Write(&src.Int, sizeof(float64));
}

void Serializer::Write(void* Data, size_t Size)
{
	Blob.Write(Data, Size);
}

Deserializer::Deserializer(ReadMemoryStream& Blob) :
	Blob(Blob)
{}

Deserializer::~Deserializer() {}

void Deserializer::Read(void* Data, size_t Size)
{
	Blob.Read(Data, Size);
}

void Deserializer::ReadByte(int8& Val)
{
	Read(&Val, sizeof(int8));
}

void Deserializer::ReadUnsigedByte(uint8& Val)
{
	Read(&Val, sizeof(uint8));
}

void Deserializer::ReadShort(int16& Val)
{
	Read(&Val, sizeof(int16));
}

void Deserializer::ReadUnsignedShort(uint16& Val)
{
	Read(&Val, sizeof(uint16));
}

void Deserializer::ReadInt(int32& Val)
{
	Read(&Val, sizeof(int32));
}

void Deserializer::ReadUnsignedInt(uint32& Val)
{
	Read(&Val, sizeof(uint32));
}

void Deserializer::ReadInt64(int64& Val)
{
	Read(&Val, sizeof(int64));
}

void Deserializer::ReadUnsignedInt64(uint64& Val)
{
	Read(&Val, sizeof(uint64));
}

void Deserializer::ReadFloat(float32& Val)
{
	astl::FloatInt dst;
	Read(&dst.Int, sizeof(float32));
	Val = dst.Float;
}

void Deserializer::ReadDouble(float64& Val)
{
	astl::FloatInt64 dst;
	Read(&dst.Int, sizeof(float64));
	Val = dst.Float;
}
