#pragma once
#ifndef LEARNVK_TEMPLATES
#define LEARNVK_TEMPLATES

#include <initializer_list>

#if _DEBUG
#include <cassert>
#define VKT_ASSERT(Exp)	assert(Exp)
#else
#define VKT_ASSERT(Exp)
#endif

#ifndef DELETE_COPY_AND_MOVE
#define DELETE_COPY_AND_MOVE(DataType)						\
	DataType(const DataType& Rhs)				= delete;	\
	DataType& operator= (const DataType& Rhs)	= delete;	\
															\
	DataType(DataType&& Rhs)					= delete;	\
	DataType& operator= (DataType&& Rhs)		= delete;
#endif // !DELETE_COPY_AND_MOVE(DataType)


#ifndef DEFAULT_COPY_AND_MOVE
#define DEFAULT_COPY_AND_MOVE(DataType)						\
	DataType(const DataType& Rhs) = default;				\
	DataType& operator= (const DataType& Rhs) = default;	\
															\
	DataType(DataType&& Rhs) = default;						\
	DataType& operator= (DataType&& Rhs) = default;
#endif // !DEFAULT_COPY_AND_MOVE

namespace astl
{

  template<typename T>
  struct TRemoveReference
  {
    using Type = T;
  };


  template <typename T>
  struct TRemoveReference<T&>
  {
    using Type = T;
  };


  template <typename T>
  struct TRemoveReference<T&&>
  {
    using Type = T;
  };


  template <typename T>
  using RemoveReference = typename TRemoveReference<T>::Type;


  template<typename T>
  struct TRemovePointer
  {
    using Type = T;
  };


  template<typename T>
  struct TRemovePointer<T*>
  {
    using Type = T;
  };


  template <typename T>
  using RemovePointer = typename TRemovePointer<T>::Type;


  template <typename A, typename B>
  struct TAreTypesEqual;


  template <typename, typename>
  struct TAreTypesEqual
  {
    enum { Value = false };
  };


  template <typename A>
  struct TAreTypesEqual<A, A>
  {
    enum { Value = true };
  };


  template <typename T>
  struct TIsPointer
  {
    enum { Value = false };
  };


  template <typename T>
  struct TIsPointer<T*>
  {
    enum { Value = true };
  };


  template <typename T>
  struct TIsPointer<const T>
  {
    enum { Value = TIsPointer<T>::Value };
  };


  template <typename T>
  struct TIsPointer<volatile T>
  {
    enum { Value = TIsPointer<T>::Value };
  };


  template <typename T>
  struct TIsPointer<const volatile T>
  {
    enum { Value = TIsPointer<T>::Value };
  };

  template <typename T, T V>
  struct IntegralConstant
  {
    enum Constant { Value = V };
    //static constexpr T Value = V;
  };

  template <typename Type>
  constexpr Type&& Forward(RemoveReference<Type>& Arg) noexcept
  {
    return static_cast<Type&&>(Arg);
  }


  template <typename Type>
  constexpr Type&& Forward(RemoveReference<Type>&& Arg) noexcept
  {
    return static_cast<Type&&>(Arg);
  }


  template <typename Type>
  constexpr RemoveReference<Type>&& Move(Type&& Arg) noexcept
  {
    using CastType = typename TRemoveReference<Type>::Type;
    static_assert(!TAreTypesEqual<CastType&, const CastType&>::Value, "Move semantics called on a const object");
    return static_cast<CastType&&>(Arg);
  }


  template <typename T>
  void Swap(T* First, T* Second)
  {
    T Temp = *First;
    *First = *Second;
    *Second = Temp;
  }


  template <typename T>
  void Swap(T& First, T& Second)
  {
    Swap<T>(&First, &Second);
  }


  template <typename T>
  T& Min(T* First, T* Second)
  {
    return (*First < *Second) ? *First : *Second;
  }


  template <typename T>
  T& Min(T& First, T& Second)
  {
    return Min(&First, &Second);
  }

  template <typename T>
  T Min(T First, T Second)
  {
    return (First < Second) ? First : Second;
  }


  template <typename T>
  T& Max(T* First, T* Second)
  {
    return (*First > *Second) ? *First : *Second;
  }


  template <typename T>
  T& Max(T& First, T& Second)
  {
    return Max(&First, &Second);
  }


  template <typename T>
  T Max(T First, T Second)
  {
    return (First > Second) ? First : Second;
  }


  template <typename T>
  T& Clamp(const T& Value, T* First, T* Second)
  {
    return Max(Min(Value, *Second), *First);
  }


  template <typename T>
  T& Clamp(const T& Value, T& First, T& Second)
  {
    return Clamp(Value, &First, &Second);
  }

  template <typename T>
  bool CompareFloat(T A, T B, T Epsilon)
  {
    return ((A - B) <= Epsilon);
  }

}

#endif // !LEARNVK_TEMPLATES
