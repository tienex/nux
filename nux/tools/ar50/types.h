/** @file
  Common Type Definitions for AR50/AR64 Tools

  Provides basic type definitions compatible with NUX coding style
  for standalone tool compilation.

  Copyright (C) 2015-2025 Gianluca Guida

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ar_types_h__
#define __ar_types_h__

#include <stdint.h>
#include <stddef.h>

// Basic integer types
typedef uint8_t   UINT8;
typedef uint16_t  UINT16;
typedef uint32_t  UINT32;
typedef uint64_t  UINT64;
typedef int8_t    INT8;
typedef int16_t   INT16;
typedef int32_t   INT32;
typedef int64_t   INT64;
typedef size_t    SIZE_T;

// Character types
typedef char      CHAR8;

// Pointer sizing types
typedef unsigned long UINTN;
typedef long          INTN;

// void type
typedef void VOID;

// Boolean type
typedef int BOOLEAN;
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// Function parameter direction macros
#define IN
#define OUT
#define CONST const

#endif /* __ar_types_h__ */
