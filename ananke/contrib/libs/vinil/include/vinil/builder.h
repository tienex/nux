/** @file
  VINIL IL Program Builder

  High-level API for programmatically constructing IL programs.
  Provides a clean interface for creating instructions, variables, and control flow.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#ifndef __vinil_builder_h__
#define __vinil_builder_h__ 1

#include <ananke/types.h>
#include <ananke/hresult.h>

//
// Opaque Types
//

typedef struct IVinilBuilder IVinilBuilder;
typedef VOID* VINIL_VARIABLE;
typedef VOID* VINIL_BLOCK;
typedef VOID* VINIL_LABEL;

//
// Variable Types
//

typedef enum _VINIL_VAR_TYPE {
    VinilVarFloat       = 0,
    VinilVarFloat2      = 1,
    VinilVarFloat3      = 2,
    VinilVarFloat4      = 3,
    VinilVarInt         = 4,
    VinilVarInt2        = 5,
    VinilVarInt3        = 6,
    VinilVarInt4        = 7,
    VinilVarMat4        = 8,
} VINIL_VAR_TYPE;

//
// Builder Functions
//

/**
  Create a new IL program builder.

  @param[out]  Builder  Created builder instance.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
VinilCreateBuilder (
    IVinilBuilder  **Builder
    );

/**
  Destroy IL program builder.

  @param[in]  Builder  Builder instance to destroy.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
**/
HRESULT
VinilDestroyBuilder (
    IVinilBuilder  *Builder
    );

/**
  Create a variable (register).

  @param[in]   Builder  Builder instance.
  @param[in]   Type     Variable type.
  @param[in]   Name     Variable name (optional).
  @param[out]  Variable Created variable.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Creation failed.
**/
HRESULT
VinilBuilderCreateVariable (
    IVinilBuilder   *Builder,
    VINIL_VAR_TYPE  Type,
    CONST CHAR8     *Name,
    VINIL_VARIABLE  *Variable
    );

/**
  Create a basic block.

  @param[in]   Builder  Builder instance.
  @param[out]  Block    Created block.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Creation failed.
**/
HRESULT
VinilBuilderCreateBlock (
    IVinilBuilder  *Builder,
    VINIL_BLOCK    *Block
    );

/**
  Set current insertion block.

  @param[in]  Builder  Builder instance.
  @param[in]  Block    Block to insert into.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
**/
HRESULT
VinilBuilderSetInsertBlock (
    IVinilBuilder  *Builder,
    VINIL_BLOCK    Block
    );

/**
  Build ADD instruction: dst = src1 + src2.

  @param[in]  Builder  Builder instance.
  @param[in]  Dst      Destination variable.
  @param[in]  Src1     First source.
  @param[in]  Src2     Second source.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
**/
HRESULT
VinilBuilderBuildAdd (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2
    );

/**
  Build SUB instruction: dst = src1 - src2.
**/
HRESULT
VinilBuilderBuildSub (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2
    );

/**
  Build MUL instruction: dst = src1 * src2.
**/
HRESULT
VinilBuilderBuildMul (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2
    );

/**
  Build MAD instruction: dst = src1 * src2 + src3.
**/
HRESULT
VinilBuilderBuildMad (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2,
    VINIL_VARIABLE  Src3
    );

/**
  Build MOV instruction: dst = src.
**/
HRESULT
VinilBuilderBuildMov (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src
    );

/**
  Build DP3 instruction: dst = dot(src1.xyz, src2.xyz).
**/
HRESULT
VinilBuilderBuildDp3 (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2
    );

/**
  Build DP4 instruction: dst = dot(src1, src2).
**/
HRESULT
VinilBuilderBuildDp4 (
    IVinilBuilder   *Builder,
    VINIL_VARIABLE  Dst,
    VINIL_VARIABLE  Src1,
    VINIL_VARIABLE  Src2
    );

/**
  Build RET instruction (return from program/function).
**/
HRESULT
VinilBuilderBuildRet (
    IVinilBuilder  *Builder
    );

/**
  Finalize and get the built program.

  @param[in]   Builder  Builder instance.
  @param[out]  Program  Built IL program.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Finalization failed.
**/
HRESULT
VinilBuilderFinalize (
    IVinilBuilder  *Builder,
    VOID           **Program
    );

#endif // __vinil_builder_h__
