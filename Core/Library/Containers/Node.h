#pragma once
#ifndef LEARNVK_NODE
#define LEARNVK_NODE

#include "Library/Templates/Templates.h"

namespace astl
{

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

  template <typename ElementType>
  struct Node
  {
    using Type = ElementType;
    using ConstType = const Type;
    using NodeType = Node;
    using ConstNodeType = const NodeType;

    Type Data;
    Node* Previous;
    Node* Next;

    Node() :
      Data(), Previous(nullptr), Next(nullptr)
    {}

    ~Node() {}

    Node(const Node& Rhs) { *this = Rhs; }
    Node(Node&& Rhs) { *this = Rhs; }

    Node& operator=(const Node& Rhs)
    {
      Data = Rhs.Data;
      Previous = Rhs.Previous;
      Next = Rhs.Next;
      return *this;
    }

    Node& operator=(Node&& Rhs)
    {
      Data = Move(Rhs.Data);
      Previous = Rhs.Previous;
      Next = Rhs.Next;
      new (&Rhs) Node();
      return *this;
    }
  };

}

#endif // !LEARNVK_NODE
