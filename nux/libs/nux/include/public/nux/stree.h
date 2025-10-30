/*
  STREE: A compact bit-tree allocator.
  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
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

/**
  Given a number of objects, find the order needed
  to create an STREE to manage them.

  @param[in] N  Number of objects.

  @return STREE order needed.
**/
static inline size_t
StreeOrder (size_t N)
{
  long Log2N = (LONG_BIT - 1 - __builtin_clzl ((long) N));
  long R = Log2N;

  /* Is the number a power of two? If not, add 1 */
  R += __builtin_popcount (N) > 1 ? 1 : 0;

  return R;
}

/** Legacy compatibility **/
#define stree_order StreeOrder

static inline size_t
StreeLmapOff (unsigned O, unsigned L)
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
  unsigned Y = L - 1;
  size_t C = 1 << (O - WORDLOG2 * Y);
  size_t R = C * ((1 << WORDLOG2 * L) - 1) / (WORDSIZE - 1);
  return R >> WORDLOG2;
}

/** Legacy compatibility **/
#define stree_lmap_off StreeLmapOff

/** Get level L bitmap of the search tree. **/
static inline WORD_T *
StreeLmap (WORD_T *Stree, unsigned O, unsigned L)
{
  return Stree + StreeLmapOff (O, L);
}

/** Legacy compatibility **/
#define stree_lmap StreeLmap

/** Get bit offset of an lmap of level L for address A. **/
static inline size_t
LmapBitOff (unsigned L, unsigned A)
{
  return ((size_t) A >> WORDLOG2 * L);
}

/** Legacy compatibility **/
#define lmap_bitoff LmapBitOff

static inline bool
SetBit (WORD_T *Map, size_t BitAddr)
{
  WORD_T Old;
  size_t Off = BitAddr >> WORDLOG2;
  size_t Bit = BitAddr & WORDMASK;

  Old = GET_WORD (Map + Off);
  OR_WORD (Map + Off, ((WORD_T) 1 << Bit));

  /* Return true if this is NOT the first bit set. */
  return !!Old;
}

/** Legacy compatibility **/
#define set_bit SetBit

static inline bool
ClrBit (WORD_T *Map, size_t BitAddr)
{
  size_t Off = BitAddr >> WORDLOG2;
  size_t Bit = BitAddr & WORDMASK;

  MASK_WORD (Map + Off, ~((WORD_T) 1 << Bit));

  /* Return true if word still has bit set. */
  return !!GET_WORD (Map + Off);
}

/** Legacy compatibility **/
#define clr_bit ClrBit

static inline int
GetBit (WORD_T *Map, size_t BitAddr)
{
  size_t Off = BitAddr >> WORDLOG2;
  size_t Bit = BitAddr & WORDMASK;

  return !!(GET_WORD (Map + Off) & ((WORD_T) 1 << Bit));
}

/** Legacy compatibility **/
#define get_bit GetBit

static inline int
StreeGetBit (WORD_T *Stree, unsigned O, size_t BitAddr)
{
  WORD_T *Lmap = StreeLmap (Stree, O, 0);

  return GetBit (Lmap, LmapBitOff (0, BitAddr));
}

/** Legacy compatibility **/
#define stree_getbit StreeGetBit

static inline void
StreeSetBit (WORD_T *Stree, unsigned O, size_t BitAddr)
{
  int L;

  for (L = 0; L <= LOGWORD (O) - 1; L++)
    {
      WORD_T *Lmap = StreeLmap (Stree, O, L);
      size_t Laddr = LmapBitOff (L, BitAddr);

      if (SetBit (Lmap, Laddr))
	{
	  /* Other bits were set before. Don't set upper levels. */
	  break;
	}
    }
}

/** Legacy compatibility **/
#define stree_setbit StreeSetBit

static inline void
StreeClrBit (WORD_T *Stree, unsigned O, size_t BitAddr)
{
  int L;

  for (L = 0; L <= LOGWORD (O) - 1; L++)
    {
      WORD_T *Lmap = StreeLmap (Stree, O, L);
      size_t Laddr = LmapBitOff (L, BitAddr);

      if (ClrBit (Lmap, Laddr))
	{
	  /* Other bits are set. Don't clear upper levels. */
	  break;
	}
    }
}

/** Legacy compatibility **/
#define stree_clrbit StreeClrBit

#include <string.h>
static inline void
StreeSetAll (WORD_T *Stree, unsigned O, unsigned long Max)
{
  int L;

  for (L = LOGWORD (O) - 1; L >= 0; L -= 1)
    {
      WORD_T *Lmap = StreeLmap (Stree, O, L);
      size_t Size = LmapBitOff (L, Max) >> WORDLOG2;
      size_t Bits = LmapBitOff (L, Max) & WORDMASK;
      memset (Lmap, -1, Size * sizeof (WORD_T));
      Lmap[Size] =
	Bits == WORDSIZE - 1 ? (WORD_T) - 1 : ((WORD_T) 1 << (Bits + 1)) - 1;
    }
}

/** Legacy compatibility **/
#define stree_setall StreeSetAll

/**
  Count all set bits.

  @param[in] Stree  Pointer to STREE.
  @param[in] O      Order.

  @return Number of set bits.
**/
static inline unsigned long
StreeCount (WORD_T *Stree, unsigned O)
{
  unsigned long Size = 0;
  WORD_T *Lmap = StreeLmap (Stree, O, 0);

  for (int i = 0; i < (1LL << O); i += (1 << WORDLOG2))
    {
      Size += __builtin_popcountl (Lmap[i >> WORDLOG2]);
    }

  return Size;
}

/** Legacy compatibility **/
#define stree_count StreeCount

/**
  Find a set bit.

  Low=1 will search the lowest address available,
  Low=0 will search the highest address available.

  @param[in] Stree  Pointer to STREE.
  @param[in] O      Order.
  @param[in] Low    Search direction (1=low, 0=high).

  @return Bit address or -1 if not found.
**/
static inline long
StreeBitSearch (WORD_T *Stree, unsigned O, int Low)
{
  int L;
  size_t Laddr;

  Laddr = 0;
  for (L = LOGWORD (O) - 1; L >= 0; L -= 1)
    {
      WORD_T *Lmap = StreeLmap (Stree, O, L);
      size_t Loff = LmapBitOff (L, Laddr) >> WORDLOG2;
      WORD_T Word = GET_WORD (Lmap + Loff);
      unsigned Bit;

#if 0
      printf ("W=%d,l=%d,w=%llx,a=%lx\n", WORDSIZE, L, Word, Laddr);
      for (int i = 0; i < (1LL << O); i += (1 << (WORDLOG2 * (L))))
	{
	  long Off = LmapBitOff (L, i);
	  printf ("%x", GetBit (Lmap, Off));	//lbitmap[Off >> WORDLOG2] & (Off & 0x3f));
	}
#endif

      if (Word == 0)
	{
	  /* Only top level should be zero, or the search table is corrupted. */
	  assert (L == LOGWORD (O) - 1);
	  return -1;
	}

      if (Low)
	Bit = ctz (Word);
      else
	Bit = WORDSIZE - 1 - clz (Word);

      Laddr |= Bit << (L * WORDLOG2);
    }
  return Laddr;
}

/** Legacy compatibility **/
#define stree_bitsearch StreeBitSearch


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
