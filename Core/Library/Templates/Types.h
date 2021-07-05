#pragma once
#ifndef LEARNVK_PLATFORM_TYPES_ALIAS
#define LEARNVK_PLATFORM_TYPES_ALIAS

#include <cstdint>

using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;

using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

using float32 = float;
using float64 = double;

namespace astl
{

  template <typename T> uint32 SizeOf(const T& Obj) { return static_cast<uint32>(sizeof(T)); }
  template <typename T> uint64 SizeOf(const T& Obj) { return static_cast<uint64>(sizeof(T)); }

  struct FloatInt
  {
    union
    {
      float32 Float;
      int32 Int = 0;
    };
  };

  struct FloatUInt
  {
    union
    {
      float32 Float;
      uint32 Int = 0;
    };
  };

  struct FloatInt64
  {
    union
    {
      float64 Float;
      int64 Int = 0;
    };
  };

  struct FloatUInt64
  {
    union
    {
      float64 Float;
      int64 Int = 0;
    };
  };

}

#endif // !LEARNVK_PLATFORM_TYPES_ALIAS
