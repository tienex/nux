/** @file
  VINIL IL Builder COM Implementation

  Production builder for programmatically constructing IL programs.
  Provides high-level API matching builder.h interface.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#define COBJMACROS
#include <vinil/builder.h>
#include <vinil/memory.h>
#include "vinil_internal.h"
#include <stdlib.h>
#include <string.h>

//
// Builder Implementation
//

typedef struct _VINIL_BUILDER_IMPL {
  IVinilBuilderVtbl   *lpVtbl;
  UINT32              RefCount;
  IVinilMemoryPool    *MemoryPool;
  IVinilProgram       *Program;
  IVinilBlock         *CurrentBlock;
  UINT32              NextVariableId;
  UINT32              NextBlockId;
  VINIL_EXECUTION_MODE Mode;
} VINIL_BUILDER_IMPL;

//
// Forward Declarations
//

static HRESULT STDMETHODCALLTYPE Builder_QueryInterface (IVinilBuilder *This, REFIID riid, void **ppvObject);
static UINT32 STDMETHODCALLTYPE Builder_AddRef (IVinilBuilder *This);
static UINT32 STDMETHODCALLTYPE Builder_Release (IVinilBuilder *This);
static HRESULT STDMETHODCALLTYPE Builder_CreateVariable (void *This, VINIL_VARIABLE_TYPE Type, CONST CHAR8 *Name, IVinilVariable **Variable);
static HRESULT STDMETHODCALLTYPE Builder_CreateBlock (void *This, IVinilBlock **Block);
static HRESULT STDMETHODCALLTYPE Builder_SetInsertBlock (void *This, IVinilBlock *Block);
static HRESULT STDMETHODCALLTYPE Builder_BuildAdd (void *This, IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2);
static HRESULT STDMETHODCALLTYPE Builder_BuildSub (void *This, IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2);
static HRESULT STDMETHODCALLTYPE Builder_BuildMul (void *This, IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2);
static HRESULT STDMETHODCALLTYPE Builder_BuildMad (void *This, IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2, IVinilVariable *Src3);
static HRESULT STDMETHODCALLTYPE Builder_BuildMov (void *This, IVinilVariable *Dst, IVinilVariable *Src);
static HRESULT STDMETHODCALLTYPE Builder_BuildDp3 (void *This, IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2);
static HRESULT STDMETHODCALLTYPE Builder_BuildDp4 (void *This, IVinilVariable *Dst, IVinilVariable *Src1, IVinilVariable *Src2);
static HRESULT STDMETHODCALLTYPE Builder_BuildRet (void *This);
static HRESULT STDMETHODCALLTYPE Builder_Finalize (void *This, IVinilProgram **Program);

//
// Vtable
//

static IVinilBuilderVtbl gBuilderVtbl = {
  Builder_QueryInterface,
  Builder_AddRef,
  Builder_Release,
  Builder_CreateVariable,
  Builder_CreateBlock,
  Builder_SetInsertBlock,
  Builder_BuildAdd,
  Builder_BuildSub,
  Builder_BuildMul,
  Builder_BuildMad,
  Builder_BuildMov,
  Builder_BuildDp3,
  Builder_BuildDp4,
  Builder_BuildRet,
  Builder_Finalize
};

//
// IUnknown Implementation
//

static
HRESULT
STDMETHODCALLTYPE
Builder_QueryInterface (
  IVinilBuilder  *This,
  REFIID         riid,
  void           **ppvObject
  )
{
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  if (IsEqualGUID (*riid, IID_IUnknown) ||
      IsEqualGUID (*riid, IID_IVinilBuilder))
  {
    *ppvObject = This;
    Builder_AddRef (This);
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static
UINT32
STDMETHODCALLTYPE
Builder_AddRef (
  IVinilBuilder  *This
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;
  return ++Builder->RefCount;
}

static
UINT32
STDMETHODCALLTYPE
Builder_Release (
  IVinilBuilder  *This
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;
  UINT32              RefCount;

  RefCount = --Builder->RefCount;
  if (RefCount == 0) {
    /* Release current block */
    if (Builder->CurrentBlock != NULL) {
      Builder->CurrentBlock->lpVtbl->Release (Builder->CurrentBlock);
    }

    /* Release program */
    if (Builder->Program != NULL) {
      Builder->Program->lpVtbl->Release (Builder->Program);
    }

    /* Release memory pool */
    if (Builder->MemoryPool != NULL) {
      Builder->MemoryPool->lpVtbl->Release (Builder->MemoryPool);
    }

    free (Builder);
  }

  return RefCount;
}

//
// IVinilBuilder Implementation
//

static
HRESULT
STDMETHODCALLTYPE
Builder_CreateVariable (
  void                 *This,
  VINIL_VARIABLE_TYPE  Type,
  CONST CHAR8          *Name,
  IVinilVariable       **Variable
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;
  IVinilType          *VarType;
  HRESULT             Result;

  if (Variable == NULL) {
    return E_POINTER;
  }

  /* Map VINIL_VARIABLE_TYPE to IVinilType */
  switch (Type) {
    case VinilVariableTypeFloat:
      Result = VinilGetBasicType (VINIL_TYPE_FLOAT, VinilPrecisionHigh, &VarType);
      break;
    case VinilVariableTypeFloat2:
      Result = VinilGetBasicType (VINIL_TYPE_FLOAT_VEC2, VinilPrecisionHigh, &VarType);
      break;
    case VinilVariableTypeFloat3:
      Result = VinilGetBasicType (VINIL_TYPE_FLOAT_VEC3, VinilPrecisionHigh, &VarType);
      break;
    case VinilVariableTypeFloat4:
      Result = VinilGetBasicType (VINIL_TYPE_FLOAT_VEC4, VinilPrecisionHigh, &VarType);
      break;
    case VinilVariableTypeInt:
      Result = VinilGetBasicType (VINIL_TYPE_INT, VinilPrecisionHigh, &VarType);
      break;
    case VinilVariableTypeInt2:
      Result = VinilGetBasicType (VINIL_TYPE_INT_VEC2, VinilPrecisionHigh, &VarType);
      break;
    case VinilVariableTypeInt3:
      Result = VinilGetBasicType (VINIL_TYPE_INT_VEC3, VinilPrecisionHigh, &VarType);
      break;
    case VinilVariableTypeInt4:
      Result = VinilGetBasicType (VINIL_TYPE_INT_VEC4, VinilPrecisionHigh, &VarType);
      break;
    case VinilVariableTypeMat4:
      Result = VinilGetBasicType (VINIL_TYPE_FLOAT_MAT4, VinilPrecisionHigh, &VarType);
      break;
    default:
      return E_INVALIDARG;
  }

  if (FAILED (Result)) {
    return Result;
  }

  /* Create variable */
  Result = VinilVariableCreate (VarType, Name, Builder->NextVariableId++, Variable);

  /* Release type (variable has its own reference) */
  VarType->lpVtbl->Release (VarType);

  return Result;
}

static
HRESULT
STDMETHODCALLTYPE
Builder_CreateBlock (
  void *This,
  IVinilBlock    **Block
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;

  if (Block == NULL) {
    return E_POINTER;
  }

  return VinilBlockCreate (Builder->NextBlockId++, Block);
}

static
HRESULT
STDMETHODCALLTYPE
Builder_SetInsertBlock (
  void *This,
  IVinilBlock    *Block
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;

  if (Block == NULL) {
    return E_POINTER;
  }

  /* Release old block */
  if (Builder->CurrentBlock != NULL) {
    Builder->CurrentBlock->lpVtbl->Release (Builder->CurrentBlock);
  }

  /* Set new block */
  Builder->CurrentBlock = Block;
  Block->lpVtbl->AddRef (Block);

  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Builder_BuildAdd (
  void *This,
  IVinilVariable  *Dst,
  IVinilVariable  *Src1,
  IVinilVariable  *Src2
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;

  if (Dst == NULL || Src1 == NULL || Src2 == NULL) {
    return E_POINTER;
  }

  return VinilProgramAddInstruction (Builder->Program, VINIL_OP_ADD, Dst, Src1, Src2, NULL);
}

static
HRESULT
STDMETHODCALLTYPE
Builder_BuildSub (
  void *This,
  IVinilVariable  *Dst,
  IVinilVariable  *Src1,
  IVinilVariable  *Src2
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;

  if (Dst == NULL || Src1 == NULL || Src2 == NULL) {
    return E_POINTER;
  }

  return VinilProgramAddInstruction (Builder->Program, VINIL_OP_SUB, Dst, Src1, Src2, NULL);
}

static
HRESULT
STDMETHODCALLTYPE
Builder_BuildMul (
  void *This,
  IVinilVariable  *Dst,
  IVinilVariable  *Src1,
  IVinilVariable  *Src2
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;

  if (Dst == NULL || Src1 == NULL || Src2 == NULL) {
    return E_POINTER;
  }

  return VinilProgramAddInstruction (Builder->Program, VINIL_OP_MUL, Dst, Src1, Src2, NULL);
}

static
HRESULT
STDMETHODCALLTYPE
Builder_BuildMad (
  void *This,
  IVinilVariable  *Dst,
  IVinilVariable  *Src1,
  IVinilVariable  *Src2,
  IVinilVariable  *Src3
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;

  if (Dst == NULL || Src1 == NULL || Src2 == NULL || Src3 == NULL) {
    return E_POINTER;
  }

  return VinilProgramAddInstruction (Builder->Program, VINIL_OP_MAD, Dst, Src1, Src2, Src3);
}

static
HRESULT
STDMETHODCALLTYPE
Builder_BuildMov (
  void *This,
  IVinilVariable  *Dst,
  IVinilVariable  *Src
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;

  if (Dst == NULL || Src == NULL) {
    return E_POINTER;
  }

  return VinilProgramAddInstruction (Builder->Program, VINIL_OP_MOV, Dst, Src, NULL, NULL);
}

static
HRESULT
STDMETHODCALLTYPE
Builder_BuildDp3 (
  void *This,
  IVinilVariable  *Dst,
  IVinilVariable  *Src1,
  IVinilVariable  *Src2
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;

  if (Dst == NULL || Src1 == NULL || Src2 == NULL) {
    return E_POINTER;
  }

  return VinilProgramAddInstruction (Builder->Program, VINIL_OP_DP3, Dst, Src1, Src2, NULL);
}

static
HRESULT
STDMETHODCALLTYPE
Builder_BuildDp4 (
  void *This,
  IVinilVariable  *Dst,
  IVinilVariable  *Src1,
  IVinilVariable  *Src2
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;

  if (Dst == NULL || Src1 == NULL || Src2 == NULL) {
    return E_POINTER;
  }

  return VinilProgramAddInstruction (Builder->Program, VINIL_OP_DP4, Dst, Src1, Src2, NULL);
}

static
HRESULT
STDMETHODCALLTYPE
Builder_BuildRet (
  void  *This
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;

  return VinilProgramAddInstruction (Builder->Program, VINIL_OP_RET, NULL, NULL, NULL, NULL);
}

static
HRESULT
STDMETHODCALLTYPE
Builder_Finalize (
  void *This,
  IVinilProgram  **Program
  )
{
  VINIL_BUILDER_IMPL  *Builder = (VINIL_BUILDER_IMPL *)This;

  if (Program == NULL) {
    return E_POINTER;
  }

  if (Builder->Program == NULL) {
    return E_FAIL;
  }

  /* Return program and AddRef */
  *Program = Builder->Program;
  Builder->Program->lpVtbl->AddRef (Builder->Program);

  return S_OK;
}

//
// Factory Function
//

HRESULT
VinilCreateBuilder (
  IVinilBuilder  **Builder
  )
{
  VINIL_BUILDER_IMPL  *BuilderImpl;
  IVinilMemoryPool    *MemoryPool;
  IVinilProgram       *Program;
  HRESULT             Result;

  if (Builder == NULL) {
    return E_POINTER;
  }

  /* Create memory pool (0 = use default page size) */
  Result = VinilCreateMemoryPool (0, &MemoryPool);
  if (FAILED (Result)) {
    return Result;
  }

  /* Create program (default to graphics mode) */
  Result = VinilProgramCreate (VinilExecutionModeGraphics, MemoryPool, &Program);
  if (FAILED (Result)) {
    MemoryPool->lpVtbl->Release (MemoryPool);
    return Result;
  }

  /* Allocate builder */
  BuilderImpl = (VINIL_BUILDER_IMPL *)malloc (sizeof (VINIL_BUILDER_IMPL));
  if (BuilderImpl == NULL) {
    Program->lpVtbl->Release (Program);
    MemoryPool->lpVtbl->Release (MemoryPool);
    return E_OUTOFMEMORY;
  }

  memset (BuilderImpl, 0, sizeof (VINIL_BUILDER_IMPL));
  BuilderImpl->lpVtbl = &gBuilderVtbl;
  BuilderImpl->RefCount = 1;
  BuilderImpl->MemoryPool = MemoryPool;
  BuilderImpl->Program = Program;
  BuilderImpl->CurrentBlock = NULL;
  BuilderImpl->NextVariableId = 0;
  BuilderImpl->NextBlockId = 0;
  BuilderImpl->Mode = VinilExecutionModeGraphics;

  *Builder = (IVinilBuilder *)BuilderImpl;
  return S_OK;
}
