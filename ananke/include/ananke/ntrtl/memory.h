/** @file
  NT RTL Memory Functions

  Memory manipulation functions following Windows NT RTL conventions.

  Copyright (C) 2025 ANANKE Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ANANKE_NTRTL_MEMORY_H__
#define __ANANKE_NTRTL_MEMORY_H__

/* ---------------------------------------------------------------
 *  Memory Fill Functions
 * --------------------------------------------------------------- */

/**
  Fill memory with zeros.

  @param[out] Destination  Destination buffer
  @param[in]  Length       Number of bytes to zero
**/
VOID
EFIAPI
RtlZeroMemory (
    OUT VOID   *Destination,
    IN  UINTN  Length
    );

/**
  Fill memory with a specified byte value.

  @param[out] Destination  Destination buffer
  @param[in]  Length       Number of bytes to fill
  @param[in]  Fill         Byte value to fill
**/
VOID
EFIAPI
RtlFillMemory (
    OUT VOID   *Destination,
    IN  UINTN  Length,
    IN  UINT8  Fill
    );

/**
  Securely zero memory (cannot be optimized away).

  @param[out] Destination  Destination buffer
  @param[in]  Length       Number of bytes to zero
**/
VOID
EFIAPI
RtlSecureZeroMemory (
    OUT VOID   *Destination,
    IN  UINTN  Length
    );

/**
  Fill memory with a pattern (32-bit).

  @param[out] Destination  Destination buffer (must be aligned)
  @param[in]  Length       Number of bytes to fill (must be multiple of 4)
  @param[in]  Pattern      32-bit pattern to fill
**/
VOID
EFIAPI
RtlFillMemoryUlong (
    OUT VOID    *Destination,
    IN  UINTN   Length,
    IN  UINT32  Pattern
    );

/**
  Fill memory with a pattern (native word size).

  @param[out] Destination  Destination buffer (must be aligned)
  @param[in]  Length       Number of bytes to fill (must be multiple of sizeof(UINTN))
  @param[in]  Pattern      Pattern to fill
**/
VOID
EFIAPI
RtlFillMemoryUintn (
    OUT VOID   *Destination,
    IN  UINTN  Length,
    IN  UINTN  Pattern
    );

/* ---------------------------------------------------------------
 *  Memory Copy Functions
 * --------------------------------------------------------------- */

/**
  Copy memory (buffers must not overlap).

  @param[out] Destination  Destination buffer
  @param[in]  Source       Source buffer
  @param[in]  Length       Number of bytes to copy
**/
VOID
EFIAPI
RtlCopyMemory (
    OUT VOID        *Destination,
    IN  CONST VOID  *Source,
    IN  UINTN       Length
    );

/**
  Move memory (buffers may overlap).

  @param[out] Destination  Destination buffer
  @param[in]  Source       Source buffer
  @param[in]  Length       Number of bytes to move
**/
VOID
EFIAPI
RtlMoveMemory (
    OUT VOID        *Destination,
    IN  CONST VOID  *Source,
    IN  UINTN       Length
    );

/**
  Copy memory in non-temporal way (bypass cache).

  Useful for large copies that should not pollute cache.

  @param[out] Destination  Destination buffer (must be aligned)
  @param[in]  Source       Source buffer (must be aligned)
  @param[in]  Length       Number of bytes to copy (must be multiple of cache line)
**/
VOID
EFIAPI
RtlCopyMemoryNonTemporal (
    OUT VOID        *Destination,
    IN  CONST VOID  *Source,
    IN  UINTN       Length
    );

/* ---------------------------------------------------------------
 *  Memory Comparison Functions
 * --------------------------------------------------------------- */

/**
  Compare two memory buffers.

  @param[in] Source1  First buffer
  @param[in] Source2  Second buffer
  @param[in] Length   Number of bytes to compare

  @return Number of bytes that match (equal to Length if buffers are identical)
**/
UINTN
EFIAPI
RtlCompareMemory (
    IN CONST VOID  *Source1,
    IN CONST VOID  *Source2,
    IN UINTN       Length
    );

/**
  Check if two memory buffers are equal.

  @param[in] Source1  First buffer
  @param[in] Source2  Second buffer
  @param[in] Length   Number of bytes to compare

  @retval TRUE   Buffers are equal
  @retval FALSE  Buffers are not equal
**/
BOOLEAN
EFIAPI
RtlEqualMemory (
    IN CONST VOID  *Source1,
    IN CONST VOID  *Source2,
    IN UINTN       Length
    );

/**
  Compare two memory buffers (byte-by-byte).

  @param[in] Source1  First buffer
  @param[in] Source2  Second buffer
  @param[in] Length   Number of bytes to compare

  @return <0 if Source1 < Source2, 0 if equal, >0 if Source1 > Source2
**/
INT32
EFIAPI
RtlCompareMemoryUlong (
    IN CONST VOID  *Source1,
    IN CONST VOID  *Source2,
    IN UINTN       Length
    );

/* ---------------------------------------------------------------
 *  Memory Macros (for performance-critical inline operations)
 * --------------------------------------------------------------- */

/**
  Zero small structures (inline).

  Use RtlZeroMemory for larger buffers.
**/
#define RtlZeroBytes(Destination, Length) \
    RtlZeroMemory((Destination), (Length))

/**
  Copy small structures (inline).

  Use RtlCopyMemory for larger buffers.
**/
#define RtlCopyBytes(Destination, Source, Length) \
    RtlCopyMemory((Destination), (Source), (Length))

/**
  Fill small structures with a byte value (inline).
**/
#define RtlFillBytes(Destination, Length, Fill) \
    RtlFillMemory((Destination), (Length), (Fill))

/* ---------------------------------------------------------------
 *  Memory Allocation (Optional - requires allocator integration)
 * --------------------------------------------------------------- */

/**
  Allocate memory from a pool.

  Note: This requires integration with a memory allocator (e.g., APXH or NUX allocator).

  @param[in] PoolType  Type of pool (reserved for future use)
  @param[in] Size      Number of bytes to allocate
  @param[in] Tag       Pool tag for tracking (4-character identifier)

  @return Pointer to allocated memory, or NULL if allocation fails
**/
VOID *
EFIAPI
RtlAllocateMemory (
    IN UINTN   PoolType,
    IN UINTN   Size,
    IN UINT32  Tag
    );

/**
  Free memory to a pool.

  @param[in] Memory  Pointer to memory to free
  @param[in] Tag     Pool tag (must match allocation tag)
**/
VOID
EFIAPI
RtlFreeMemory (
    IN VOID    *Memory,
    IN UINT32  Tag
    );

/**
  Reallocate memory.

  @param[in] Memory   Existing memory pointer (NULL for new allocation)
  @param[in] OldSize  Size of existing allocation
  @param[in] NewSize  New size requested
  @param[in] Tag      Pool tag

  @return Pointer to reallocated memory, or NULL if reallocation fails
**/
VOID *
EFIAPI
RtlReallocateMemory (
    IN VOID    *Memory OPTIONAL,
    IN UINTN   OldSize,
    IN UINTN   NewSize,
    IN UINT32  Tag
    );

/* ---------------------------------------------------------------
 *  Memory Prefetch Hints
 * --------------------------------------------------------------- */

/**
  Prefetch memory for reading.

  Hints to the CPU to prefetch memory into cache for upcoming reads.

  @param[in] Address  Address to prefetch
**/
VOID
EFIAPI
RtlPrefetchForRead (
    IN CONST VOID  *Address
    );

/**
  Prefetch memory for writing.

  Hints to the CPU to prefetch memory into cache for upcoming writes.

  @param[in] Address  Address to prefetch
**/
VOID
EFIAPI
RtlPrefetchForWrite (
    IN VOID  *Address
    );

#endif /* __ANANKE_NTRTL_MEMORY_H__ */
