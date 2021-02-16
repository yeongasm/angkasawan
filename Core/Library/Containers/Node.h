#pragma once
#ifndef LEARNVK_NODE
#define LEARNVK_NODE

#include "Library/Templates/Templates.h"

template <typename ElementType>
struct ForwardNode
{
	using Type = ElementType;
	using ConstType = const Type;
	using NodeType = ForwardNode;
	using ConstNodeType = const NodeType;

	ElementType Data;
	ForwardNode* Next;

	ForwardNode() :
		Data(), Next(nullptr)
	{}

	~ForwardNode() {}

	ForwardNode(const ForwardNode& Rhs) { *this = Rhs; }
	ForwardNode(ForwardNode&& Rhs) { *this = Rhs; }

	ForwardNode& operator=(const ForwardNode& Rhs)
	{
		Data = Rhs.Data;
		Next = Rhs.Next;
		return *this;
	}

	ForwardNode& operator=(ForwardNode&& Rhs)
	{
		Data = Move(Rhs.Data);
		Next = Rhs.Next;
		new (&Rhs) ForwardNode();
		return *this;
	}
};

//template <typename ElementType>
//class Node
//{
//private:
//
//
//public:
//};

#endif // !LEARNVK_NODE