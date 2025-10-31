/** @file
  NT RTL Tree Functions Implementation

  Balanced tree (AVL) and generic tree manipulation functions following
  Windows NT RTL conventions.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <ananke/base.h>
#include <ananke/ntrtl.h>

/* ---------------------------------------------------------------
 *  AVL Tree Helper Functions
 * --------------------------------------------------------------- */

/**
  Get the height of a node.

  @param[in] Node  Node to get height of

  @return Height of node (0 for NULL)
**/
static INLINE INTN
GetHeight (
  IN PRTL_AVL_TREE_NODE  Node
  )
{
  if (Node == NULL) {
    return 0;
  }

  INTN LeftHeight = GetHeight(Node->Left);
  INTN RightHeight = GetHeight(Node->Right);

  return 1 + ((LeftHeight > RightHeight) ? LeftHeight : RightHeight);
}

/**
  Update the balance factor of a node.

  @param[in,out] Node  Node to update
**/
static INLINE VOID
UpdateBalance (
  IN OUT PRTL_AVL_TREE_NODE  Node
  )
{
  if (Node != NULL) {
    Node->Balance = GetHeight(Node->Right) - GetHeight(Node->Left);
  }
}

/**
  Perform a left rotation on a node.

  @param[in,out] Node  Node to rotate

  @return New root after rotation
**/
static PRTL_AVL_TREE_NODE
RotateLeft (
  IN OUT PRTL_AVL_TREE_NODE  Node
  )
{
  PRTL_AVL_TREE_NODE NewRoot = Node->Right;
  PRTL_AVL_TREE_NODE Parent = Node->Parent;

  Node->Right = NewRoot->Left;
  if (NewRoot->Left != NULL) {
    NewRoot->Left->Parent = Node;
  }

  NewRoot->Left = Node;
  NewRoot->Parent = Parent;
  Node->Parent = NewRoot;

  UpdateBalance(Node);
  UpdateBalance(NewRoot);

  return NewRoot;
}

/**
  Perform a right rotation on a node.

  @param[in,out] Node  Node to rotate

  @return New root after rotation
**/
static PRTL_AVL_TREE_NODE
RotateRight (
  IN OUT PRTL_AVL_TREE_NODE  Node
  )
{
  PRTL_AVL_TREE_NODE NewRoot = Node->Left;
  PRTL_AVL_TREE_NODE Parent = Node->Parent;

  Node->Left = NewRoot->Right;
  if (NewRoot->Right != NULL) {
    NewRoot->Right->Parent = Node;
  }

  NewRoot->Right = Node;
  NewRoot->Parent = Parent;
  Node->Parent = NewRoot;

  UpdateBalance(Node);
  UpdateBalance(NewRoot);

  return NewRoot;
}

/**
  Rebalance a node if necessary.

  @param[in,out] Node  Node to rebalance

  @return New root after rebalancing
**/
static PRTL_AVL_TREE_NODE
Rebalance (
  IN OUT PRTL_AVL_TREE_NODE  Node
  )
{
  if (Node == NULL) {
    return NULL;
  }

  UpdateBalance(Node);

  if (Node->Balance > 1) {
    // Right-heavy
    if (Node->Right != NULL && Node->Right->Balance < 0) {
      // Right-Left case
      Node->Right = RotateRight(Node->Right);
    }
    // Right-Right case
    return RotateLeft(Node);
  } else if (Node->Balance < -1) {
    // Left-heavy
    if (Node->Left != NULL && Node->Left->Balance > 0) {
      // Left-Right case
      Node->Left = RotateLeft(Node->Left);
    }
    // Left-Left case
    return RotateRight(Node);
  }

  return Node;
}

/**
  Find the minimum node in a subtree.

  @param[in] Node  Root of subtree

  @return Pointer to minimum node
**/
static PRTL_AVL_TREE_NODE
FindMinimumNode (
  IN PRTL_AVL_TREE_NODE  Node
  )
{
  while (Node != NULL && Node->Left != NULL) {
    Node = Node->Left;
  }

  return Node;
}

/**
  Find the maximum node in a subtree.

  @param[in] Node  Root of subtree

  @return Pointer to maximum node
**/
static PRTL_AVL_TREE_NODE
FindMaximumNode (
  IN PRTL_AVL_TREE_NODE  Node
  )
{
  while (Node != NULL && Node->Right != NULL) {
    Node = Node->Right;
  }

  return Node;
}

/* ---------------------------------------------------------------
 *  AVL Tree Initialization
 * --------------------------------------------------------------- */

VOID
EFIAPI
RtlInitializeAvlTree (
  OUT PRTL_AVL_TREE                Tree,
  IN  PRTL_AVL_COMPARE_ROUTINE     CompareRoutine,
  IN  PRTL_AVL_ALLOCATE_ROUTINE    AllocateRoutine OPTIONAL,
  IN  PRTL_AVL_FREE_ROUTINE        FreeRoutine OPTIONAL,
  IN  VOID                         *Context OPTIONAL
  )
{
  Tree->Root = NULL;
  Tree->CompareRoutine = CompareRoutine;
  Tree->AllocateRoutine = AllocateRoutine;
  Tree->FreeRoutine = FreeRoutine;
  Tree->Context = Context;
  Tree->NumberOfNodes = 0;
}

/* ---------------------------------------------------------------
 *  AVL Tree Operations
 * --------------------------------------------------------------- */

/**
  Internal function to insert a node into an AVL tree.

  @param[in,out] Tree     Tree to insert into
  @param[in,out] Root     Current root of subtree
  @param[in]     Node     Node to insert
  @param[in]     Balance  Whether to rebalance

  @return New root of subtree
**/
static PRTL_AVL_TREE_NODE
InsertNode (
  IN OUT PRTL_AVL_TREE       Tree,
  IN OUT PRTL_AVL_TREE_NODE  Root,
  IN     PRTL_AVL_TREE_NODE  Node,
  IN     BOOLEAN             Balance
  )
{
  if (Root == NULL) {
    Tree->NumberOfNodes++;
    return Node;
  }

  INTN CompareResult = Tree->CompareRoutine(Node, Root, Tree->Context);

  if (CompareResult < 0) {
    Root->Left = InsertNode(Tree, Root->Left, Node, Balance);
    if (Root->Left != NULL) {
      Root->Left->Parent = Root;
    }
  } else if (CompareResult > 0) {
    Root->Right = InsertNode(Tree, Root->Right, Node, Balance);
    if (Root->Right != NULL) {
      Root->Right->Parent = Root;
    }
  } else {
    // Duplicate node
    return Root;
  }

  if (Balance) {
    return Rebalance(Root);
  }

  return Root;
}

BOOLEAN
EFIAPI
RtlInsertAvlTreeNode (
  IN OUT PRTL_AVL_TREE       Tree,
  IN     PRTL_AVL_TREE_NODE  Node,
  IN     BOOLEAN             Balance
  )
{
  UINTN OldCount = Tree->NumberOfNodes;

  RtlInitializeAvlTreeNode(Node);
  Tree->Root = InsertNode(Tree, Tree->Root, Node, Balance);

  return Tree->NumberOfNodes > OldCount;
}

/**
  Internal function to remove a node from an AVL tree.

  @param[in,out] Tree     Tree to remove from
  @param[in,out] Root     Current root of subtree
  @param[in]     Node     Node to remove
  @param[in]     Balance  Whether to rebalance

  @return New root of subtree
**/
static PRTL_AVL_TREE_NODE
RemoveNode (
  IN OUT PRTL_AVL_TREE       Tree,
  IN OUT PRTL_AVL_TREE_NODE  Root,
  IN     PRTL_AVL_TREE_NODE  Node,
  IN     BOOLEAN             Balance
  )
{
  if (Root == NULL) {
    return NULL;
  }

  INTN CompareResult = Tree->CompareRoutine(Node, Root, Tree->Context);

  if (CompareResult < 0) {
    Root->Left = RemoveNode(Tree, Root->Left, Node, Balance);
    if (Root->Left != NULL) {
      Root->Left->Parent = Root;
    }
  } else if (CompareResult > 0) {
    Root->Right = RemoveNode(Tree, Root->Right, Node, Balance);
    if (Root->Right != NULL) {
      Root->Right->Parent = Root;
    }
  } else {
    // Found the node to remove
    Tree->NumberOfNodes--;

    if (Root->Left == NULL && Root->Right == NULL) {
      // Leaf node
      return NULL;
    } else if (Root->Left == NULL) {
      // Only right child
      PRTL_AVL_TREE_NODE Temp = Root->Right;
      Temp->Parent = Root->Parent;
      return Temp;
    } else if (Root->Right == NULL) {
      // Only left child
      PRTL_AVL_TREE_NODE Temp = Root->Left;
      Temp->Parent = Root->Parent;
      return Temp;
    } else {
      // Both children - replace with successor
      PRTL_AVL_TREE_NODE Successor = FindMinimumNode(Root->Right);

      // Copy successor's data to root (in real use, you'd swap the nodes)
      // For simplicity, we'll just remove the successor and move its links
      Root->Right = RemoveNode(Tree, Root->Right, Successor, FALSE);
      Tree->NumberOfNodes++;  // Compensate for the decrement in recursive call

      Successor->Left = Root->Left;
      Successor->Right = Root->Right;
      Successor->Parent = Root->Parent;

      if (Successor->Left != NULL) {
        Successor->Left->Parent = Successor;
      }
      if (Successor->Right != NULL) {
        Successor->Right->Parent = Successor;
      }

      Root = Successor;
    }
  }

  if (Balance) {
    return Rebalance(Root);
  }

  return Root;
}

BOOLEAN
EFIAPI
RtlRemoveAvlTreeNode (
  IN OUT PRTL_AVL_TREE       Tree,
  IN     PRTL_AVL_TREE_NODE  Node,
  IN     BOOLEAN             Balance
  )
{
  UINTN OldCount = Tree->NumberOfNodes;

  Tree->Root = RemoveNode(Tree, Tree->Root, Node, Balance);

  return Tree->NumberOfNodes < OldCount;
}

/**
  Internal function to find a node in an AVL tree.

  @param[in] Tree       Tree to search
  @param[in] Root       Current root of subtree
  @param[in] SearchNode Node to search for

  @return Pointer to found node, or NULL if not found
**/
static PRTL_AVL_TREE_NODE
FindNode (
  IN PRTL_AVL_TREE       Tree,
  IN PRTL_AVL_TREE_NODE  Root,
  IN PRTL_AVL_TREE_NODE  SearchNode
  )
{
  if (Root == NULL) {
    return NULL;
  }

  INTN CompareResult = Tree->CompareRoutine(SearchNode, Root, Tree->Context);

  if (CompareResult < 0) {
    return FindNode(Tree, Root->Left, SearchNode);
  } else if (CompareResult > 0) {
    return FindNode(Tree, Root->Right, SearchNode);
  } else {
    return Root;
  }
}

PRTL_AVL_TREE_NODE
EFIAPI
RtlFindAvlTreeNode (
  IN PRTL_AVL_TREE       Tree,
  IN PRTL_AVL_TREE_NODE  SearchNode
  )
{
  return FindNode(Tree, Tree->Root, SearchNode);
}

PRTL_AVL_TREE_NODE
EFIAPI
RtlFindMinimumAvlTreeNode (
  IN PRTL_AVL_TREE  Tree
  )
{
  return FindMinimumNode(Tree->Root);
}

PRTL_AVL_TREE_NODE
EFIAPI
RtlFindMaximumAvlTreeNode (
  IN PRTL_AVL_TREE  Tree
  )
{
  return FindMaximumNode(Tree->Root);
}

PRTL_AVL_TREE_NODE
EFIAPI
RtlAvlTreePredecessor (
  IN PRTL_AVL_TREE_NODE  Node
  )
{
  if (Node == NULL) {
    return NULL;
  }

  // If left subtree exists, predecessor is maximum in left subtree
  if (Node->Left != NULL) {
    return FindMaximumNode(Node->Left);
  }

  // Otherwise, go up until we find a node that is a right child
  PRTL_AVL_TREE_NODE Current = Node;
  PRTL_AVL_TREE_NODE Parent = Node->Parent;

  while (Parent != NULL && Current == Parent->Left) {
    Current = Parent;
    Parent = Parent->Parent;
  }

  return Parent;
}

PRTL_AVL_TREE_NODE
EFIAPI
RtlAvlTreeSuccessor (
  IN PRTL_AVL_TREE_NODE  Node
  )
{
  if (Node == NULL) {
    return NULL;
  }

  // If right subtree exists, successor is minimum in right subtree
  if (Node->Right != NULL) {
    return FindMinimumNode(Node->Right);
  }

  // Otherwise, go up until we find a node that is a left child
  PRTL_AVL_TREE_NODE Current = Node;
  PRTL_AVL_TREE_NODE Parent = Node->Parent;

  while (Parent != NULL && Current == Parent->Right) {
    Current = Parent;
    Parent = Parent->Parent;
  }

  return Parent;
}

/**
  Internal function to clear an AVL tree recursively.

  @param[in,out] Tree  Tree being cleared
  @param[in,out] Node  Current node to clear
**/
static VOID
ClearNode (
  IN OUT PRTL_AVL_TREE       Tree,
  IN OUT PRTL_AVL_TREE_NODE  Node
  )
{
  if (Node == NULL) {
    return;
  }

  ClearNode(Tree, Node->Left);
  ClearNode(Tree, Node->Right);

  if (Tree->FreeRoutine != NULL) {
    Tree->FreeRoutine(Node, Tree->Context);
  }
}

VOID
EFIAPI
RtlClearAvlTree (
  IN OUT PRTL_AVL_TREE  Tree
  )
{
  ClearNode(Tree, Tree->Root);
  Tree->Root = NULL;
  Tree->NumberOfNodes = 0;
}

/* ---------------------------------------------------------------
 *  Generic Tree Operations
 * --------------------------------------------------------------- */

BOOLEAN
EFIAPI
RtlAddGenericTreeNode (
  IN OUT PRTL_GENERIC_TREE       Tree,
  IN OUT PRTL_GENERIC_TREE_NODE  Parent OPTIONAL,
  IN     PRTL_GENERIC_TREE_NODE  Child
  )
{
  Child->Parent = Parent;
  Child->NextSibling = NULL;

  if (Parent == NULL) {
    // Adding as root
    if (Tree->Root != NULL) {
      return FALSE;  // Root already exists
    }
    Tree->Root = Child;
  } else {
    // Adding as child
    if (Parent->FirstChild == NULL) {
      Parent->FirstChild = Child;
    } else {
      // Add to end of sibling list
      PRTL_GENERIC_TREE_NODE Sibling = Parent->FirstChild;
      while (Sibling->NextSibling != NULL) {
        Sibling = Sibling->NextSibling;
      }
      Sibling->NextSibling = Child;
    }
  }

  Tree->NumberOfNodes++;
  return TRUE;
}

BOOLEAN
EFIAPI
RtlRemoveGenericTreeNode (
  IN OUT PRTL_GENERIC_TREE       Tree,
  IN     PRTL_GENERIC_TREE_NODE  Node
  )
{
  if (Node == NULL) {
    return FALSE;
  }

  // Remove from parent's child list or root
  if (Node->Parent == NULL) {
    // Removing root
    if (Tree->Root != Node) {
      return FALSE;
    }
    Tree->Root = NULL;
  } else {
    PRTL_GENERIC_TREE_NODE Parent = Node->Parent;

    if (Parent->FirstChild == Node) {
      Parent->FirstChild = Node->NextSibling;
    } else {
      PRTL_GENERIC_TREE_NODE Sibling = Parent->FirstChild;
      while (Sibling != NULL && Sibling->NextSibling != Node) {
        Sibling = Sibling->NextSibling;
      }

      if (Sibling == NULL) {
        return FALSE;  // Node not found in parent's child list
      }

      Sibling->NextSibling = Node->NextSibling;
    }
  }

  Tree->NumberOfNodes--;
  return TRUE;
}

/**
  Internal function to find a node by data recursively.

  @param[in] Node  Current node to search
  @param[in] Data  Data to search for

  @return Pointer to found node, or NULL if not found
**/
static PRTL_GENERIC_TREE_NODE
FindGenericNode (
  IN PRTL_GENERIC_TREE_NODE  Node,
  IN VOID                    *Data
  )
{
  if (Node == NULL) {
    return NULL;
  }

  if (Node->Data == Data) {
    return Node;
  }

  // Search children
  PRTL_GENERIC_TREE_NODE Child = Node->FirstChild;
  while (Child != NULL) {
    PRTL_GENERIC_TREE_NODE Found = FindGenericNode(Child, Data);
    if (Found != NULL) {
      return Found;
    }
    Child = Child->NextSibling;
  }

  return NULL;
}

PRTL_GENERIC_TREE_NODE
EFIAPI
RtlFindGenericTreeNode (
  IN PRTL_GENERIC_TREE  Tree,
  IN VOID               *Data
  )
{
  return FindGenericNode(Tree->Root, Data);
}

/**
  Internal function to clear a generic tree recursively.

  @param[in,out] Node  Current node to clear
**/
static VOID
ClearGenericNode (
  IN OUT PRTL_GENERIC_TREE_NODE  Node
  )
{
  if (Node == NULL) {
    return;
  }

  // Clear all children
  PRTL_GENERIC_TREE_NODE Child = Node->FirstChild;
  while (Child != NULL) {
    PRTL_GENERIC_TREE_NODE Next = Child->NextSibling;
    ClearGenericNode(Child);
    Child = Next;
  }
}

VOID
EFIAPI
RtlClearGenericTree (
  IN OUT PRTL_GENERIC_TREE  Tree
  )
{
  ClearGenericNode(Tree->Root);
  Tree->Root = NULL;
  Tree->NumberOfNodes = 0;
}
