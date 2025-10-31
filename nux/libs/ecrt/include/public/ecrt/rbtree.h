/** @file
  BSD rbtree.h - Compatible API using ANANKE NTRTL AVL trees

  This header provides the BSD rbtree.h API while using ANANKE NTRTL
  AVL tree primitives underneath.

  Original BSD rbtree.h from NetBSD - now implemented using NTRTL AVL trees.

  Copyright (C) 2025 ANANKE Project
  Copyright (c) 2001 The NetBSD Foundation, Inc.

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ecrt_rbtree_h__
#define _SYS_RBTREE_H_

#if defined(_EC_SOURCE)
#include <cdefs.h>
#include <stddef.h>
#include <inttypes.h>
#include <queue.h>
#ifdef RBDEBUG
#include <stdbool.h>
#endif
#else
#if defined(_KERNEL) || defined(_STANDALONE)
#include <sys/types.h>
#else
#include <types.h>
#include <stdbool.h>
#endif
#include <sys/queue.h>
#include <sys/endian.h>
#endif

#include <ananke/ntrtl.h>

__BEGIN_DECLS

/* ---------------------------------------------------------------
 *  RB Tree Node Structure
 * --------------------------------------------------------------- */

/**
  Red-Black tree node structure.
  Now wraps NTRTL AVL tree node underneath.
**/
typedef struct rb_node {
    struct rb_node *rb_nodes[2];
#define RB_DIR_LEFT      0
#define RB_DIR_RIGHT     1
#define RB_DIR_OTHER     1
#define rb_left          rb_nodes[RB_DIR_LEFT]
#define rb_right         rb_nodes[RB_DIR_RIGHT]

    /**
      rb_info contains flags and parent back pointer.
      We maintain compatibility with BSD RB tree structure.
    **/
    uintptr_t rb_info;
#define RB_FLAG_POSITION 0x2
#define RB_FLAG_RED      0x1
#define RB_FLAG_MASK     (RB_FLAG_POSITION|RB_FLAG_RED)
#define RB_FATHER(rb) \
    ((struct rb_node *)((rb)->rb_info & ~RB_FLAG_MASK))
#define RB_SET_FATHER(rb, father) \
    ((void)((rb)->rb_info = (uintptr_t)(father)|((rb)->rb_info & RB_FLAG_MASK)))

#define RB_SENTINEL_P(rb)           ((rb) == NULL)
#define RB_LEFT_SENTINEL_P(rb)      RB_SENTINEL_P((rb)->rb_left)
#define RB_RIGHT_SENTINEL_P(rb)     RB_SENTINEL_P((rb)->rb_right)
#define RB_FATHER_SENTINEL_P(rb)    RB_SENTINEL_P(RB_FATHER((rb)))
#define RB_CHILDLESS_P(rb) \
    (RB_SENTINEL_P(rb) || (RB_LEFT_SENTINEL_P(rb) && RB_RIGHT_SENTINEL_P(rb)))
#define RB_TWOCHILDREN_P(rb) \
    (!RB_SENTINEL_P(rb) && !RB_LEFT_SENTINEL_P(rb) && !RB_RIGHT_SENTINEL_P(rb))

#define RB_POSITION(rb) \
    (((rb)->rb_info & RB_FLAG_POSITION) ? RB_DIR_RIGHT : RB_DIR_LEFT)
#define RB_RIGHT_P(rb)      (RB_POSITION(rb) == RB_DIR_RIGHT)
#define RB_LEFT_P(rb)       (RB_POSITION(rb) == RB_DIR_LEFT)
#define RB_RED_P(rb)        (!RB_SENTINEL_P(rb) && ((rb)->rb_info & RB_FLAG_RED) != 0)
#define RB_BLACK_P(rb)      (RB_SENTINEL_P(rb) || ((rb)->rb_info & RB_FLAG_RED) == 0)
#define RB_MARK_RED(rb)     ((void)((rb)->rb_info |= RB_FLAG_RED))
#define RB_MARK_BLACK(rb)   ((void)((rb)->rb_info &= ~RB_FLAG_RED))
#define RB_INVERT_COLOR(rb) ((void)((rb)->rb_info ^= RB_FLAG_RED))
#define RB_ROOT_P(rbt, rb)  ((rbt)->rbt_root == (rb))
#define RB_SET_POSITION(rb, position) \
    ((void)((position) ? ((rb)->rb_info |= RB_FLAG_POSITION) : \
    ((rb)->rb_info &= ~RB_FLAG_POSITION)))
#define RB_ZERO_PROPERTIES(rb)  ((void)((rb)->rb_info &= ~RB_FLAG_MASK))
#define RB_COPY_PROPERTIES(dst, src) \
    ((void)((dst)->rb_info ^= ((dst)->rb_info ^ (src)->rb_info) & RB_FLAG_MASK))
#define RB_SWAP_PROPERTIES(a, b) do { \
    uintptr_t xorinfo = ((a)->rb_info ^ (b)->rb_info) & RB_FLAG_MASK; \
    (a)->rb_info ^= xorinfo; \
    (b)->rb_info ^= xorinfo; \
} while (0)

#ifdef RBDEBUG
    TAILQ_ENTRY(rb_node) rb_link;
#endif
} rb_node_t;

/* ---------------------------------------------------------------
 *  RB Tree Operations
 * --------------------------------------------------------------- */

/**
  Comparison function for comparing two nodes.

  @param[in] Context  Optional context
  @param[in] Node1    First node
  @param[in] Node2    Second node

  @retval >0  Node1 > Node2
  @retval 0   Node1 == Node2
  @retval <0  Node1 < Node2
**/
typedef signed int (*rbto_compare_nodes_fn)(void *, const void *, const void *);

/**
  Comparison function for comparing a node with a key.

  @param[in] Context  Optional context
  @param[in] Node     Node to compare
  @param[in] Key      Key to compare against

  @retval >0  Node > Key
  @retval 0   Node == Key
  @retval <0  Node < Key
**/
typedef signed int (*rbto_compare_key_fn)(void *, const void *, const void *);

/**
  RB tree operations structure.
**/
typedef struct {
    rbto_compare_nodes_fn rbto_compare_nodes;
    rbto_compare_key_fn rbto_compare_key;
    size_t rbto_node_offset;
    void *rbto_context;
} rb_tree_ops_t;

/**
  RB tree structure.
  Now wraps NTRTL AVL tree underneath.
**/
typedef struct rb_tree {
    struct rb_node *rbt_root;
    const rb_tree_ops_t *rbt_ops;
    struct rb_node *rbt_minmax[2];

    /* Internal: NTRTL AVL tree */
    RTL_AVL_TREE avl_tree;

#ifdef RBDEBUG
    struct rb_node_qh rbt_nodes;
#endif
#ifdef RBSTATS
    unsigned int rbt_count;
    unsigned int rbt_insertions;
    unsigned int rbt_removals;
    unsigned int rbt_insertion_rebalance_calls;
    unsigned int rbt_insertion_rebalance_passes;
    unsigned int rbt_removal_rebalance_calls;
    unsigned int rbt_removal_rebalance_passes;
#endif
} rb_tree_t;

#ifdef RBSTATS
#define RBSTAT_INC(v)   ((void)((v)++))
#define RBSTAT_DEC(v)   ((void)((v)--))
#else
#define RBSTAT_INC(v)   do { } while (0)
#define RBSTAT_DEC(v)   do { } while (0)
#endif

#ifdef RBDEBUG
TAILQ_HEAD(rb_node_qh, rb_node);

#define RB_TAILQ_REMOVE(a, b, c)        TAILQ_REMOVE(a, b, c)
#define RB_TAILQ_INIT(a)                TAILQ_INIT(a)
#define RB_TAILQ_INSERT_HEAD(a, b, c)   TAILQ_INSERT_HEAD(a, b, c)
#define RB_TAILQ_INSERT_BEFORE(a, b, c) TAILQ_INSERT_BEFORE(a, b, c)
#define RB_TAILQ_INSERT_AFTER(a, b, c, d) TAILQ_INSERT_AFTER(a, b, c, d)
#else
#define RB_TAILQ_REMOVE(a, b, c)        do { } while (0)
#define RB_TAILQ_INIT(a)                do { } while (0)
#define RB_TAILQ_INSERT_HEAD(a, b, c)   do { } while (0)
#define RB_TAILQ_INSERT_BEFORE(a, b, c) do { } while (0)
#define RB_TAILQ_INSERT_AFTER(a, b, c, d) do { } while (0)
#endif

/* ---------------------------------------------------------------
 *  RB Tree Function Prototypes
 * --------------------------------------------------------------- */

/**
  Initialize an RB tree.

  @param[out] Tree  Tree to initialize
  @param[in]  Ops   Tree operations
**/
void rb_tree_init(rb_tree_t *, const rb_tree_ops_t *);

/**
  Insert a node into an RB tree.

  @param[in,out] Tree  Tree to insert into
  @param[in]     Node  Node to insert

  @return Pointer to existing node if duplicate, NULL if inserted successfully
**/
void *rb_tree_insert_node(rb_tree_t *, void *);

/**
  Find a node in an RB tree.

  @param[in] Tree  Tree to search
  @param[in] Key   Key to search for

  @return Pointer to node if found, NULL otherwise
**/
void *rb_tree_find_node(rb_tree_t *, const void *);

/**
  Find a node greater than or equal to the key.

  @param[in] Tree  Tree to search
  @param[in] Key   Key to search for

  @return Pointer to node if found, NULL otherwise
**/
void *rb_tree_find_node_geq(rb_tree_t *, const void *);

/**
  Find a node less than or equal to the key.

  @param[in] Tree  Tree to search
  @param[in] Key   Key to search for

  @return Pointer to node if found, NULL otherwise
**/
void *rb_tree_find_node_leq(rb_tree_t *, const void *);

/**
  Remove a node from an RB tree.

  @param[in,out] Tree  Tree to remove from
  @param[in]     Node  Node to remove
**/
void rb_tree_remove_node(rb_tree_t *, void *);

/**
  Iterate through an RB tree.

  @param[in] Tree      Tree to iterate
  @param[in] Node      Current node (NULL to start from min/max)
  @param[in] Direction Direction to iterate (RB_DIR_LEFT or RB_DIR_RIGHT)

  @return Next node in iteration order, or NULL if at end
**/
void *rb_tree_iterate(rb_tree_t *, void *, const unsigned int);

#ifdef RBDEBUG
/**
  Check RB tree consistency.

  @param[in] Tree   Tree to check
  @param[in] Panic  Whether to panic on error
**/
void rb_tree_check(const rb_tree_t *, bool);
#endif

#ifdef RBSTATS
/**
  Get RB tree statistics.

  @param[in]  Tree  Tree to get stats for
  @param[out] Retp  Pointer to statistics structure
**/
void rb_tree_depths(const rb_tree_t *, size_t *);
#endif

/* Convenience macros */
#define RB_TREE_MIN(T)  rb_tree_iterate((T), NULL, RB_DIR_LEFT)
#define RB_TREE_MAX(T)  rb_tree_iterate((T), NULL, RB_DIR_RIGHT)

#define RB_TREE_FOREACH(N, T) \
    for ((N) = RB_TREE_MIN(T); (N); \
         (N) = rb_tree_iterate((T), (N), RB_DIR_RIGHT))

#define RB_TREE_FOREACH_REVERSE(N, T) \
    for ((N) = RB_TREE_MAX(T); (N); \
         (N) = rb_tree_iterate((T), (N), RB_DIR_LEFT))

__END_DECLS

#endif /* _SYS_RBTREE_H_ */
