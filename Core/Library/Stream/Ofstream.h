#pragma once
#ifndef LEARNVK_LIBRARY_STREAM_OFSTREAM
#define LEARNVK_LIBRARY_STREAM_OFSTREAM

#include "Platform/Minimal.h"
#include "Platform/EngineAPI.h"

namespace astl
{

  class ENGINE_API Ofstream
  {
  public:
    Ofstream();
    ~Ofstream();

    Ofstream(const Ofstream& Rhs);
    Ofstream(Ofstream&& Rhs);

    Ofstream& operator=(const Ofstream& Rhs);
    Ofstream& operator=(Ofstream&& Rhs);

    bool	Open(const char* Path);
    bool	Write(void* Buf, size_t Size);
    bool	Close();
    bool	IsValid() const;
    bool	Flush();
  private:
    FILE* Stream;
  };

}

#endif // !LEARNVK_LIBRARY_STREAM_OFSTREAM
