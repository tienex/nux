/** @file
  BATREE: Binary Allocator Tree - Compatible API using ANANKE NTRTL Bitmaps

  This header provides the BATREE API while using ANANKE NTRTL bitmap
  primitives underneath for simplified and portable implementation.

  Original BATREE by Gianluca Guida - now implemented using NTRTL bitmaps.

  Copyright (C) 2025 A•NUX Project
  Copyright (C) 2019 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __NUX_BATREE_H__
#define __NUX_BATREE_H__

#include <ananke/base.h>
#include <ananke/ntrtl.h>

/* ---------------------------------------------------------------
 *  BATREE Configuration
 * --------------------------------------------------------------- */

/**
  BATREE now uses NTRTL bitmaps underneath.
  We maintain the same API for compatibility.
**/

#define BATREE_USE_LONG_LONG
#define WORDSIZE  64
#define WORDLOG2  6
#define WORDMASK  0x3f
#define WORD_T    UINT64

/* ---------------------------------------------------------------
 *  BATREE Size Calculation
 * --------------------------------------------------------------- */

/**
  Calculate the size of a BATREE for a given order.

  With NTRTL bitmaps, we only need space for the actual bitmap,
  not the hierarchical structure. However, we maintain the same
  size calculation for compatibility.

  @param[in] O  Order (number of bits = 2^O)

  @return Size in WORD_T units
**/
#define BATREE_SIZE(_o) ((1ULL << (_o)) / WORDSIZE)

/* ---------------------------------------------------------------
 *  BATREE Helper Functions
 * --------------------------------------------------------------- */

/**
  Given a number of objects, find the order needed.

  @param[in] N  Number of objects

  @return BATREE order needed
**/
static INLINE UINTN
BatreeOrder (
  UINTN N
  )
{
    UINTN Log2N = (sizeof(UINTN) * 8 - 1 - ANX_CLZN(N));
    UINTN R = Log2N;

    /* Is the number a power of two? If not, add 1 */
    R += ANX_POPCOUNTN(N) > 1 ? 1 : 0;

    return R;
}

/* ---------------------------------------------------------------
 *  BATREE Operations using NTRTL Bitmaps
 * --------------------------------------------------------------- */

/**
  Get a bit from the BATREE.

  @param[in] Batree   Pointer to BATREE
  @param[in] O        Order
  @param[in] BitAddr  Bit address

  @return TRUE if bit is set, FALSE otherwise
**/
static INLINE BOOLEAN
BatreeGetBit (
  WORD_T *Batree,
  UINT32 O,
  UINTN BitAddr
  )
{
    RTL_BITMAP BitMap;
    UINTN SizeOfBitMap = (1ULL << O);

    RtlInitializeBitMap(&BitMap, (UINTN *)Batree, SizeOfBitMap);
    return RtlTestBit(&BitMap, BitAddr);
}

/**
  Set a bit in the BATREE.

  @param[in,out] Batree   Pointer to BATREE
  @param[in]     O        Order
  @param[in]     BitAddr  Bit address
**/
static INLINE VOID
BatreeSetBit (
  WORD_T *Batree,
  UINT32 O,
  UINTN BitAddr
  )
{
    RTL_BITMAP BitMap;
    UINTN SizeOfBitMap = (1ULL << O);

    RtlInitializeBitMap(&BitMap, (UINTN *)Batree, SizeOfBitMap);
    RtlSetBit(&BitMap, BitAddr);
}

/**
  Clear a bit in the BATREE.

  @param[in,out] Batree   Pointer to BATREE
  @param[in]     O        Order
  @param[in]     BitAddr  Bit address
**/
static INLINE VOID
BatreeClrBit (
  WORD_T *Batree,
  UINT32 O,
  UINTN BitAddr
  )
{
    RTL_BITMAP BitMap;
    UINTN SizeOfBitMap = (1ULL << O);

    RtlInitializeBitMap(&BitMap, (UINTN *)Batree, SizeOfBitMap);
    RtlClearBit(&BitMap, BitAddr);
}

/**
  Set all bits in the BATREE up to Max.

  @param[in,out] Batree  Pointer to BATREE
  @param[in]     O       Order
  @param[in]     Max     Maximum bit to set
**/
static INLINE VOID
BatreeSetAll (
  WORD_T *Batree,
  UINT32 O,
  UINTN Max
  )
{
    RTL_BITMAP BitMap;
    UINTN SizeOfBitMap = (1ULL << O);

    RtlInitializeBitMap(&BitMap, (UINTN *)Batree, SizeOfBitMap);

    if (Max >= SizeOfBitMap) {
        Max = SizeOfBitMap - 1;
    }

    RtlSetBits(&BitMap, 0, Max + 1);
}

/**
  Count all set bits in the BATREE.

  @param[in] Batree  Pointer to BATREE
  @param[in] O       Order

  @return Number of set bits
**/
static INLINE UINTN
BatreeCount (
  WORD_T *Batree,
  UINT32 O
  )
{
    RTL_BITMAP BitMap;
    UINTN SizeOfBitMap = (1ULL << O);

    RtlInitializeBitMap(&BitMap, (UINTN *)Batree, SizeOfBitMap);
    return RtlNumberOfSetBits(&BitMap);
}

/**
  Find a set bit in the BATREE.

  Low=1 will search the lowest address available,
  Low=0 will search the highest address available.

  @param[in] Batree  Pointer to BATREE
  @param[in] O       Order
  @param[in] Low     Search direction (1=low, 0=high)

  @return Bit address or -1 if not found
**/
static INLINE INTN
BatreeBitSearch (
  WORD_T *Batree,
  UINT32 O,
  INT32 Low
  )
{
    RTL_BITMAP BitMap;
    UINTN SizeOfBitMap = (1ULL << O);
    INTN Result;

    RtlInitializeBitMap(&BitMap, (UINTN *)Batree, SizeOfBitMap);

    if (Low) {
        /* Search for lowest (first) set bit */
        Result = RtlFindFirstSetBit(&BitMap);
    } else {
        /* Search for highest (last) set bit */
        Result = RtlFindLastSetBit(&BitMap);
    }

    return Result;
}

/* ---------------------------------------------------------------
 *  Legacy BATREE Functions (for compatibility)
 * --------------------------------------------------------------- */

/**
  Get level L bitmap of the search tree.
  (Legacy function - now just returns the base bitmap)

  @param[in] Batree  Pointer to BATREE
  @param[in] O       Order
  @param[in] L       Level

  @return Pointer to bitmap
**/
static INLINE WORD_T *
BatreeLmap (
  WORD_T *Batree,
  UINT32 O,
  UINT32 L
  )
{
    (VOID)O;
    (VOID)L;
    /* With NTRTL bitmaps, we only have one level */
    return Batree;
}

/**
  Get level L bitmap offset.
  (Legacy function - now always returns 0)

  @param[in] O  Order
  @param[in] L  Level

  @return Offset (always 0)
**/
static INLINE UINTN
BatreeLmapOff (
  UINT32 O,
  UINT32 L
  )
{
    (VOID)O;
    (VOID)L;
    /* With NTRTL bitmaps, there's no hierarchical structure */
    return 0;
}

#endif /* __NUX_BATREE_H__ */
