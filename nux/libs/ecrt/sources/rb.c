/** @file
  BSD RB Tree Implementation using ANANKE NTRTL AVL Trees

  This provides the BSD rbtree API while using ANANKE NTRTL AVL tree
  primitives underneath.

  Original BSD rbtree from NetBSD - now implemented using NTRTL AVL trees.

  Copyright (C) 2025 ANANKE Project
  Copyright (c) 2001 The NetBSD Foundation, Inc.

  SPDX-License-Identifier: BSD-2-Clause
**/

#if defined(_EC_SOURCE)
#include <inttypes.h>
#include <assert.h>
#include <stdbool.h>
#ifdef RBDEBUG
#define KASSERT(s) assert(s)
#else
#define KASSERT(s) do { } while (0)
#endif
#else
#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <sys/types.h>
#include <stddef.h>
#include <assert.h>
#include <stdbool.h>
#ifdef RBDEBUG
#define KASSERT(s) assert(s)
#else
#define KASSERT(s) do { } while (0)
#endif
#else
#include <lib/libkern/libkern.h>
#define KASSERT(s) /* nothing */
#endif
#endif

#include <rbtree.h>
#include <ananke/base.h>
#include <ananke/ntrtl.h>

/* ---------------------------------------------------------------
 *  Helper Functions
 * --------------------------------------------------------------- */

/**
  Get container object from rb_node pointer.

  @param[in] Tree  RB tree
  @param[in] Node  RB node

  @return Pointer to container object
**/
static INLINE void *
RbNodeToObject (
  const rb_tree_t *Tree,
  rb_node_t *Node
  )
{
    if (Node == NULL) {
        return NULL;
    }
    return (void *)((char *)Node - Tree->rbt_ops->rbto_node_offset);
}

/**
  Get rb_node from container object pointer.

  @param[in] Tree    RB tree
  @param[in] Object  Container object

  @return Pointer to rb_node within object
**/
static INLINE rb_node_t *
ObjectToRbNode (
  const rb_tree_t *Tree,
  const void *Object
  )
{
    if (Object == NULL) {
        return NULL;
    }
    return (rb_node_t *)((char *)Object + Tree->rbt_ops->rbto_node_offset);
}

/**
  Comparison routine wrapper for NTRTL AVL tree.

  Converts NTRTL AVL tree node pointers to rb_node pointers and calls
  the RB tree comparison function.

  @param[in] Node1    First AVL tree node (as rb_node)
  @param[in] Node2    Second AVL tree node (as rb_node)
  @param[in] Context  RB tree pointer

  @return Comparison result
**/
static INTN
EFIAPI
AvlCompareWrapper (
  IN PRTL_AVL_TREE_NODE  Node1,
  IN PRTL_AVL_TREE_NODE  Node2,
  IN VOID                *Context
  )
{
    rb_tree_t *Tree = (rb_tree_t *)Context;
    rb_node_t *RbNode1 = (rb_node_t *)Node1;
    rb_node_t *RbNode2 = (rb_node_t *)Node2;
    void *Object1 = RbNodeToObject(Tree, RbNode1);
    void *Object2 = RbNodeToObject(Tree, RbNode2);

    return (INTN)Tree->rbt_ops->rbto_compare_nodes(
        Tree->rbt_ops->rbto_context,
        Object1,
        Object2
    );
}

/**
  Update RB tree structure from AVL tree.

  Updates the rbt_root and rbt_minmax fields to match the AVL tree state.

  @param[in,out] Tree  RB tree to update
**/
static void
UpdateRbTreeFromAvl (
  rb_tree_t *Tree
  )
{
    PRTL_AVL_TREE_NODE AvlRoot = Tree->avl_tree.Root;
    PRTL_AVL_TREE_NODE AvlMin = RtlFindMinimumAvlTreeNode(&Tree->avl_tree);
    PRTL_AVL_TREE_NODE AvlMax = RtlFindMaximumAvlTreeNode(&Tree->avl_tree);

    Tree->rbt_root = (rb_node_t *)AvlRoot;
    Tree->rbt_minmax[RB_DIR_LEFT] = (rb_node_t *)AvlMin;
    Tree->rbt_minmax[RB_DIR_RIGHT] = (rb_node_t *)AvlMax;
}

/* ---------------------------------------------------------------
 *  RB Tree Functions
 * --------------------------------------------------------------- */

/**
  Initialize an RB tree.

  @param[out] Tree  Tree to initialize
  @param[in]  Ops   Tree operations
**/
void
rb_tree_init (
  rb_tree_t *Tree,
  const rb_tree_ops_t *Ops
  )
{
    Tree->rbt_ops = Ops;
    Tree->rbt_root = NULL;
    Tree->rbt_minmax[RB_DIR_LEFT] = NULL;
    Tree->rbt_minmax[RB_DIR_RIGHT] = NULL;

    /* Initialize the underlying NTRTL AVL tree */
    RtlInitializeAvlTree(
        &Tree->avl_tree,
        AvlCompareWrapper,
        NULL,
        NULL,
        (void *)Tree
    );

#ifdef RBDEBUG
    RB_TAILQ_INIT(&Tree->rbt_nodes);
#endif

#ifdef RBSTATS
    Tree->rbt_count = 0;
    Tree->rbt_insertions = 0;
    Tree->rbt_removals = 0;
    Tree->rbt_insertion_rebalance_calls = 0;
    Tree->rbt_insertion_rebalance_passes = 0;
    Tree->rbt_removal_rebalance_calls = 0;
    Tree->rbt_removal_rebalance_passes = 0;
#endif
}

/**
  Insert a node into an RB tree.

  @param[in,out] Tree  Tree to insert into
  @param[in]     Node  Node to insert

  @return Pointer to existing node if duplicate, NULL if inserted successfully
**/
void *
rb_tree_insert_node (
  rb_tree_t *Tree,
  void *Node
  )
{
    rb_node_t *RbNode = ObjectToRbNode(Tree, Node);
    PRTL_AVL_TREE_NODE AvlNode = (PRTL_AVL_TREE_NODE)RbNode;

    /* Try to insert into AVL tree */
    BOOLEAN Success = RtlInsertAvlTreeNode(&Tree->avl_tree, AvlNode, TRUE);

    if (Success) {
        /* Successfully inserted */
        UpdateRbTreeFromAvl(Tree);
        RB_TAILQ_INSERT_HEAD(&Tree->rbt_nodes, RbNode, rb_link);
        RBSTAT_INC(Tree->rbt_count);
        RBSTAT_INC(Tree->rbt_insertions);
        return NULL;
    } else {
        /* Node already exists - find and return it */
        PRTL_AVL_TREE_NODE Existing = RtlFindAvlTreeNode(&Tree->avl_tree, AvlNode);
        if (Existing != NULL) {
            rb_node_t *ExistingRb = (rb_node_t *)Existing;
            return RbNodeToObject(Tree, ExistingRb);
        }
        return NULL;
    }
}

/**
  Find a node in an RB tree by key.

  @param[in] Tree  Tree to search
  @param[in] Key   Key to search for

  @return Pointer to node if found, NULL otherwise
**/
void *
rb_tree_find_node (
  rb_tree_t *Tree,
  const void *Key
  )
{
    PRTL_AVL_TREE_NODE Current = Tree->avl_tree.Root;

    while (Current != NULL) {
        rb_node_t *RbNode = (rb_node_t *)Current;
        void *Object = RbNodeToObject(Tree, RbNode);
        signed int Cmp = Tree->rbt_ops->rbto_compare_key(
            Tree->rbt_ops->rbto_context,
            Object,
            Key
        );

        if (Cmp == 0) {
            /* Found it */
            return Object;
        } else if (Cmp > 0) {
            /* Node > Key, search left */
            Current = Current->Left;
        } else {
            /* Node < Key, search right */
            Current = Current->Right;
        }
    }

    return NULL;
}

/**
  Find a node greater than or equal to the key.

  @param[in] Tree  Tree to search
  @param[in] Key   Key to search for

  @return Pointer to node if found, NULL otherwise
**/
void *
rb_tree_find_node_geq (
  rb_tree_t *Tree,
  const void *Key
  )
{
    PRTL_AVL_TREE_NODE Current = Tree->avl_tree.Root;
    PRTL_AVL_TREE_NODE Best = NULL;

    while (Current != NULL) {
        rb_node_t *RbNode = (rb_node_t *)Current;
        void *Object = RbNodeToObject(Tree, RbNode);
        signed int Cmp = Tree->rbt_ops->rbto_compare_key(
            Tree->rbt_ops->rbto_context,
            Object,
            Key
        );

        if (Cmp == 0) {
            /* Exact match */
            return Object;
        } else if (Cmp > 0) {
            /* Node > Key, this is a candidate */
            Best = Current;
            Current = Current->Left;
        } else {
            /* Node < Key, search right */
            Current = Current->Right;
        }
    }

    if (Best != NULL) {
        rb_node_t *RbNode = (rb_node_t *)Best;
        return RbNodeToObject(Tree, RbNode);
    }

    return NULL;
}

/**
  Find a node less than or equal to the key.

  @param[in] Tree  Tree to search
  @param[in] Key   Key to search for

  @return Pointer to node if found, NULL otherwise
**/
void *
rb_tree_find_node_leq (
  rb_tree_t *Tree,
  const void *Key
  )
{
    PRTL_AVL_TREE_NODE Current = Tree->avl_tree.Root;
    PRTL_AVL_TREE_NODE Best = NULL;

    while (Current != NULL) {
        rb_node_t *RbNode = (rb_node_t *)Current;
        void *Object = RbNodeToObject(Tree, RbNode);
        signed int Cmp = Tree->rbt_ops->rbto_compare_key(
            Tree->rbt_ops->rbto_context,
            Object,
            Key
        );

        if (Cmp == 0) {
            /* Exact match */
            return Object;
        } else if (Cmp < 0) {
            /* Node < Key, this is a candidate */
            Best = Current;
            Current = Current->Right;
        } else {
            /* Node > Key, search left */
            Current = Current->Left;
        }
    }

    if (Best != NULL) {
        rb_node_t *RbNode = (rb_node_t *)Best;
        return RbNodeToObject(Tree, RbNode);
    }

    return NULL;
}

/**
  Remove a node from an RB tree.

  @param[in,out] Tree  Tree to remove from
  @param[in]     Node  Node to remove
**/
void
rb_tree_remove_node (
  rb_tree_t *Tree,
  void *Node
  )
{
    rb_node_t *RbNode = ObjectToRbNode(Tree, Node);
    PRTL_AVL_TREE_NODE AvlNode = (PRTL_AVL_TREE_NODE)RbNode;

    BOOLEAN Success = RtlRemoveAvlTreeNode(&Tree->avl_tree, AvlNode, TRUE);

    if (Success) {
        UpdateRbTreeFromAvl(Tree);
        RB_TAILQ_REMOVE(&Tree->rbt_nodes, RbNode, rb_link);
        RBSTAT_DEC(Tree->rbt_count);
        RBSTAT_INC(Tree->rbt_removals);
    }
}

/**
  Iterate through an RB tree.

  @param[in] Tree      Tree to iterate
  @param[in] Node      Current node (NULL to start from min/max)
  @param[in] Direction Direction to iterate (RB_DIR_LEFT for ascending, RB_DIR_RIGHT for descending)

  @return Next node in iteration order, or NULL if at end
**/
void *
rb_tree_iterate (
  rb_tree_t *Tree,
  void *Node,
  const unsigned int Direction
  )
{
    PRTL_AVL_TREE_NODE AvlNode;

    if (Node == NULL) {
        /* Start from min or max */
        if (Direction == RB_DIR_LEFT) {
            /* Forward iteration: start from minimum */
            AvlNode = RtlFindMinimumAvlTreeNode(&Tree->avl_tree);
        } else {
            /* Reverse iteration: start from maximum */
            AvlNode = RtlFindMaximumAvlTreeNode(&Tree->avl_tree);
        }
    } else {
        rb_node_t *RbNode = ObjectToRbNode(Tree, Node);
        AvlNode = (PRTL_AVL_TREE_NODE)RbNode;

        /* Get successor or predecessor */
        if (Direction == RB_DIR_RIGHT) {
            /* Forward: get successor */
            AvlNode = RtlAvlTreeSuccessor(AvlNode);
        } else {
            /* Reverse: get predecessor */
            AvlNode = RtlAvlTreePredecessor(AvlNode);
        }
    }

    if (AvlNode != NULL) {
        rb_node_t *RbNode = (rb_node_t *)AvlNode;
        return RbNodeToObject(Tree, RbNode);
    }

    return NULL;
}

#ifdef RBDEBUG
/**
  Check RB tree consistency.

  @param[in] Tree   Tree to check
  @param[in] Panic  Whether to panic on error
**/
void
rb_tree_check (
  const rb_tree_t *Tree,
  bool Panic
  )
{
    /* Basic consistency checks */
    KASSERT(Tree != NULL);

    /* Verify that rbt_root matches AVL tree root */
    KASSERT(Tree->rbt_root == (rb_node_t *)Tree->avl_tree.Root);

    /* Verify min/max */
    PRTL_AVL_TREE_NODE AvlMin = RtlFindMinimumAvlTreeNode((PRTL_AVL_TREE)&Tree->avl_tree);
    PRTL_AVL_TREE_NODE AvlMax = RtlFindMaximumAvlTreeNode((PRTL_AVL_TREE)&Tree->avl_tree);

    KASSERT(Tree->rbt_minmax[RB_DIR_LEFT] == (rb_node_t *)AvlMin);
    KASSERT(Tree->rbt_minmax[RB_DIR_RIGHT] == (rb_node_t *)AvlMax);

    (void)Panic;
}
#endif

#ifdef RBSTATS
/**
  Get RB tree statistics.

  @param[in]  Tree  Tree to get stats for
  @param[out] Retp  Pointer to statistics structure
**/
void
rb_tree_depths (
  const rb_tree_t *Tree,
  size_t *Retp
  )
{
    /* Return the node count */
    *Retp = Tree->avl_tree.NumberOfNodes;
}
#endif
