#include "Ifstream.h"
#include "Library/Templates/Templates.h"

Ifstream::Ifstream() :
	Stream(nullptr), Len(0)
{}

Ifstream::~Ifstream() { Close(); }

Ifstream::Ifstream(const Ifstream& Rhs) { *this = Rhs; }
Ifstream::Ifstream(Ifstream&& Rhs) { *this = Move(Rhs); }

Ifstream& Ifstream::operator=(const Ifstream& Rhs)
{
	if (this != &Rhs)
	{
		Close();
		Stream = Rhs.Stream;
		Len = Rhs.Len;
	}
	return *this;
}

Ifstream& Ifstream::operator=(Ifstream&& Rhs)
{
	if (this != &Rhs)
	{
		Close();
		Stream = Rhs.Stream;
		Len = Rhs.Len;
		Rhs.Close();
		new (&Rhs) Ifstream();
	}
	return *this;
}

bool Ifstream::Open(const char* Path)
{
	fpos_t pos = {};
	Stream = fopen(Path, "rb");
	if (!Stream)
	{
		return false;
	}
	fseek(Stream, 0, SEEK_END);
	fgetpos(Stream, &pos);
	rewind(Stream);
	Len = static_cast<size_t>(pos);
	return true;
}

bool Ifstream::Read(void* Buf, size_t Size)
{
	if (!Stream)
	{
		return false;
	}
	fread(Buf, sizeof(uint8), Len, Stream);
	return true;
}

bool Ifstream::Close()
{
	if (!Stream) return false;
	fclose(Stream);
	Stream = nullptr;
	return true;
}

bool Ifstream::IsValid() const
{
	return Stream && Len;
}

size_t Ifstream::Size() const
{
	return Len;
}