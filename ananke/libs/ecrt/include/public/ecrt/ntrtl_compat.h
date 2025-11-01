/** @file
  eCRT - An embedded C runtime library

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __eCRT_NTRTL_COMPAT_H__
#define __eCRT_NTRTL_COMPAT_H__

/**
  NT RTL Compatibility Layer for eCRT

  This header provides integration between eCRT and the NT Runtime Library (ntrtl).
  When USE_NTRTL is defined, eCRT can delegate to optimized ntrtl implementations
  for better performance and compatibility with the A•NUX Project ecosystem.
**/

#ifdef USE_NTRTL

/* Include NT RTL headers */
#include <ananke/ntrtl/memory.h>
#include <ananke/ntrtl/string.h>
#include <ananke/ntrtl/bitmap.h>
#include <ananke/ntrtl/list.h>
#include <ananke/ntrtl/tree.h>

/* Map eCRT functions to ntrtl equivalents where appropriate */

/**
  When USE_NTRTL is defined, applications can use ntrtl's memory and string
  management functions which provide enhanced functionality and integration
  with the A•NUX Project kernel and system libraries.

  Note: Full binary compatibility depends on proper configuration of both
  eCRT and ntrtl with matching calling conventions and data types.
**/

#endif /* USE_NTRTL */

#endif /* __eCRT_NTRTL_COMPAT_H__ */
