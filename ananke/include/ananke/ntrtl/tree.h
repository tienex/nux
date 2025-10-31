/** @file
  NT RTL Tree Functions

  Balanced tree (AVL) manipulation functions following Windows NT RTL conventions.

  Copyright (C) 2025 ANANKE Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ANANKE_NTRTL_TREE_H__
#define __ANANKE_NTRTL_TREE_H__

/* ---------------------------------------------------------------
 *  AVL Tree Types
 * --------------------------------------------------------------- */

/**
  AVL tree node structure.

  This is typically embedded within a larger structure using
  CONTAINING_RECORD to get the containing structure.
**/
typedef struct _RTL_AVL_TREE_NODE {
  struct _RTL_AVL_TREE_NODE  *Left;
  struct _RTL_AVL_TREE_NODE  *Right;
  struct _RTL_AVL_TREE_NODE  *Parent;
  INTN                       Balance;  // Height(Right) - Height(Left)
} RTL_AVL_TREE_NODE, *PRTL_AVL_TREE_NODE;

/**
  Comparison function for AVL tree.

  @param[in] Node1  First node to compare
  @param[in] Node2  Second node to compare
  @param[in] Context Optional context passed to comparison function

  @retval <0  Node1 < Node2
  @retval 0   Node1 == Node2
  @retval >0  Node1 > Node2
**/
typedef
INTN
(EFIAPI *PRTL_AVL_COMPARE_ROUTINE) (
  IN PRTL_AVL_TREE_NODE  Node1,
  IN PRTL_AVL_TREE_NODE  Node2,
  IN VOID                *Context OPTIONAL
  );

/**
  Allocation function for AVL tree nodes.

  @param[in] Size     Size of memory to allocate
  @param[in] Context  Optional context passed to allocate function

  @return Pointer to allocated memory, or NULL on failure
**/
typedef
VOID *
(EFIAPI *PRTL_AVL_ALLOCATE_ROUTINE) (
  IN UINTN  Size,
  IN VOID   *Context OPTIONAL
  );

/**
  Free function for AVL tree nodes.

  @param[in] Buffer   Buffer to free
  @param[in] Context  Optional context passed to free function
**/
typedef
VOID
(EFIAPI *PRTL_AVL_FREE_ROUTINE) (
  IN VOID  *Buffer,
  IN VOID  *Context OPTIONAL
  );

/**
  AVL tree descriptor.
**/
typedef struct _RTL_AVL_TREE {
  PRTL_AVL_TREE_NODE          Root;
  PRTL_AVL_COMPARE_ROUTINE    CompareRoutine;
  PRTL_AVL_ALLOCATE_ROUTINE   AllocateRoutine;
  PRTL_AVL_FREE_ROUTINE       FreeRoutine;
  VOID                        *Context;
  UINTN                       NumberOfNodes;
} RTL_AVL_TREE, *PRTL_AVL_TREE;

/* ---------------------------------------------------------------
 *  Generic Tree Types
 * --------------------------------------------------------------- */

/**
  Generic tree node structure with arbitrary number of children.
**/
typedef struct _RTL_GENERIC_TREE_NODE {
  struct _RTL_GENERIC_TREE_NODE  *Parent;
  struct _RTL_GENERIC_TREE_NODE  *FirstChild;
  struct _RTL_GENERIC_TREE_NODE  *NextSibling;
  VOID                           *Data;
} RTL_GENERIC_TREE_NODE, *PRTL_GENERIC_TREE_NODE;

/**
  Generic tree descriptor.
**/
typedef struct _RTL_GENERIC_TREE {
  PRTL_GENERIC_TREE_NODE  Root;
  UINTN                   NumberOfNodes;
} RTL_GENERIC_TREE, *PRTL_GENERIC_TREE;

/* ---------------------------------------------------------------
 *  AVL Tree Initialization
 * --------------------------------------------------------------- */

/**
  Initialize an AVL tree.

  @param[out] Tree              Tree to initialize
  @param[in]  CompareRoutine    Comparison function
  @param[in]  AllocateRoutine   Allocation function (optional)
  @param[in]  FreeRoutine       Free function (optional)
  @param[in]  Context           Context to pass to routines (optional)
**/
VOID
EFIAPI
RtlInitializeAvlTree (
  OUT PRTL_AVL_TREE                Tree,
  IN  PRTL_AVL_COMPARE_ROUTINE     CompareRoutine,
  IN  PRTL_AVL_ALLOCATE_ROUTINE    AllocateRoutine OPTIONAL,
  IN  PRTL_AVL_FREE_ROUTINE        FreeRoutine OPTIONAL,
  IN  VOID                         *Context OPTIONAL
  );

/**
  Initialize an AVL tree node.

  @param[out] Node  Node to initialize
**/
static INLINE VOID
RtlInitializeAvlTreeNode (
  OUT PRTL_AVL_TREE_NODE  Node
  )
{
  Node->Left = NULL;
  Node->Right = NULL;
  Node->Parent = NULL;
  Node->Balance = 0;
}

/* ---------------------------------------------------------------
 *  AVL Tree Operations
 * --------------------------------------------------------------- */

/**
  Insert a node into an AVL tree.

  @param[in,out] Tree    Tree to insert into
  @param[in]     Node    Node to insert
  @param[in]     Balance Whether to rebalance tree after insertion

  @retval TRUE   Node was inserted
  @retval FALSE  Node already exists or error occurred
**/
BOOLEAN
EFIAPI
RtlInsertAvlTreeNode (
  IN OUT PRTL_AVL_TREE       Tree,
  IN     PRTL_AVL_TREE_NODE  Node,
  IN     BOOLEAN             Balance
  );

/**
  Remove a node from an AVL tree.

  @param[in,out] Tree    Tree to remove from
  @param[in]     Node    Node to remove
  @param[in]     Balance Whether to rebalance tree after removal

  @retval TRUE   Node was removed
  @retval FALSE  Node not found or error occurred
**/
BOOLEAN
EFIAPI
RtlRemoveAvlTreeNode (
  IN OUT PRTL_AVL_TREE       Tree,
  IN     PRTL_AVL_TREE_NODE  Node,
  IN     BOOLEAN             Balance
  );

/**
  Find a node in an AVL tree.

  @param[in] Tree       Tree to search
  @param[in] SearchNode Node to search for

  @return Pointer to found node, or NULL if not found
**/
PRTL_AVL_TREE_NODE
EFIAPI
RtlFindAvlTreeNode (
  IN PRTL_AVL_TREE       Tree,
  IN PRTL_AVL_TREE_NODE  SearchNode
  );

/**
  Find the minimum (leftmost) node in an AVL tree.

  @param[in] Tree  Tree to search

  @return Pointer to minimum node, or NULL if tree is empty
**/
PRTL_AVL_TREE_NODE
EFIAPI
RtlFindMinimumAvlTreeNode (
  IN PRTL_AVL_TREE  Tree
  );

/**
  Find the maximum (rightmost) node in an AVL tree.

  @param[in] Tree  Tree to search

  @return Pointer to maximum node, or NULL if tree is empty
**/
PRTL_AVL_TREE_NODE
EFIAPI
RtlFindMaximumAvlTreeNode (
  IN PRTL_AVL_TREE  Tree
  );

/**
  Find the predecessor (next smaller) node.

  @param[in] Node  Node to find predecessor of

  @return Pointer to predecessor node, or NULL if none
**/
PRTL_AVL_TREE_NODE
EFIAPI
RtlAvlTreePredecessor (
  IN PRTL_AVL_TREE_NODE  Node
  );

/**
  Find the successor (next larger) node.

  @param[in] Node  Node to find successor of

  @return Pointer to successor node, or NULL if none
**/
PRTL_AVL_TREE_NODE
EFIAPI
RtlAvlTreeSuccessor (
  IN PRTL_AVL_TREE_NODE  Node
  );

/**
  Delete all nodes in an AVL tree.

  @param[in,out] Tree  Tree to clear
**/
VOID
EFIAPI
RtlClearAvlTree (
  IN OUT PRTL_AVL_TREE  Tree
  );

/**
  Get the number of nodes in an AVL tree.

  @param[in] Tree  Tree to query

  @return Number of nodes in tree
**/
static INLINE UINTN
RtlNumberOfAvlTreeNodes (
  IN PRTL_AVL_TREE  Tree
  )
{
  return Tree->NumberOfNodes;
}

/**
  Check if an AVL tree is empty.

  @param[in] Tree  Tree to check

  @retval TRUE   Tree is empty
  @retval FALSE  Tree has nodes
**/
static INLINE BOOLEAN
RtlIsAvlTreeEmpty (
  IN PRTL_AVL_TREE  Tree
  )
{
  return Tree->Root == NULL;
}

/* ---------------------------------------------------------------
 *  Generic Tree Operations
 * --------------------------------------------------------------- */

/**
  Initialize a generic tree.

  @param[out] Tree  Tree to initialize
**/
static INLINE VOID
RtlInitializeGenericTree (
  OUT PRTL_GENERIC_TREE  Tree
  )
{
  Tree->Root = NULL;
  Tree->NumberOfNodes = 0;
}

/**
  Initialize a generic tree node.

  @param[out] Node  Node to initialize
  @param[in]  Data  Data to store in node
**/
static INLINE VOID
RtlInitializeGenericTreeNode (
  OUT PRTL_GENERIC_TREE_NODE  Node,
  IN  VOID                    *Data
  )
{
  Node->Parent = NULL;
  Node->FirstChild = NULL;
  Node->NextSibling = NULL;
  Node->Data = Data;
}

/**
  Add a child node to a parent node in a generic tree.

  @param[in,out] Tree    Tree to add to
  @param[in,out] Parent  Parent node (NULL for root)
  @param[in]     Child   Child node to add

  @retval TRUE   Node was added
  @retval FALSE  Error occurred
**/
BOOLEAN
EFIAPI
RtlAddGenericTreeNode (
  IN OUT PRTL_GENERIC_TREE       Tree,
  IN OUT PRTL_GENERIC_TREE_NODE  Parent OPTIONAL,
  IN     PRTL_GENERIC_TREE_NODE  Child
  );

/**
  Remove a node from a generic tree.

  @param[in,out] Tree  Tree to remove from
  @param[in]     Node  Node to remove

  @retval TRUE   Node was removed
  @retval FALSE  Node not found or error occurred
**/
BOOLEAN
EFIAPI
RtlRemoveGenericTreeNode (
  IN OUT PRTL_GENERIC_TREE       Tree,
  IN     PRTL_GENERIC_TREE_NODE  Node
  );

/**
  Find a node in a generic tree by data.

  @param[in] Tree  Tree to search
  @param[in] Data  Data to search for

  @return Pointer to found node, or NULL if not found
**/
PRTL_GENERIC_TREE_NODE
EFIAPI
RtlFindGenericTreeNode (
  IN PRTL_GENERIC_TREE  Tree,
  IN VOID               *Data
  );

/**
  Clear all nodes in a generic tree.

  @param[in,out] Tree  Tree to clear
**/
VOID
EFIAPI
RtlClearGenericTree (
  IN OUT PRTL_GENERIC_TREE  Tree
  );

/**
  Get the number of nodes in a generic tree.

  @param[in] Tree  Tree to query

  @return Number of nodes in tree
**/
static INLINE UINTN
RtlNumberOfGenericTreeNodes (
  IN PRTL_GENERIC_TREE  Tree
  )
{
  return Tree->NumberOfNodes;
}

#endif /* __ANANKE_NTRTL_TREE_H__ */
