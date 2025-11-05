/** @file
  VINIL Program COM Implementation

  Production IVinilProgram implementation with instruction storage and metadata.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#define COBJMACROS
#include <vinil/vinil.h>
#include <vinil/il.h>
#include <vinil/memory.h>
#include "vinil_internal.h"
#include <stdlib.h>
#include <string.h>

//
// Forward Declarations
//

static HRESULT STDMETHODCALLTYPE Program_QueryInterface (IVinilProgram *This, REFIID riid, void **ppvObject);
static UINT32 STDMETHODCALLTYPE Program_AddRef (IVinilProgram *This);
static UINT32 STDMETHODCALLTYPE Program_Release (IVinilProgram *This);
static HRESULT STDMETHODCALLTYPE Program_GetMode (void *This, VINIL_EXECUTION_MODE *Mode);
static HRESULT STDMETHODCALLTYPE Program_GetInstructionCount (void *This, UINT32 *Count);

//
// Vtable
//

static IVinilProgramVtbl gProgramVtbl = {
  Program_QueryInterface,
  Program_AddRef,
  Program_Release,
  Program_GetMode,
  Program_GetInstructionCount
};

//
// IUnknown Implementation
//

static
HRESULT
STDMETHODCALLTYPE
Program_QueryInterface (
  IVinilProgram  *This,
  REFIID         riid,
  void           **ppvObject
  )
{
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  if (IsEqualGUID (*riid, IID_IUnknown) ||
      IsEqualGUID (*riid, IID_IVinilProgram))
  {
    *ppvObject = This;
    Program_AddRef (This);
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static
UINT32
STDMETHODCALLTYPE
Program_AddRef (
  IVinilProgram  *This
  )
{
  VINIL_PROGRAM_IMPL  *Program = (VINIL_PROGRAM_IMPL *)This;
  return ++Program->RefCount;
}

static
UINT32
STDMETHODCALLTYPE
Program_Release (
  IVinilProgram  *This
  )
{
  VINIL_PROGRAM_IMPL  *Program = (VINIL_PROGRAM_IMPL *)This;
  UINT32              RefCount;

  RefCount = --Program->RefCount;
  if (RefCount == 0) {
    /* Release memory pool */
    if (Program->MemoryPool != NULL) {
      Program->MemoryPool->lpVtbl->Release (Program->MemoryPool);
    }

    free (Program);
  }

  return RefCount;
}

//
// IVinilProgram Implementation
//

static
HRESULT
STDMETHODCALLTYPE
Program_GetMode (
  void                  *This,
  VINIL_EXECUTION_MODE  *Mode
  )
{
  VINIL_PROGRAM_IMPL  *Program = (VINIL_PROGRAM_IMPL *)This;

  if (Mode == NULL) {
    return E_POINTER;
  }

  *Mode = Program->Mode;
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Program_GetInstructionCount (
  void           *This,
  UINT32         *Count
  )
{
  VINIL_PROGRAM_IMPL  *Program = (VINIL_PROGRAM_IMPL *)This;

  if (Count == NULL) {
    return E_POINTER;
  }

  *Count = Program->InstructionCount;
  return S_OK;
}

//
// Internal Functions
//

HRESULT
VinilProgramCreate (
  VINIL_EXECUTION_MODE  Mode,
  IVinilMemoryPool      *MemoryPool,
  IVinilProgram         **Program
  )
{
  VINIL_PROGRAM_IMPL  *ProgramImpl;

  if (Program == NULL || MemoryPool == NULL) {
    return E_POINTER;
  }

  ProgramImpl = (VINIL_PROGRAM_IMPL *)malloc (sizeof (VINIL_PROGRAM_IMPL));
  if (ProgramImpl == NULL) {
    return E_OUTOFMEMORY;
  }

  memset (ProgramImpl, 0, sizeof (VINIL_PROGRAM_IMPL));
  ProgramImpl->lpVtbl = &gProgramVtbl;
  ProgramImpl->RefCount = 1;
  ProgramImpl->Mode = Mode;
  ProgramImpl->MemoryPool = MemoryPool;
  ProgramImpl->FirstInstruction = NULL;
  ProgramImpl->LastInstruction = NULL;
  ProgramImpl->InstructionCount = 0;

  /* AddRef memory pool */
  IVinilMemoryPool_AddRef (MemoryPool);

  *Program = (IVinilProgram *)ProgramImpl;
  return S_OK;
}

HRESULT
VinilProgramAddInstruction (
  IVinilProgram   *Program,
  VINIL_OPCODE    Opcode,
  IVinilVariable  *Dst,
  IVinilVariable  *Src0,
  IVinilVariable  *Src1,
  IVinilVariable  *Src2
  )
{
  VINIL_PROGRAM_IMPL      *ProgramImpl = (VINIL_PROGRAM_IMPL *)Program;
  VINIL_INSTRUCTION_NODE  *Node;
  VOID                    *Memory;
  HRESULT                 Result;

  if (Program == NULL) {
    return E_POINTER;
  }

  /* Allocate instruction node */
  Result = ProgramImpl->MemoryPool->lpVtbl->Allocate (
    ProgramImpl->MemoryPool,
    sizeof (VINIL_INSTRUCTION_NODE),
    &Memory
  );

  if (FAILED (Result)) {
    return Result;
  }

  Node = (VINIL_INSTRUCTION_NODE *)Memory;
  memset (Node, 0, sizeof (VINIL_INSTRUCTION_NODE));
  Node->Opcode = Opcode;
  Node->Dst = Dst;
  Node->Src[0] = Src0;
  Node->Src[1] = Src1;
  Node->Src[2] = Src2;
  Node->Next = NULL;

  /* AddRef all variables */
  if (Dst != NULL) {
    IVinilMemoryPool_AddRef (Dst);
  }
  if (Src0 != NULL) {
    IVinilMemoryPool_AddRef (Src0);
  }
  if (Src1 != NULL) {
    IVinilMemoryPool_AddRef (Src1);
  }
  if (Src2 != NULL) {
    IVinilMemoryPool_AddRef (Src2);
  }

  /* Append to instruction list */
  if (ProgramImpl->LastInstruction != NULL) {
    ProgramImpl->LastInstruction->Next = Node;
  } else {
    ProgramImpl->FirstInstruction = Node;
  }
  ProgramImpl->LastInstruction = Node;
  ProgramImpl->InstructionCount++;

  return S_OK;
}
