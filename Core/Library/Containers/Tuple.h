#pragma once
#ifndef LEARNVK_TUPLE
#define LEARNVK_TUPLE

#include "Library/Templates/Templates.h"

//template <typename Type>
//struct TupleValue
//{
//	Type Value;
//
//	TupleValue() :
//		Value()
//	{}
//
//	~TupleValue()
//	{}
//
//	TupleValue(const TupleValue& Rhs) { *this = Rhs; }
//	TupleValue(TupleValue&& Rhs) { *this = Move(Rhs); }
//
//	TupleValue& operator=(const TupleValue& Rhs)
//	{
//		if (this != &Rhs)
//		{
//			Value = Rhs.Value;
//		}
//		return *this;
//	}
//
//	TupleValue& operator=(TupleValue&& Rhs)
//	{
//		if (this != &Rhs)
//		{
//			Value = Move(Rhs.Value);
//			new (&Rhs) TupleValue();
//		}
//		return *this;
//	}
//};


template <typename Type>
struct TupleValue
{
	Type Value;

	TupleValue() : Value()	{}
	~TupleValue()			{}

	template <typename Other>
	TupleValue(Other&& Arg) : Value(Forward<Other>(Arg)) {}

	TupleValue(const TupleValue& Rhs)	= default;
	TupleValue(TupleValue&& Rhs)		= default;

	TupleValue& operator=(const TupleValue& Rhs)	= default;
	TupleValue& operator=(TupleValue&& Rhs)			= default;
};


template <typename... Types>
class Tuple;


template <>
class Tuple<>
{
public:

	Tuple()		{}
	~Tuple()	{}

	Tuple(const Tuple& Rhs)		{}
	Tuple(Tuple&& Rhs) noexcept	{}

	Tuple& operator=(const Tuple& Rhs)	= default;
	Tuple& operator=(Tuple&& Rhs)		= default;

	bool Equals(const Tuple& Rhs)	const { return true; }
	bool Less(const Tuple& Rhs)		const { return false; }
};


template <typename This, typename... Rest>
class Tuple<This, Rest...> : public Tuple<Rest...>
{
public:

	using ThisType	= This;
	using Super		= Tuple<Rest...>;

	TupleValue<ThisType> Element;

private:

	Super& GetRest()			 noexcept { return *this; }
	const Super& GetRest() const noexcept { return *this; }

	template<size_t Index, typename First, typename... Others>
	struct GetElementAt
	{
		using Type = typename GetElementAt<Index - 1, Others...>::Type;

		static Type& ValueFor(Tuple<First, Others...>& Tpl)
		{
			return GetElementAt<Index - 1, Others...>::ValueFor(Tpl);
		}
	};

	template<typename First, typename... Others>
	struct GetElementAt<0, First, Others...>
	{
		using Type = First;

		static Type& ValueFor(Tuple<First, Others...>& Tpl)
		{
			return Tpl.Element.Value;
		}
	};

	template <typename Type, typename Tpl>
	struct TupleElement {};

	template <typename ThisTupleType, typename... OtherTupleType>
	struct TupleElement<ThisTupleType, Tuple<OtherTupleType...>> 
	{ 
		using SearchType = Tuple<ThisTupleType, OtherTupleType...>; 
	};

	template <typename Type, typename ThisTupleType, typename... OtherTupleType>
	struct TupleElement<Type, Tuple<ThisTupleType, OtherTupleType...>>
	{
		using SearchType = typename TupleElement<Type, Tuple<OtherTupleType...>>::SearchType;
	};

	//template <typename... Types> struct GetTupleLength {};
	//template <> struct GetTupleLength<> { static size_t Length() { return 0; } };

	//template <typename First, typename... Others>
	//struct GetTupleLength<First, Others...> : public GetTupleLength<Others...>
	//{
	//	using Super = GetTupleLength<Others...>;
	//	static size_t Length() { return 1 + Super::Length(); }
	//};

public:

	Tuple()  = default;
	~Tuple() = default;

	template <typename BaseType, typename... OtherTypes>
	Tuple(BaseType&& BaseArgs, OtherTypes&&... OtherArgs) :
		Super(Forward<OtherTypes>(OtherArgs)...), Element(Forward<BaseType>(BaseArgs))
	{}

	Tuple(const Tuple& Rhs) { *this = Rhs; }
	Tuple(Tuple&& Rhs)		{ *this = Move(Rhs); }

	Tuple& operator=(const Tuple& Rhs)
	{
		Element.Value	= Rhs.Element.Value;
		GetRest()		= Rhs.GetRest();
		return *this;
	}

	Tuple& operator=(Tuple&& Rhs)
	{
		Element.Value	= Forward<ThisType>(Rhs.Element.Value);
		GetRest()		= Forward<Super>(Rhs.GetRest());
		new (&Rhs) Tuple();
		return *this;
	}

	template <typename... Other>
	Tuple& operator=(const Tuple<Other...>& Rhs)
	{
		Element.Value	= Rhs.Element.Value;
		GetRest()		= Rhs.GetRest();
		return *this;
	}

	template <typename... Other>
	Tuple& operator=(Tuple<Other...>&& Rhs)
	{
		Element.Value	= Forward<typename Tuple<Other...>::ThisType>(Rhs.Element.Value);
		GetRest()		= Forward<typename Tuple<Other...>::Super>(Rhs.GetRest());
		return *this;
	}

	template <typename... Other>
	bool Equals(const Tuple<Other...>& Rhs) const 
	{ 
		return Element.Value == Rhs.Element.Value && Super::Equals(Rhs.GetRest()); 
	}
	
	template <typename... Other>
	bool Less(const Tuple<Other...>& Rhs) const 
	{ 
		return Element.Value < Rhs.Element.Value || (!(Rhs.Element.Value < Element.Value) && Super::Less(Rhs.GetRest())); 
	}

	template <size_t Index>
	auto Get() -> decltype(GetElementAt<Index, ThisType, Rest...>::ValueFor(*this))
	{
		using Type = typename GetElementAt<Index, ThisType, Rest...>::Type;
		return static_cast<Type&>(GetElementAt<Index, ThisType, Rest...>::ValueFor(*this));
	}

	template <typename Type>
	Type& Get()
	{
		using TType = typename TupleElement<Type, Tuple<ThisType, Rest...>>::SearchType;
		return static_cast<TType&>(*this).Element.Value;
	}

	size_t Length()
	{
		//return GetTupleLength<ThisType, Rest...>::Length();

		// NOTE: 
		// Discovered parameter pack sizeof operator. Above method aborted!!! xD
		return 1 + sizeof...(Rest);
	}

};


#endif // !LEARNVK_TUPLE