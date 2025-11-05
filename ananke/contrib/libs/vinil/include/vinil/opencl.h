/** @file
  VINIL OpenCL C Compiler Frontend

  Compiles OpenCL C kernel source to VINIL IL.
  Supports OpenCL 1.0 through OpenCL 3.0.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once
#include <ananke/types.h>
#include <ananke/hresult.h>

//
// OpenCL Versions
//

typedef enum _VINIL_OPENCL_VERSION {
    VinilOpenCL_1_0 = 100,
    VinilOpenCL_1_1 = 110,
    VinilOpenCL_1_2 = 120,
    VinilOpenCL_2_0 = 200,
    VinilOpenCL_2_1 = 210,
    VinilOpenCL_2_2 = 220,
    VinilOpenCL_3_0 = 300,
} VINIL_OPENCL_VERSION;

//
// Compilation Options
//

typedef enum _VINIL_OPENCL_FLAGS {
    VinilOpenCLNone                 = 0,
    VinilOpenCLOptimize             = (1 << 0),  /* Optimize kernel */
    VinilOpenCLDebugInfo            = (1 << 1),  /* Include debug info */
    VinilOpenCLMadEnable            = (1 << 2),  /* Allow MAD optimizations */
    VinilOpenCLNoSignedZeros        = (1 << 3),  /* No signed zeros */
    VinilOpenCLUnsafeMathOpt        = (1 << 4),  /* Unsafe math optimizations */
    VinilOpenCLFiniteMathOnly       = (1 << 5),  /* Finite math only */
    VinilOpenCLFastRelaxedMath      = (1 << 6),  /* Fast relaxed math */
    VinilOpenCLUniformWorkGroupSize = (1 << 7),  /* Uniform work-group size */
} VINIL_OPENCL_FLAGS;

//
// Error Information
//

typedef struct _VINIL_OPENCL_ERROR {
    UINT32      Line;           /* Line number */
    UINT32      Column;         /* Column number */
    CONST CHAR8 *Message;       /* Error message */
    CONST CHAR8 *KernelName;    /* Kernel name (if applicable) */
} VINIL_OPENCL_ERROR;

//
// Compiler Functions
//

/**
  Compile OpenCL C source to VINIL IL.

  @param[in]   Source      OpenCL C source code.
  @param[in]   SourceSize  Size of source in bytes.
  @param[in]   Version     OpenCL C version.
  @param[in]   Options     Compiler options string (optional).
  @param[in]   Flags       Compilation flags.
  @param[out]  Program     Compiled IL program.
  @param[out]  Error       Error information (optional).

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Compilation failed.
**/
HRESULT
VinilCompileOpenCL (
    CONST CHAR8             *Source,
    UINTN                   SourceSize,
    VINIL_OPENCL_VERSION    Version,
    CONST CHAR8             *Options,
    VINIL_OPENCL_FLAGS      Flags,
    VOID                    **Program,
    VINIL_OPENCL_ERROR      *Error
    );

/**
  Compile OpenCL C file to VINIL IL.

  @param[in]   FilePath  Path to OpenCL C source file.
  @param[in]   Version   OpenCL C version.
  @param[in]   Options   Compiler options string (optional).
  @param[in]   Flags     Compilation flags.
  @param[out]  Program   Compiled IL program.
  @param[out]  Error     Error information (optional).

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Compilation or I/O failed.
**/
HRESULT
VinilCompileOpenCLFile (
    CONST CHAR8             *FilePath,
    VINIL_OPENCL_VERSION    Version,
    CONST CHAR8             *Options,
    VINIL_OPENCL_FLAGS      Flags,
    VOID                    **Program,
    VINIL_OPENCL_ERROR      *Error
    );

/**
  Validate OpenCL C syntax without compiling.

  @param[in]   Source      OpenCL C source code.
  @param[in]   SourceSize  Size of source in bytes.
  @param[in]   Version     OpenCL C version.
  @param[out]  Error       Error information (optional).

  @retval  S_OK   Valid syntax.
  @retval  E_FAIL Invalid syntax.
**/
HRESULT
VinilValidateOpenCL (
    CONST CHAR8             *Source,
    UINTN                   SourceSize,
    VINIL_OPENCL_VERSION    Version,
    VINIL_OPENCL_ERROR      *Error
    );

/**
  Get list of kernels in OpenCL C source.

  @param[in]   Source       OpenCL C source code.
  @param[in]   SourceSize   Size of source in bytes.
  @param[out]  KernelNames  Array of kernel name strings.
  @param[out]  Count        Number of kernels.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Parsing failed.
**/
HRESULT
VinilGetOpenCLKernels (
    CONST CHAR8     *Source,
    UINTN           SourceSize,
    CONST CHAR8     ***KernelNames,
    UINTN           *Count
    );

