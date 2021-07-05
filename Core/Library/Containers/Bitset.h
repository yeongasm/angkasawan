#pragma once
#ifndef LEARNVK_CORE_LIBRARY_CONTAINERS_BITSET_H
#define LEARNVK_CORE_LIBRARY_CONTAINERS_BITSET_H

#include "Library/Templates/Types.h"
#include "Library/Templates/Templates.h"

namespace astl
{

  template <typename NumericWidth>
  class BitSet
  {
  private:
    using width_t = NumericWidth;
    width_t Data;
  public:

    BitSet() : Data{ 0 } {}

    BitSet(width_t Val) :
      Data{ Val }
    {}

    ~BitSet() { Data = 0; }

    BitSet(const BitSet& Rhs) { *this = Rhs; }
    BitSet(BitSet&& Rhs) { *this = Move(Rhs); }

    BitSet& operator=(const BitSet& Rhs)
    {
      if (this != &Rhs)
      {
        Data = Rhs.Data;
      }
      return *this;
    }

    BitSet& operator=(BitSet&& Rhs)
    {
      if (this != &Rhs)
      {
        Data = Move(Rhs.Data);
        new (&Rhs) BitSet();
      }
      return *this;
    }

    void        Set       (width_t Bit)         { Data |= (1 << Bit); }
    void        SetBit    (width_t Bit)         { Data |= Bit; }
    void        Remove    (width_t Bit)         { Data &= ~(1 << Bit); }
    void        RemoveBit (width_t Bit)         { Data &= ~Bit; }
    const bool  Has       (width_t Bit)   const { return (Data >> Bit) & 1U; }
    bool        Has       (width_t Bit)         { return (Data >> Bit) & 1U; }
    const bool  HasBit    (width_t Bit)   const { return Data & Bit; }
    bool        HasBit    (width_t Bit)         { return Data & Bit; }
    void        Toggle    (width_t Bit)         { Data ^= (1 << Bit); }
    void        ToggleBit (width_t Bit)         { Data ^= Bit; }
    width_t     Value     ()              const { return Data; }
    void        Assign    (width_t Value)       { Data = Value; }
  };

}

#endif // !LEARNVK_CORE_LIBRARY_CONTAINERS_BITSET_H
