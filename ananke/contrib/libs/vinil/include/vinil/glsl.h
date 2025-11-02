/** @file
  VINIL GLSL Compiler Frontend

  Compiles OpenGL Shading Language (GLSL) to VINIL IL.
  Supports vertex, fragment, geometry, and compute shaders.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once
#include <ananke/types.h>
#include <ananke/hresult.h>

//
// Shader Types
//

typedef enum _VINIL_GLSL_SHADER_TYPE {
    VinilGlslVertex     = 0,
    VinilGlslFragment   = 1,
    VinilGlslGeometry   = 2,
    VinilGlslCompute    = 3,
    VinilGlslTessControl = 4,
    VinilGlslTessEval   = 5,
} VINIL_GLSL_SHADER_TYPE;

//
// Compilation Options
//

typedef enum _VINIL_GLSL_FLAGS {
    VinilGlslNone               = 0,
    VinilGlslOptimize           = (1 << 0),  /* Optimize shader */
    VinilGlslDebugInfo          = (1 << 1),  /* Include debug info */
    VinilGlslES                 = (1 << 2),  /* GLSL ES mode */
    VinilGlslCore               = (1 << 3),  /* Core profile */
    VinilGlslCompatibility      = (1 << 4),  /* Compatibility profile */
    VinilGlslVulkan             = (1 << 5),  /* Vulkan GLSL */
} VINIL_GLSL_FLAGS;

//
// Error Information
//

typedef struct _VINIL_GLSL_ERROR {
    UINT32      Line;           /* Line number */
    UINT32      Column;         /* Column number */
    CONST CHAR8 *Message;       /* Error message */
    CONST CHAR8 *File;          /* Source file (for #include) */
} VINIL_GLSL_ERROR;

//
// Compiler Functions
//

/**
  Compile GLSL source to VINIL IL.

  @param[in]   Source      GLSL source code.
  @param[in]   SourceSize  Size of source in bytes.
  @param[in]   ShaderType  Type of shader.
  @param[in]   Version     GLSL version (e.g., 330, 450).
  @param[in]   Flags       Compilation flags.
  @param[out]  Program     Compiled IL program.
  @param[out]  Error       Error information (optional).

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Compilation failed.
**/
HRESULT
VinilCompileGLSL (
    CONST CHAR8             *Source,
    UINTN                   SourceSize,
    VINIL_GLSL_SHADER_TYPE  ShaderType,
    UINT32                  Version,
    VINIL_GLSL_FLAGS        Flags,
    VOID                    **Program,
    VINIL_GLSL_ERROR        *Error
    );

/**
  Compile GLSL file to VINIL IL.

  @param[in]   FilePath    Path to GLSL source file.
  @param[in]   ShaderType  Type of shader.
  @param[in]   Version     GLSL version.
  @param[in]   Flags       Compilation flags.
  @param[out]  Program     Compiled IL program.
  @param[out]  Error       Error information (optional).

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Compilation or I/O failed.
**/
HRESULT
VinilCompileGLSLFile (
    CONST CHAR8             *FilePath,
    VINIL_GLSL_SHADER_TYPE  ShaderType,
    UINT32                  Version,
    VINIL_GLSL_FLAGS        Flags,
    VOID                    **Program,
    VINIL_GLSL_ERROR        *Error
    );

/**
  Validate GLSL syntax without compiling.

  @param[in]   Source      GLSL source code.
  @param[in]   SourceSize  Size of source in bytes.
  @param[in]   ShaderType  Type of shader.
  @param[in]   Version     GLSL version.
  @param[out]  Error       Error information (optional).

  @retval  S_OK   Valid syntax.
  @retval  E_FAIL Invalid syntax.
**/
HRESULT
VinilValidateGLSL (
    CONST CHAR8             *Source,
    UINTN                   SourceSize,
    VINIL_GLSL_SHADER_TYPE  ShaderType,
    UINT32                  Version,
    VINIL_GLSL_ERROR        *Error
    );

