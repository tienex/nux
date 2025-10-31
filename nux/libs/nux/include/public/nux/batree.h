/*
  BATREE: Binary Allocator Tree - A compact bit-tree allocator.

  Renamed from STREE to BATREE to avoid confusion with splay trees.
  BATREE uses a hierarchical bitmap structure for fast O(log n) allocation
  and deallocation of fixed-size objects.

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

  Set BATREE_USE_INT, BATREE_USE_LONG, BATREE_USE_LONG_LONG to use the
  native compiler types as a node. The number of bits in these types
  will describe how many subtrees you can have in a single node.

  Set WORDSIZE to 8 or 16 to have smaller nodes.
*/
//#define BATREE_USE_INT
//#define BATREE_USE_LONG
#define BATREE_USE_LONG_LONG

#ifdef BATREE_USE_INT
#define WORDSIZE WORD_BIT
#define ctz(_x)   ANX_CTZ32(_x)
#define clz(_x)   ANX_CLZ32(_x)
#endif

#ifdef BATREE_USE_LONG
#define WORDSIZE LONG_BIT
#define ctz(_x)   ANX_CTZL(_x)
#define clz(_x)   ANX_CLZL(_x)
#endif

#ifdef BATREE_USE_LONG_LONG
#define WORDSIZE  64
#define ctz(_x)   ANX_CTZ64(_x)
#define clz(_x)   ANX_CLZ64(_x)
#endif

#ifndef WORDSIZE
#define WORDSIZE 16
#endif

#if WORDSIZE==8
#define WORD_T    UINT8
#define WORDLOG2  3
#define WORDMASK  0x7
#define ctz(_x)   (ANX_CTZ32(_x))
#define clz(_x)   (ANX_CLZ32(_x) - WORD_BIT + 8)
#elif WORDSIZE==16
#define WORD_T    UINT16
#define WORDLOG2  4
#define WORDMASK  0xf
#define ctz(_x)   (ANX_CTZ32(_x))
#define clz(_x)   (ANX_CLZ32(_x) - WORD_BIT + 16)
#elif WORDSIZE==32
#define WORD_T    UINT32
#define WORDLOG2  5
#define WORDMASK  0x1f
#elif WORDSIZE==64
#define WORD_T    UINT64
#define WORDLOG2  6
#define WORDMASK  0x3f
#else
#error "Unsupported WORDSIZE"
#endif

#define CEIL_DIV(_n,_d) (((_n) + (_d) - 1)/(_d))
#define LOGWORD(_x) (((_x) + (WORDLOG2-1)) / WORDLOG2)

//#define clz(_x) ANX_CLZ32(_x)
//#define ctz(_x) ANX_CTZ32(_x)

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
#define BATREE_SIZE(_o) CEIL_DIV((1LL << (_o)) - 1, WORDSIZE - 1)

/**
  Given a number of objects, find the order needed
  to create a BATREE to manage them.

  @param[in] N  Number of objects.

  @return BATREE order needed.
**/
static INLINE UINTN
BatreeOrder (UINTN N)
{
  long Log2N = (LONG_BIT - 1 - ANX_CLZL ((long) N));
  long R = Log2N;

  /* Is the number a power of two? If not, add 1 */
  R += ANX_POPCOUNT32 (N) > 1 ? 1 : 0;

  return R;
}

static INLINE UINTN
BatreeLmapOff (UINT32 O, UINT32 L)
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
  UINT32 Y = L - 1;
  UINTN C = 1 << (O - WORDLOG2 * Y);
  UINTN R = C * ((1 << WORDLOG2 * L) - 1) / (WORDSIZE - 1);
  return R >> WORDLOG2;
}

/** Get level L bitmap of the search tree. **/
static INLINE WORD_T *
BatreeLmap (WORD_T *Batree, UINT32 O, UINT32 L)
{
  return Batree + BatreeLmapOff (O, L);
}

/** Get bit offset of an lmap of level L for address A. **/
static INLINE UINTN
LmapBitOff (UINT32 L, UINT32 A)
{
  return ((UINTN) A >> WORDLOG2 * L);
}

static INLINE BOOLEAN
SetBit (WORD_T *Map, UINTN BitAddr)
{
  WORD_T Old;
  UINTN Off = BitAddr >> WORDLOG2;
  UINTN Bit = BitAddr & WORDMASK;

  Old = GET_WORD (Map + Off);
  OR_WORD (Map + Off, ((WORD_T) 1 << Bit));

  /* Return TRUE if this is NOT the first bit set. */
  return !!Old;
}

static INLINE BOOLEAN
ClrBit (WORD_T *Map, UINTN BitAddr)
{
  UINTN Off = BitAddr >> WORDLOG2;
  UINTN Bit = BitAddr & WORDMASK;

  MASK_WORD (Map + Off, ~((WORD_T) 1 << Bit));

  /* Return TRUE if word still has bit set. */
  return !!GET_WORD (Map + Off);
}

static INLINE int
GetBit (WORD_T *Map, UINTN BitAddr)
{
  UINTN Off = BitAddr >> WORDLOG2;
  UINTN Bit = BitAddr & WORDMASK;

  return !!(GET_WORD (Map + Off) & ((WORD_T) 1 << Bit));
}

static INLINE int
BatreeGetBit (WORD_T *Batree, UINT32 O, UINTN BitAddr)
{
  WORD_T *Lmap = BatreeLmap (Batree, O, 0);

  return GetBit (Lmap, LmapBitOff (0, BitAddr));
}

static INLINE VOID
BatreeSetBit (WORD_T *Batree, UINT32 O, UINTN BitAddr)
{
  INT32 L;

  for (L = 0; L <= LOGWORD (O) - 1; L++)
    {
      WORD_T *Lmap = BatreeLmap (Batree, O, L);
      UINTN Laddr = LmapBitOff (L, BitAddr);

      if (SetBit (Lmap, Laddr))
	{
	  /* Other bits were set before. Don't set upper levels. */
	  break;
	}
    }
}

static INLINE VOID
BatreeClrBit (WORD_T *Batree, UINT32 O, UINTN BitAddr)
{
  INT32 L;

  for (L = 0; L <= LOGWORD (O) - 1; L++)
    {
      WORD_T *Lmap = BatreeLmap (Batree, O, L);
      UINTN Laddr = LmapBitOff (L, BitAddr);

      if (ClrBit (Lmap, Laddr))
	{
	  /* Other bits are set. Don't clear upper levels. */
	  break;
	}
    }
}

#include <string.h>
static INLINE VOID
BatreeSetAll (WORD_T *Batree, UINT32 O, unsigned long Max)
{
  INT32 L;

  for (L = LOGWORD (O) - 1; L >= 0; L -= 1)
    {
      WORD_T *Lmap = BatreeLmap (Batree, O, L);
      UINTN Size = LmapBitOff (L, Max) >> WORDLOG2;
      UINTN Bits = LmapBitOff (L, Max) & WORDMASK;
      memset (Lmap, -1, Size * sizeof (WORD_T));
      Lmap[Size] =
	Bits == WORDSIZE - 1 ? (WORD_T) - 1 : ((WORD_T) 1 << (Bits + 1)) - 1;
    }
}

/**
  Count all set bits.

  @param[in] Batree  Pointer to BATREE.
  @param[in] O       Order.

  @return Number of set bits.
**/
static INLINE unsigned long
BatreeCount (WORD_T *Batree, UINT32 O)
{
  unsigned long Size = 0;
  WORD_T *Lmap = BatreeLmap (Batree, O, 0);

  for (INT32 i = 0; i < (1LL << O); i += (1 << WORDLOG2))
    {
      Size += ANX_POPCOUNTL (Lmap[i >> WORDLOG2]);
    }

  return Size;
}

/**
  Find a set bit.

  Low=1 will search the lowest address available,
  Low=0 will search the highest address available.

  @param[in] Batree  Pointer to BATREE.
  @param[in] O       Order.
  @param[in] Low     Search direction (1=low, 0=high).

  @return Bit address or -1 if not found.
**/
static INLINE long
BatreeBitSearch (WORD_T *Batree, UINT32 O, INT32 Low)
{
  INT32 L;
  UINTN Laddr;

  Laddr = 0;
  for (L = LOGWORD (O) - 1; L >= 0; L -= 1)
    {
      WORD_T *Lmap = BatreeLmap (Batree, O, L);
      UINTN Loff = LmapBitOff (L, Laddr) >> WORDLOG2;
      WORD_T Word = GET_WORD (Lmap + Loff);
      UINT32 Bit;

#if 0
      printf ("W=%d,l=%d,w=%llx,a=%lx\n", WORDSIZE, L, Word, Laddr);
      for (INT32 i = 0; i < (1LL << O); i += (1 << (WORDLOG2 * (L))))
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


#if 0

#include <stdint.h>
#define ORDER 7

WORD_T tree[BATREE_SIZE (ORDER)];

int
main ()
{
  printf ("ORDER = %d, WORDSIZE = %d, DEPTH = %d\n", ORDER, WORDSIZE,
	  LOGWORD (ORDER));
  printf ("size of tree: %d WORD_T, %d bytes\n",
	  sizeof (tree) / sizeof (WORD_T), sizeof (tree));

  BatreeSetBit (tree, ORDER, 42);
  BatreeSetBit (tree, ORDER, 43);

  BatreeSetBit (tree, ORDER, 90);

  BatreeSetBit (tree, ORDER, 1270);
  //  BatreeClrBit(tree, ORDER, 42);
  //  BatreeClrBit(tree, ORDER, 43);

  BatreeBitSearch (tree, ORDER, 1);

  signed l, i;
  for (l = LOGWORD (ORDER) - 1; l >= 0; l -= 1)
    {
      WORD_T *lbitmap = BatreeLmap (tree, ORDER, l);

      printf ("Order %d Bitmap\n", l);
      printf ("Offset is %d\n", BatreeLmapOff (ORDER, l));

      for (i = 0; i < (1LL << ORDER); i += (1 << (WORDLOG2 * l)))
	{
	  long off = LmapBitOff (l, i);
	  printf ("%d", GetBit (lbitmap, off));	//lbitmap[off >> WORDLOG2] & (off & 0x3f));
	}
      printf ("\n");
    }

  BatreeBitSearch (tree, ORDER, 0);
  BatreeClrBit (tree, ORDER, 1270);

  BatreeBitSearch (tree, ORDER, 0);
  BatreeClrBit (tree, ORDER, 90);

  BatreeBitSearch (tree, ORDER, 0);
  BatreeClrBit (tree, ORDER, 43);

  BatreeBitSearch (tree, ORDER, 0);
  BatreeClrBit (tree, ORDER, 42);

  BatreeBitSearch (tree, ORDER, 0);

}
#endif
