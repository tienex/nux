/** @file
  VINIL AOT Compiler

  Ahead-Of-Time compiler that translates VINIL IL to native object files.
  Produces ELF, Mach-O, or PE/COFF object files for linking with applications.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#ifndef __vinil_aot_h__
#define __vinil_aot_h__ 1

#include <ananke/types.h>
#include <ananke/hresult.h>

//
// Target Architectures
//

typedef enum _VINIL_AOT_ARCH {
    VinilAotX86         = 0,
    VinilAotX86_64      = 1,
    VinilAotARM         = 2,
    VinilAotARM64       = 3,
    VinilAotRISCV32     = 4,
    VinilAotRISCV64     = 5,
    VinilAotPowerPC     = 6,
    VinilAotPowerPC64   = 7,
    VinilAotMIPS        = 8,
    VinilAotMIPS64      = 9,
} VINIL_AOT_ARCH;

//
// Object File Formats
//

typedef enum _VINIL_AOT_FORMAT {
    VinilAotELF         = 0,  /* ELF (Linux, BSD) */
    VinilAotMachO       = 1,  /* Mach-O (macOS, iOS) */
    VinilAotCOFF        = 2,  /* PE/COFF (Windows) */
    VinilAotWasm        = 3,  /* WebAssembly */
} VINIL_AOT_FORMAT;

//
// Optimization Levels
//

typedef enum _VINIL_AOT_OPT_LEVEL {
    VinilAotOptNone     = 0,  /* No optimization */
    VinilAotOptSize     = 1,  /* Optimize for size */
    VinilAotOptSpeed    = 2,  /* Optimize for speed */
    VinilAotOptMax      = 3,  /* Maximum optimization */
} VINIL_AOT_OPT_LEVEL;

//
// Compilation Options
//

typedef enum _VINIL_AOT_FLAGS {
    VinilAotNone                = 0,
    VinilAotPIC                 = (1 << 0),  /* Position-independent code */
    VinilAotDebugInfo           = (1 << 1),  /* Include debug information */
    VinilAotUnwindInfo          = (1 << 2),  /* Include unwind tables */
    VinilAotSafeStack           = (1 << 3),  /* Use safe stack */
    VinilAotVectorize           = (1 << 4),  /* Enable vectorization */
    VinilAotLinkTimeOpt         = (1 << 5),  /* Enable LTO */
} VINIL_AOT_FLAGS;

//
// Target Description
//

typedef struct _VINIL_AOT_TARGET {
    VINIL_AOT_ARCH      Arch;           /* Target architecture */
    VINIL_AOT_FORMAT    Format;         /* Object file format */
    VINIL_AOT_OPT_LEVEL OptLevel;       /* Optimization level */
    VINIL_AOT_FLAGS     Flags;          /* Compilation flags */
    CONST CHAR8         *Triple;        /* Target triple (e.g., "x86_64-pc-linux-gnu") */
    CONST CHAR8         *CPU;           /* CPU variant (e.g., "haswell", "cortex-a72") */
    CONST CHAR8         *Features;      /* CPU features (e.g., "+avx2,+fma") */
} VINIL_AOT_TARGET;

//
// AOT Compiler Functions
//

/**
  Compile IL program to native object file.

  @param[in]   Program      IL program to compile.
  @param[in]   Target       Target description.
  @param[out]  ObjectData   Generated object file data.
  @param[out]  ObjectSize   Size of object file in bytes.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
  @retval  E_FAIL         Compilation failed.
**/
HRESULT
VinilCompileAOT (
    CONST VOID              *Program,
    CONST VINIL_AOT_TARGET  *Target,
    VOID                    **ObjectData,
    UINTN                   *ObjectSize
    );

/**
  Compile IL program to object file on disk.

  @param[in]  Program     IL program to compile.
  @param[in]  Target      Target description.
  @param[in]  OutputPath  Path to output object file.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Compilation or I/O failed.
**/
HRESULT
VinilCompileAOTFile (
    CONST VOID              *Program,
    CONST VINIL_AOT_TARGET  *Target,
    CONST CHAR8             *OutputPath
    );

/**
  Compile IL binary to object file.

  @param[in]  BinaryPath  Path to IL binary file.
  @param[in]  Target      Target description.
  @param[in]  OutputPath  Path to output object file.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Compilation or I/O failed.
**/
HRESULT
VinilCompileBinaryToObject (
    CONST CHAR8             *BinaryPath,
    CONST VINIL_AOT_TARGET  *Target,
    CONST CHAR8             *OutputPath
    );

/**
  Get default target for current platform.

  @param[out]  Target  Default target description.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
**/
HRESULT
VinilGetDefaultTarget (
    VINIL_AOT_TARGET  *Target
    );

/**
  Get list of supported architectures.

  @param[out]  Architectures  Array of supported architectures.
  @param[out]  Count          Number of architectures.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
**/
HRESULT
VinilGetSupportedArchitectures (
    CONST VINIL_AOT_ARCH  **Architectures,
    UINTN                 *Count
    );

/**
  Get architecture name string.

  @param[in]  Arch  Architecture.

  @return  Architecture name or "Unknown".
**/
CONST CHAR8 *
VinilGetArchName (
    VINIL_AOT_ARCH  Arch
    );

/**
  Get format name string.

  @param[in]  Format  Object file format.

  @return  Format name or "Unknown".
**/
CONST CHAR8 *
VinilGetFormatName (
    VINIL_AOT_FORMAT  Format
    );

#endif // __vinil_aot_h__
