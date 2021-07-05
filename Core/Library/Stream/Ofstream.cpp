#include <new>
#include "Ofstream.h"
#include "Library/Templates/Templates.h"

namespace astl
{

  Ofstream::Ofstream() :
    Stream(nullptr)
  {}

  Ofstream::~Ofstream() { Close(); }

  Ofstream::Ofstream(const Ofstream& Rhs) { *this = Rhs; }
  Ofstream::Ofstream(Ofstream&& Rhs) { *this = Move(Rhs); }

  Ofstream& Ofstream::operator=(const Ofstream& Rhs)
  {
    if (this != &Rhs)
    {
      Close();
      Stream = Rhs.Stream;
    }
    return *this;
  }

  Ofstream& Ofstream::operator=(Ofstream&& Rhs)
  {
    if (this != &Rhs)
    {
      Close();
      Stream = Rhs.Stream;
      Rhs.Close();
      new (&Rhs) Ofstream();
    }
    return *this;
  }

  bool Ofstream::Open(const char* Path)
  {
    Stream = fopen(Path, "wb");
    if (!Stream)
    {
      return false;
    }
    return true;
  }

  bool Ofstream::Write(void* Buf, size_t Size)
  {
    if (!Stream)
    {
      return false;
    }
    fwrite(Buf, sizeof(uint8), Size, Stream);
    return true;
  }

  bool Ofstream::Close()
  {
    if (!Stream)
    {
      return false;
    }
    fclose(Stream);
    return true;
  }

  bool Ofstream::IsValid() const
  {
    return Stream;
  }

  bool Ofstream::Flush()
  {
    return fflush(Stream);
  }

}
