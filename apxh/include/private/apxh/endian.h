/** @file
  APXH Endianness Utilities

  Provides endianness detection and byte-swapping functions for
  cross-endian boot structure handling. Uses ananke byte-swap primitives.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#pragma once

#include <apxh/types.h>
#include <ananke/platform.h>
#include <ananke/intrinsics.h>

//
// Detect bootloader's native endianness at compile time using ananke macros
//

#if defined(ANX_ENDIAN_LE)
  #define APXH_BOOTLOADER_ENDIAN  ImgEndianLittle
#elif defined(ANX_ENDIAN_BE)
  #define APXH_BOOTLOADER_ENDIAN  ImgEndianBig
#else
  #error "Cannot detect bootloader endianness from ananke"
#endif

//
// Byte-swap primitives using ananke intrinsics
//

/**
  Swap bytes in 16-bit value.

  @param[in] Value  16-bit value to swap.

  @return Byte-swapped value.
**/
static inline UINT16
Swap16 (
  IN UINT16  Value
  )
{
  return ANX_BSWAP16(Value);
}

/**
  Swap bytes in 32-bit value.

  @param[in] Value  32-bit value to swap.

  @return Byte-swapped value.
**/
static inline UINT32
Swap32 (
  IN UINT32  Value
  )
{
  return ANX_BSWAP32(Value);
}

/**
  Swap bytes in 64-bit value.

  @param[in] Value  64-bit value to swap.

  @return Byte-swapped value.
**/
static inline UINT64
Swap64 (
  IN UINT64  Value
  )
{
  return ANX_BSWAP64(Value);
}

//
// Conditional swap based on target endianness
//

/**
  Convert UINT16 from bootloader endianness to target endianness.

  @param[in] Value          Value in bootloader endianness.
  @param[in] TargetEndian   Target endianness.

  @return Value in target endianness.
**/
static inline UINT16
ConvertEndian16 (
  IN UINT16          Value,
  IN IMGLOAD_ENDIAN  TargetEndian
  )
{
  if (TargetEndian == APXH_BOOTLOADER_ENDIAN || TargetEndian == ImgEndianUnknown) {
    return Value;
  }
  return Swap16(Value);
}

/**
  Convert UINT32 from bootloader endianness to target endianness.

  @param[in] Value          Value in bootloader endianness.
  @param[in] TargetEndian   Target endianness.

  @return Value in target endianness.
**/
static inline UINT32
ConvertEndian32 (
  IN UINT32          Value,
  IN IMGLOAD_ENDIAN  TargetEndian
  )
{
  if (TargetEndian == APXH_BOOTLOADER_ENDIAN || TargetEndian == ImgEndianUnknown) {
    return Value;
  }
  return Swap32(Value);
}

/**
  Convert UINT64 from bootloader endianness to target endianness.

  @param[in] Value          Value in bootloader endianness.
  @param[in] TargetEndian   Target endianness.

  @return Value in target endianness.
**/
static inline UINT64
ConvertEndian64 (
  IN UINT64          Value,
  IN IMGLOAD_ENDIAN  TargetEndian
  )
{
  if (TargetEndian == APXH_BOOTLOADER_ENDIAN || TargetEndian == ImgEndianUnknown) {
    return Value;
  }
  return Swap64(Value);
}

/**
  Get bootloader's native endianness.

  @return Bootloader endianness (ImgEndianLittle or ImgEndianBig).
**/
static inline IMGLOAD_ENDIAN
GetBootloaderEndianness (
  VOID
  )
{
  return APXH_BOOTLOADER_ENDIAN;
}

//
// Structure endianness conversion
//

VOID ConvertBootInfoEndianness(IN OUT APXH_BOOT_INFO *BootInfo, IN IMGLOAD_ENDIAN TargetEndian);
VOID ConvertRegionEndianness(IN OUT APXH_REGION *Region, IN IMGLOAD_ENDIAN TargetEndian);
VOID ConvertBatreeHeaderEndianness(IN OUT APXH_BATREE *BatreeHeader, IN IMGLOAD_ENDIAN TargetEndian);
