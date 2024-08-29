/*
  STREE: A compact bit-tree allocator.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  SPDX-License-Identifier:	GPL2.0+
*/


#define _XOPEN_SOURCE
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <limits.h>

/*
  A simple but non-trivial fast searchable bitmap.

  This code builds higher level maps (LMAPs) of a bitmap as bits are
  modified, to allow a tree-like scan to find a set bit.

  The order of the search is logarithmic, and pointlessly more precisely:

        O(log_W(S))

  where S is the width of the bitmap, and W is the number of nodes per
  level. The number of nodes per level can be set by changing WORDSIZE.
*/


/*
  Configure how to access the bitmap.

  Define *_WORD(ptr, x) to change the default behaviour for accessing
  a bitmap pointer.
*/
#ifndef OR_WORD
#define OR_WORD(_p, _x) (*(_p) |= (_x))
#endif

#ifndef MASK_WORD
#define MASK_WORD(_p, _x) (*(_p) &= (_x))
#endif

#ifndef GET_WORD
#define GET_WORD(_p) (*(_p))
#endif

#ifndef SET_WORD
#define SET_WORD(_p, _x) (*(_p) = (_x))
#endif



/*
  Select number of nodes in the tree.

  A short number of nodes might make the table slighlty smaller, but
  increase the depth of the tree.

  Set STREE_USE_INT, STREE_UES_LONG, STREE_USE_LONG_LONG to use the
  native compiler types as a node. The number of bits in these types
  will describe how many subtrees you can have in a single node.

  Set WORDSIZE to 8 or 16 to have smaller nodes.
*/
//#define STREE_USE_INT
//#define STREE_USE_LONG
#define STREE_USE_LONG_LONG

#ifdef STREE_USE_INT
#define WORDSIZE WORD_BIT
#define ctz(_x)   __builtin_ctz(_x)
#define clz(_x)   __builtin_clz(_x)
#endif

#ifdef STREE_USE_LONG
#define WORDSIZE LONG_BIT
#define ctz(_x)   __builtin_ctzl(_x)
#define clz(_x)   __builtin_clzl(_x)
#endif

#ifdef STREE_USE_LONG_LONG
#define WORDSIZE  64
#define ctz(_x)   __builtin_ctzll(_x)
#define clz(_x)   __builtin_clzll(_x)
#endif

#ifndef WORDSIZE
#define WORDSIZE 16
#endif

#if WORDSIZE==8
#define WORD_T    uint8_t
#define WORDLOG2  3
#define WORDMASK  0x7
#define ctz(_x)   (__builtin_ctz(_x))
#define clz(_x)   (__builtin_clz(_x) - WORD_BIT + 8)
#elif WORDSIZE==16
#define WORD_T    uint16_t
#define WORDLOG2  4
#define WORDMASK  0xf
#define ctz(_x)   (__builtin_ctz(_x))
#define clz(_x)   (__builtin_clz(_x) - WORD_BIT + 16)
#elif WORDSIZE==32
#define WORD_T    uint32_t
#define WORDLOG2  5
#define WORDMASK  0x1f
#elif WORDSIZE==64
#define WORD_T    uint64_t
#define WORDLOG2  6
#define WORDMASK  0x3f
#else
#error "Unsupported WORDSIZE"
#endif

#define CEIL_DIV(_n,_d) (((_n) + (_d) - 1)/(_d))
#define LOGWORD(_x) (((_x) + (WORDLOG2-1)) / WORDLOG2)

//#define clz(_x) __builtin_clz(_x)
//#define ctz(_x) __builtin_ctz(_x)

/*
  The size of a searcheable bitmap.

  This is the result of the sum:
      L
     ---
     >   W^i
     ---i
      0

   where L is the log_W(O) (i.e., W^L = O).

   It is the sum of all LMAPS + bitmap.
*/
#define STREE_SIZE(_o) CEIL_DIV((1LL << (_o)) - 1, WORDSIZE - 1)

/*
  Given a number of objects, find the order needed
  to create an STREE to manage them.
*/
static inline size_t
stree_order (size_t n)
{
  int r = LONG_BIT - 1 - __builtin_clzl ((long) n);
  printf ("%d -1 - __clzl(%d)[%d] = %d\n", WORD_BIT, n, __builtin_clzl (n),
	  r);

  /* Is the number a power of two? If not, add 1 */
  r += __builtin_popcount (n) > 1 ? 1 : 0;

  return r;
}


static inline size_t
stree_lmap_off (unsigned o, unsigned l)
{
  /*
     This mysterious code is the result of this sum:
     l
     ---   2^o
     >    -----
     ---i  W^i
     0

     for a generic 'o' which is not a power of 64, and where 'W' is the
     word width.

     This tells where the LMAP for level l starts. This series in
     particular returns the bit offset, which is why we divide by
     WORDLOG2 before returning.
   */
  unsigned y = l - 1;
  size_t c = 1 << (o - WORDLOG2 * y);
  size_t r = c * ((1 << WORDLOG2 * l) - 1) / (WORDSIZE - 1);
  return r >> WORDLOG2;
}

/* Get level L bitmap of the search tree. */
static inline WORD_T *
stree_lmap (WORD_T * stree, unsigned o, unsigned l)
{
  return stree + stree_lmap_off (o, l);
}

/* Get bit offset of an lmap of level L for address A. */
static inline size_t
lmap_bitoff (unsigned l, unsigned a)
{
  return ((size_t) a >> WORDLOG2 * l);
}

static inline bool
set_bit (WORD_T * map, size_t bitaddr)
{
  WORD_T old;
  size_t off = bitaddr >> WORDLOG2;
  size_t bit = bitaddr & WORDMASK;

  old = GET_WORD (map + off);
  OR_WORD (map + off, ((WORD_T) 1 << bit));

  /* Return true if this is NOT the first bit set. */
  return !!old;
}

static inline bool
clr_bit (WORD_T * map, size_t bitaddr)
{
  size_t off = bitaddr >> WORDLOG2;
  size_t bit = bitaddr & WORDMASK;

  MASK_WORD (map + off, ~((WORD_T) 1 << bit));

  /* Return true if word still has bit set. */
  return !!GET_WORD (map + off);
}

static inline int
get_bit (WORD_T * map, size_t bitaddr)
{
  size_t off = bitaddr >> WORDLOG2;
  size_t bit = bitaddr & WORDMASK;

  return !!(GET_WORD (map + off) & ((WORD_T) 1 << bit));
}

static inline void
stree_setbit (WORD_T * stree, unsigned o, size_t bitaddr)
{
  int l;

  for (l = 0; l <= LOGWORD (o) - 1; l++)
    {
      WORD_T *lmap = stree_lmap (stree, o, l);
      size_t laddr = lmap_bitoff (l, bitaddr);

      if (set_bit (lmap, laddr))
	{
	  /* Other bits were set before. Don't set upper levels. */
	  break;
	}
    }
}

static inline void
stree_clrbit (WORD_T * stree, unsigned o, size_t bitaddr)
{
  int l;

  for (l = 0; l <= LOGWORD (o) - 1; l++)
    {
      WORD_T *lmap = stree_lmap (stree, o, l);
      size_t laddr = lmap_bitoff (l, bitaddr);

      if (clr_bit (lmap, laddr))
	{
	  /* Other bits are set. Don't clear upper levels. */
	  break;
	}
    }
}

/*
  Find a set bit.

  LOW=1 will search the lowest address available,
  LOW=0 will search the highest address available.

  CLEAR=1 will clear the bit as it is found.

  Return the bit address.
*/
static inline long
stree_bitsearch (WORD_T * stree, unsigned o, int low)
{
  int l;
  size_t laddr;

  laddr = 0;
  for (l = LOGWORD (o) - 1; l >= 0; l -= 1)
    {
      WORD_T *lmap = stree_lmap (stree, o, l);
      size_t loff = lmap_bitoff (l, laddr) >> WORDLOG2;
      WORD_T word = GET_WORD (lmap + loff);
      unsigned bit;

#if 0
      printf ("W=%d,l=%d,w=%llx,a=%lx\n", WORDSIZE, l, word, laddr);
      for (int i = 0; i < (1LL << o); i += (1 << (WORDLOG2 * (l))))
	{
	  long off = lmap_bitoff (l, i);
	  printf ("%x", get_bit (lmap, off));	//lbitmap[off >> WORDLOG2] & (off & 0x3f));
	}
#endif

      if (word == 0)
	{
	  /* Only top level should be zero, or the search table is corrupted. */
	  assert (l == LOGWORD (o) - 1);
	  return -1;
	}

      if (low)
	bit = ctz (word);
      else
	bit = WORDSIZE - 1 - clz (word);

      laddr |= bit << (l * WORDLOG2);
    }
  return laddr;
}


#if 0

#include <stdint.h>
#define ORDER 7

WORD_T tree[STREE_SIZE (ORDER)];

int
main ()
{
  printf ("ODER = %d, WORDSIZE = %d, DEPTH = %d\n", ORDER, WORDSIZE,
	  LOGWORD (ORDER));
  printf ("size of tree: %d WORD_T, %d bytes\n",
	  sizeof (tree) / sizeof (WORD_T), sizeof (tree));

  stree_setbit (tree, ORDER, 42);
  stree_setbit (tree, ORDER, 43);

  stree_setbit (tree, ORDER, 90);

  stree_setbit (tree, ORDER, 1270);
  //  stree_clrbit(tree, ORDER, 42);
  //  stree_clrbit(tree, ORDER, 43);

  stree_bitsearch (tree, ORDER, 1);

  signed l, i;
  for (l = LOGWORD (ORDER) - 1; l >= 0; l -= 1)
    {
      WORD_T *lbitmap = stree_lmap (tree, ORDER, l);

      printf ("Order %d Bitmap\n", l);
      printf ("Offset is %d\n", stree_lmap_off (ORDER, l));

      for (i = 0; i < (1LL << ORDER); i += (1 << (WORDLOG2 * l)))
	{
	  long off = lmap_bitoff (l, i);
	  printf ("%d", get_bit (lbitmap, off));	//lbitmap[off >> WORDLOG2] & (off & 0x3f));
	}
      printf ("\n");
    }

  stree_bitsearch (tree, ORDER, 0);
  stree_clrbit (tree, ORDER, 1270);

  stree_bitsearch (tree, ORDER, 0);
  stree_clrbit (tree, ORDER, 90);

  stree_bitsearch (tree, ORDER, 0);
  stree_clrbit (tree, ORDER, 43);

  stree_bitsearch (tree, ORDER, 0);
  stree_clrbit (tree, ORDER, 42);

  stree_bitsearch (tree, ORDER, 0);

}
#endif
