#pragma once
#ifndef LEARNVK_CORE_LIBRARY_ALGORITHMS_TOKENIZER
#define LEARNVK_CORE_LIBRARY_ALGORITHMS_TOKENIZER
#include "Library/Containers/String.h"
#include "Library/Containers/Array.h"
#include "Library/Templates/Templates.h"

namespace astl
{

  template <typename Type>
  class Token
  {
  private:
    Type* Data;
    size_t	Len;
  public:

    Token() :
      Data(nullptr), Len(0)
    {}

    Token(Type* InData, size_t Size) :
      Data(InData), Len(Size)
    {}

    ~Token()
    {
      Data = nullptr;
      Len = 0;
    }

    Token(const Token& Rhs) { *this = Rhs; }
    Token(Token&& Rhs) { *this = Move(Rhs); }

    Token& operator=(const Token& Rhs)
    {
      if (this != &Rhs)
      {
        Data = Rhs.Data;
        Len = Rhs.Len;
      }
      return *this;
    }

    Token& operator=(Token&& Rhs)
    {
      if (this != &Rhs)
      {
        Data = Rhs.Data;
        Len = Rhs.Len;
        new (&Rhs) Token();
      }
      return *this;
    }

    size_t	Length() { return Len; }
    Type* First() { return Data; }
    Type* Last() { return Data + Len; }
  };

  template <typename TokenType>
  class Tokenizer : public Array<Token<TokenType>>
  {
  private:

    struct TotalChars
    {
      enum
      {
        Value = (1 << 8)
      };
    };

    using Type = Token<TokenType>;
    using EleType = TokenType;
    using Super = Array<Type>;

    size_t SearchBoyerMoore(const EleType* Source, size_t Strlen, const EleType* Pattern, size_t Ptnlen)
    {
      auto find = [](const EleType* Str, size_t Size, EleType Ch) -> size_t
      {
        for (size_t i = Size - 2; i != -1; i--)
        {
          if (Ch == Str[i])
          {
            return i;
          }
        }
        return -1;
      };

      const size_t m = Ptnlen;
      const size_t n = Strlen;

      size_t i, j, lastCh;
      i = j = m - 1;

      while (i < n)
      {
        if (Pattern[j] == Source[i])
        {
          if (!j)
          {
            return i;
          }

          j--;
          i--;
          continue;
        }

        lastCh = find(Pattern, Ptnlen, Source[i]);
        if (lastCh == -1)
        {
          i = i + m;
        }
        else
        {
          i = i + j - lastCh;
        }
        j = m - 1;
      }

      return -1;
    }

  public:

    Tokenizer() : Array<Type>() {}
    ~Tokenizer() {}

    Tokenizer(const Tokenizer& Rhs)
    {
      *this = Rhs;
    }

    Tokenizer(Tokenizer&& Rhs)
    {
      *this = Move(Rhs);
    }

    Tokenizer& operator= (const Tokenizer& Rhs)
    {
      return Super::operator=(Rhs);
    }

    Tokenizer& operator= (Tokenizer&& Rhs)
    {
      return Super::operator=(Move(Rhs));
    }

    void Tokenize(BasicString<EleType>& Source, const EleType* Delimiters)
    {
      TokenizeUntil(Source, Delimiters, nullptr);
    }

    void TokenizeUntil(BasicString<EleType>& Source, const EleType* Delimiters, const EleType* Until)
    {
      size_t start = 0;
      size_t dlen = strlen(Delimiters);
      size_t offset = 0;
      size_t capacity = 0;
      uint64 strLen = static_cast<uint64>(Source.Length());

      size_t untilOffset = strLen;

      if (Until)
      {
        untilOffset = SearchBoyerMoore(Source.First(), strLen, Until, strlen(Until));
      }

      while (strLen >= 0 && start < untilOffset)
      {
        EleType* string = Source.First() + start;
        offset = SearchBoyerMoore(string, strLen, Delimiters, dlen);

        if (start + offset >= untilOffset) { break; }

        if (offset == -1)
        {
          if (strLen)
          {
            Super::Insert(Type(string, strLen));
          }
          break;
        }

        // Prevent repetition.
        if (offset)
        {
          Super::Insert(Type(string, offset));
        }

        capacity = offset + dlen;
        start += capacity;
        strLen -= capacity;
      }
    }

    size_t Find(Type& InToken, const EleType* Delimeters)
    {
      return SearchBoyerMoore(InToken.First(), InToken.Length(), Delimeters, strlen(Delimeters));
    }
  };

  using StringTokenizer = Tokenizer<char>;
  using WStringTokenizer = Tokenizer<wchar_t>;

}

#endif // !LEARNVK_CORE_LIBRARY_ALGORITHMS_TOKENIZER
