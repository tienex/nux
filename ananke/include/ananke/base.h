/** @file
  ANANKE Base Definitions

  Provides base type definitions and calling conventions for ANANKE.
  This is the primary header for ANANKE components.

  Copyright (C) 2025 A•NUX Project

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ANANKE_BASE_H__
#define __ANANKE_BASE_H__

/* Include the main ANANKE foundation header */
#include <ananke/ananke.h>

/* ---------------------------------------------------------------
 *  ANANKE Calling Convention
 * --------------------------------------------------------------- */

/**
  ANXAPI - ANANKE API calling convention

  This is the standard calling convention for ANANKE APIs.
  It follows Microsoft x64/UEFI calling conventions:
  - On x64: Microsoft x64 calling convention (RCX, RDX, R8, R9)
  - On x86: cdecl calling convention
  - On ARM/ARM64: AAPCS calling convention
  - On RISC-V: Standard calling convention
**/
#ifndef ANXAPI
#   if defined(_MSC_VER)
        /* Microsoft compiler: use default (which is already correct) */
#       if defined(_M_X64) || defined(_M_AMD64)
#           define ANXAPI __fastcall
#       elif defined(_M_IX86)
#           define ANXAPI __cdecl
#       elif defined(_M_ARM) || defined(_M_ARM64)
#           define ANXAPI /* AAPCS is default */
#       else
#           define ANXAPI
#       endif
#   elif defined(__GNUC__) || defined(__clang__)
        /* GCC/Clang: use appropriate calling convention */
#       if defined(__x86_64__) || defined(__amd64__)
#           if defined(__WIN32__) || defined(__WIN64__) || defined(__CYGWIN__)
                /* Windows x64: Microsoft x64 calling convention */
#               define ANXAPI __attribute__((ms_abi))
#           else
                /* Unix x64: System V AMD64 ABI */
#               define ANXAPI
#           endif
#       elif defined(__i386__)
#           define ANXAPI __attribute__((cdecl))
#       else
            /* Other architectures: use default calling convention */
#           define ANXAPI
#       endif
#   elif defined(__WATCOMC__)
        /* Watcom C: use appropriate calling convention */
#       if defined(__386__) || defined(_M_I386)
#           define ANXAPI __cdecl
#       else
#           define ANXAPI
#       endif
#   else
        /* Unknown compiler: use default */
#       define ANXAPI
#   endif
#endif

/* ---------------------------------------------------------------
 *  Legacy Compatibility
 * --------------------------------------------------------------- */

/**
  For UEFI compatibility, we also provide EFIAPI as an alias to ANXAPI.
  This allows smooth transition from UEFI code.
**/
#ifndef EFIAPI
#   define EFIAPI ANXAPI
#endif

#endif /* __ANANKE_BASE_H__ */
