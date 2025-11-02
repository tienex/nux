/** @file
  VINIL HLSL Compiler Frontend

  Compiles High-Level Shading Language (HLSL) to VINIL IL.
  Supports DirectX shaders and compute kernels.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once
#include <ananke/types.h>
#include <ananke/hresult.h>

//
// Shader Types
//

typedef enum _VINIL_HLSL_SHADER_TYPE {
    VinilHlslVertex     = 0,
    VinilHlslPixel      = 1,
    VinilHlslGeometry   = 2,
    VinilHlslCompute    = 3,
    VinilHlslHull       = 4,
    VinilHlslDomain     = 5,
} VINIL_HLSL_SHADER_TYPE;

//
// Shader Models
//

typedef enum _VINIL_HLSL_SHADER_MODEL {
    VinilHlslSM_4_0  = 0x0400,
    VinilHlslSM_5_0  = 0x0500,
    VinilHlslSM_5_1  = 0x0501,
    VinilHlslSM_6_0  = 0x0600,
    VinilHlslSM_6_1  = 0x0601,
    VinilHlslSM_6_2  = 0x0602,
    VinilHlslSM_6_3  = 0x0603,
    VinilHlslSM_6_4  = 0x0604,
    VinilHlslSM_6_5  = 0x0605,
} VINIL_HLSL_SHADER_MODEL;

//
// Compilation Options
//

typedef enum _VINIL_HLSL_FLAGS {
    VinilHlslNone               = 0,
    VinilHlslOptimize           = (1 << 0),  /* Optimize shader */
    VinilHlslDebugInfo          = (1 << 1),  /* Include debug info */
    VinilHlslWarningsAsErrors   = (1 << 2),  /* Treat warnings as errors */
    VinilHlslPackMatrixRowMajor = (1 << 3),  /* Row-major matrices */
    VinilHlslPackMatrixColMajor = (1 << 4),  /* Column-major matrices */
} VINIL_HLSL_FLAGS;

//
// Error Information
//

typedef struct _VINIL_HLSL_ERROR {
    UINT32      Line;           /* Line number */
    UINT32      Column;         /* Column number */
    CONST CHAR8 *Message;       /* Error message */
    CONST CHAR8 *File;          /* Source file */
} VINIL_HLSL_ERROR;

//
// Compiler Functions
//

/**
  Compile HLSL source to VINIL IL.

  @param[in]   Source       HLSL source code.
  @param[in]   SourceSize   Size of source in bytes.
  @param[in]   EntryPoint   Entry point function name.
  @param[in]   ShaderType   Type of shader.
  @param[in]   ShaderModel  Target shader model.
  @param[in]   Flags        Compilation flags.
  @param[out]  Program      Compiled IL program.
  @param[out]  Error        Error information (optional).

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Compilation failed.
**/
HRESULT
VinilCompileHLSL (
    CONST CHAR8             *Source,
    UINTN                   SourceSize,
    CONST CHAR8             *EntryPoint,
    VINIL_HLSL_SHADER_TYPE  ShaderType,
    VINIL_HLSL_SHADER_MODEL ShaderModel,
    VINIL_HLSL_FLAGS        Flags,
    VOID                    **Program,
    VINIL_HLSL_ERROR        *Error
    );

/**
  Compile HLSL file to VINIL IL.

  @param[in]   FilePath     Path to HLSL source file.
  @param[in]   EntryPoint   Entry point function name.
  @param[in]   ShaderType   Type of shader.
  @param[in]   ShaderModel  Target shader model.
  @param[in]   Flags        Compilation flags.
  @param[out]  Program      Compiled IL program.
  @param[out]  Error        Error information (optional).

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Compilation or I/O failed.
**/
HRESULT
VinilCompileHLSLFile (
    CONST CHAR8             *FilePath,
    CONST CHAR8             *EntryPoint,
    VINIL_HLSL_SHADER_TYPE  ShaderType,
    VINIL_HLSL_SHADER_MODEL ShaderModel,
    VINIL_HLSL_FLAGS        Flags,
    VOID                    **Program,
    VINIL_HLSL_ERROR        *Error
    );

/**
  Validate HLSL syntax without compiling.

  @param[in]   Source      HLSL source code.
  @param[in]   SourceSize  Size of source in bytes.
  @param[in]   ShaderType  Type of shader.
  @param[out]  Error       Error information (optional).

  @retval  S_OK   Valid syntax.
  @retval  E_FAIL Invalid syntax.
**/
HRESULT
VinilValidateHLSL (
    CONST CHAR8             *Source,
    UINTN                   SourceSize,
    VINIL_HLSL_SHADER_TYPE  ShaderType,
    VINIL_HLSL_ERROR        *Error
    );

