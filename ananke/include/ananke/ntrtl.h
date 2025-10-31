/** @file
  NT Run-Time Library (RTL) for ANANKE

  Provides comprehensive runtime library functions following Windows NT
  conventions for use by APXH bootloader and NUX kernel.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ANANKE_NTRTL_H__
#define __ANANKE_NTRTL_H__

#include <ananke/base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------
 *  Core NT RTL Types
 * --------------------------------------------------------------- */

/**
  NT String Descriptor (ANSI)

  Describes a counted string (not necessarily null-terminated).
**/
typedef struct _STRING {
    UINT16  Length;         ///< Current length in bytes
    UINT16  MaximumLength;  ///< Maximum length in bytes (buffer size)
    CHAR8   *Buffer;        ///< Pointer to string buffer
} STRING, *PSTRING, ANSI_STRING, *PANSI_STRING;

/**
  NT Unicode String Descriptor

  Describes a counted Unicode (UTF-16) string.
**/
typedef struct _UNICODE_STRING {
    UINT16  Length;         ///< Current length in bytes (not characters!)
    UINT16  MaximumLength;  ///< Maximum length in bytes
    CHAR16  *Buffer;        ///< Pointer to Unicode string buffer
} UNICODE_STRING, *PUNICODE_STRING;

/**
  Object String Descriptor (Generic)

  Can represent either ANSI or Unicode string.
**/
typedef STRING OEM_STRING;
typedef PSTRING POEM_STRING;

/**
  Doubly-Linked List Entry

  Standard NT list entry structure for intrusive doubly-linked lists.
**/
typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY  *Flink;  ///< Forward link
    struct _LIST_ENTRY  *Blink;  ///< Backward link
} LIST_ENTRY, *PLIST_ENTRY;

/**
  Single-Linked List Entry

  Standard NT single-linked list entry.
**/
typedef struct _SINGLE_LIST_ENTRY {
    struct _SINGLE_LIST_ENTRY  *Next;  ///< Next entry
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

/**
  Bitmap Descriptor

  Describes a bitmap for efficient bit manipulation.
**/
typedef struct _RTL_BITMAP {
    UINTN   SizeOfBitMap;  ///< Number of bits in bitmap
    UINTN   *Buffer;       ///< Pointer to bitmap buffer
} RTL_BITMAP, *PRTL_BITMAP;

/**
  Time Fields Structure

  Represents broken-down time fields.
**/
typedef struct _TIME_FIELDS {
    UINT16  Year;         ///< 1601 - 30827
    UINT16  Month;        ///< 1 - 12
    UINT16  Day;          ///< 1 - 31
    UINT16  Hour;         ///< 0 - 23
    UINT16  Minute;       ///< 0 - 59
    UINT16  Second;       ///< 0 - 59
    UINT16  Milliseconds; ///< 0 - 999
    UINT16  Weekday;      ///< 0 - 6 (Sunday = 0)
} TIME_FIELDS, *PTIME_FIELDS;

/**
  Large Integer (64-bit)

  Union for 64-bit integer manipulation.
**/
typedef union _LARGE_INTEGER {
    struct {
        UINT32  LowPart;
        INT32   HighPart;
    } u;
    INT64  QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

/**
  Unsigned Large Integer
**/
typedef union _ULARGE_INTEGER {
    struct {
        UINT32  LowPart;
        UINT32  HighPart;
    } u;
    UINT64  QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

/* ---------------------------------------------------------------
 *  String Functions
 * --------------------------------------------------------------- */

#include <ananke/ntrtl/string.h>

/* ---------------------------------------------------------------
 *  Memory Functions
 * --------------------------------------------------------------- */

#include <ananke/ntrtl/memory.h>

/* ---------------------------------------------------------------
 *  List Functions
 * --------------------------------------------------------------- */

#include <ananke/ntrtl/list.h>

/* ---------------------------------------------------------------
 *  Bitmap Functions
 * --------------------------------------------------------------- */

#include <ananke/ntrtl/bitmap.h>

/* ---------------------------------------------------------------
 *  Tree Functions
 * --------------------------------------------------------------- */

#include <ananke/ntrtl/tree.h>

/* ---------------------------------------------------------------
 *  Utility Functions
 * --------------------------------------------------------------- */

/**
  Raise an assertion failure.

  @param[in] Expression  Expression that failed
  @param[in] File        Source file name
  @param[in] Line        Line number
**/
VOID
EFIAPI
RtlAssert (
    IN CONST CHAR8  *Expression,
    IN CONST CHAR8  *File,
    IN UINT32       Line
    );

/**
  Capture the current stack backtrace.

  @param[in]  FramesToSkip    Number of frames to skip
  @param[in]  FramesToCapture Number of frames to capture
  @param[out] BackTrace       Buffer to store backtrace
  @param[out] BackTraceHash   Optional hash of backtrace

  @return Number of frames captured
**/
UINTN
EFIAPI
RtlCaptureStackBackTrace (
    IN  UINT32  FramesToSkip,
    IN  UINT32  FramesToCapture,
    OUT VOID    **BackTrace,
    OUT UINT32  *BackTraceHash OPTIONAL
    );

/**
  Compute CRC32 checksum.

  @param[in] InitialCrc  Initial CRC value
  @param[in] Buffer      Buffer to checksum
  @param[in] Length      Length of buffer

  @return CRC32 checksum
**/
UINT32
EFIAPI
RtlComputeCrc32 (
    IN UINT32       InitialCrc,
    IN CONST VOID   *Buffer,
    IN UINTN        Length
    );

/**
  Get version of NT RTL library.

  @return Version number (major << 16 | minor)
**/
UINT32
EFIAPI
RtlGetVersion (
    VOID
    );

/* RTL Version */
#define RTL_VERSION_MAJOR 1
#define RTL_VERSION_MINOR 0

#ifdef __cplusplus
}
#endif

#endif /* __ANANKE_NTRTL_H__ */
