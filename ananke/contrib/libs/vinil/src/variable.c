/** @file
  VINIL Variable and Block COM Implementation

  Production implementations of IVinilVariable and IVinilBlock matching il.h interfaces.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#define COBJMACROS
#include <vinil/il.h>
#include <vinil/types.h>
#include <stdlib.h>
#include <string.h>

//
// Variable Implementation
//

typedef struct _VINIL_VARIABLE_IMPL {
  IVinilVariableVtbl  *lpVtbl;
  UINT32              RefCount;
  IVinilType          *Type;          /* Internal type, not exposed by interface */
  CHAR8               Name[64];
  UINT32              Id;
} VINIL_VARIABLE_IMPL;

//
// Block Implementation
//

typedef struct _VINIL_BLOCK_IMPL {
  IVinilBlockVtbl  *lpVtbl;
  UINT32           RefCount;
  UINT32           Id;
  /* TODO: Add instruction storage */
} VINIL_BLOCK_IMPL;

//
// Forward Declarations - Variable
//

static HRESULT STDMETHODCALLTYPE Variable_QueryInterface (IVinilVariable *This, REFIID riid, void **ppvObject);
static UINT32 STDMETHODCALLTYPE Variable_AddRef (IVinilVariable *This);
static UINT32 STDMETHODCALLTYPE Variable_Release (IVinilVariable *This);
static HRESULT STDMETHODCALLTYPE Variable_GetId (IVinilVariable *This, UINT32 *Id);
static HRESULT STDMETHODCALLTYPE Variable_GetName (IVinilVariable *This, CONST CHAR8 **Name, UINTN *NameLength);

//
// Forward Declarations - Block
//

static HRESULT STDMETHODCALLTYPE Block_QueryInterface (IVinilBlock *This, REFIID riid, void **ppvObject);
static UINT32 STDMETHODCALLTYPE Block_AddRef (IVinilBlock *This);
static UINT32 STDMETHODCALLTYPE Block_Release (IVinilBlock *This);
static HRESULT STDMETHODCALLTYPE Block_GetInstructionCount (IVinilBlock *This, UINT32 *Count);
static HRESULT STDMETHODCALLTYPE Block_GetInstruction (IVinilBlock *This, UINT32 Index, IVinilInstruction **Instruction);
static HRESULT STDMETHODCALLTYPE Block_AppendInstruction (IVinilBlock *This, IVinilInstruction *Instruction);

//
// Vtables
//

static IVinilVariableVtbl gVariableVtbl = {
  Variable_QueryInterface,
  Variable_AddRef,
  Variable_Release,
  Variable_GetId,
  Variable_GetName
};

static IVinilBlockVtbl gBlockVtbl = {
  Block_QueryInterface,
  Block_AddRef,
  Block_Release,
  Block_GetInstructionCount,
  Block_GetInstruction,
  Block_AppendInstruction
};

//
// IVinilVariable Implementation
//

static
HRESULT
STDMETHODCALLTYPE
Variable_QueryInterface (
  IVinilVariable  *This,
  REFIID          riid,
  void            **ppvObject
  )
{
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  if (IsEqualGUID (*riid, IID_IUnknown) ||
      IsEqualGUID (*riid, IID_IVinilVariable))
  {
    *ppvObject = This;
    Variable_AddRef (This);
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static
UINT32
STDMETHODCALLTYPE
Variable_AddRef (
  IVinilVariable  *This
  )
{
  VINIL_VARIABLE_IMPL  *Variable = (VINIL_VARIABLE_IMPL *)This;
  return ++Variable->RefCount;
}

static
UINT32
STDMETHODCALLTYPE
Variable_Release (
  IVinilVariable  *This
  )
{
  VINIL_VARIABLE_IMPL  *Variable = (VINIL_VARIABLE_IMPL *)This;
  UINT32               RefCount;

  RefCount = --Variable->RefCount;
  if (RefCount == 0) {
    /* Release type */
    if (Variable->Type != NULL) {
      Variable->Type->lpVtbl->Release (Variable->Type);
    }

    free (Variable);
  }

  return RefCount;
}

static
HRESULT
STDMETHODCALLTYPE
Variable_GetId (
  IVinilVariable  *This,
  UINT32          *Id
  )
{
  VINIL_VARIABLE_IMPL  *Variable = (VINIL_VARIABLE_IMPL *)This;

  if (Id == NULL) {
    return E_POINTER;
  }

  *Id = Variable->Id;
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Variable_GetName (
  IVinilVariable  *This,
  CONST CHAR8     **Name,
  UINTN           *NameLength
  )
{
  VINIL_VARIABLE_IMPL  *Variable = (VINIL_VARIABLE_IMPL *)This;

  if (Name == NULL) {
    return E_POINTER;
  }

  *Name = Variable->Name;
  if (NameLength != NULL) {
    *NameLength = strlen ((const char *)Variable->Name);
  }

  return S_OK;
}

//
// IVinilBlock Implementation
//

static
HRESULT
STDMETHODCALLTYPE
Block_QueryInterface (
  IVinilBlock  *This,
  REFIID       riid,
  void         **ppvObject
  )
{
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  if (IsEqualGUID (*riid, IID_IUnknown) ||
      IsEqualGUID (*riid, IID_IVinilBlock))
  {
    *ppvObject = This;
    Block_AddRef (This);
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static
UINT32
STDMETHODCALLTYPE
Block_AddRef (
  IVinilBlock  *This
  )
{
  VINIL_BLOCK_IMPL  *Block = (VINIL_BLOCK_IMPL *)This;
  return ++Block->RefCount;
}

static
UINT32
STDMETHODCALLTYPE
Block_Release (
  IVinilBlock  *This
  )
{
  VINIL_BLOCK_IMPL  *Block = (VINIL_BLOCK_IMPL *)This;
  UINT32            RefCount;

  RefCount = --Block->RefCount;
  if (RefCount == 0) {
    free (Block);
  }

  return RefCount;
}

static
HRESULT
STDMETHODCALLTYPE
Block_GetInstructionCount (
  IVinilBlock  *This,
  UINT32       *Count
  )
{
  if (Count == NULL) {
    return E_POINTER;
  }

  /* TODO: Track instructions */
  *Count = 0;
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Block_GetInstruction (
  IVinilBlock        *This,
  UINT32             Index,
  IVinilInstruction  **Instruction
  )
{
  if (Instruction == NULL) {
    return E_POINTER;
  }

  /* TODO: Implement instruction storage */
  *Instruction = NULL;
  return E_NOTIMPL;
}

static
HRESULT
STDMETHODCALLTYPE
Block_AppendInstruction (
  IVinilBlock        *This,
  IVinilInstruction  *Instruction
  )
{
  if (Instruction == NULL) {
    return E_POINTER;
  }

  /* TODO: Implement instruction storage */
  return E_NOTIMPL;
}

//
// Factory Functions
//

HRESULT
VinilVariableCreate (
  IVinilType      *Type,
  CONST CHAR8     *Name,
  UINT32          Id,
  IVinilVariable  **Variable
  )
{
  VINIL_VARIABLE_IMPL  *VariableImpl;

  if (Variable == NULL || Type == NULL) {
    return E_POINTER;
  }

  VariableImpl = (VINIL_VARIABLE_IMPL *)malloc (sizeof (VINIL_VARIABLE_IMPL));
  if (VariableImpl == NULL) {
    return E_OUTOFMEMORY;
  }

  memset (VariableImpl, 0, sizeof (VINIL_VARIABLE_IMPL));
  VariableImpl->lpVtbl = &gVariableVtbl;
  VariableImpl->RefCount = 1;
  VariableImpl->Type = Type;
  VariableImpl->Id = Id;

  /* Copy name if provided */
  if (Name != NULL) {
    strncpy ((char *)VariableImpl->Name, (const char *)Name, sizeof (VariableImpl->Name) - 1);
    VariableImpl->Name[sizeof (VariableImpl->Name) - 1] = '\0';
  } else {
    VariableImpl->Name[0] = '\0';
  }

  /* AddRef type */
  IVinilMemoryPool_AddRef (Type);

  *Variable = (IVinilVariable *)VariableImpl;
  return S_OK;
}

HRESULT
VinilBlockCreate (
  UINT32       Id,
  IVinilBlock  **Block
  )
{
  VINIL_BLOCK_IMPL  *BlockImpl;

  if (Block == NULL) {
    return E_POINTER;
  }

  BlockImpl = (VINIL_BLOCK_IMPL *)malloc (sizeof (VINIL_BLOCK_IMPL));
  if (BlockImpl == NULL) {
    return E_OUTOFMEMORY;
  }

  memset (BlockImpl, 0, sizeof (VINIL_BLOCK_IMPL));
  BlockImpl->lpVtbl = &gBlockVtbl;
  BlockImpl->RefCount = 1;
  BlockImpl->Id = Id;

  *Block = (IVinilBlock *)BlockImpl;
  return S_OK;
}

/* Internal function to get variable type (not exposed by COM interface) */
IVinilType *
VinilVariableGetTypeInternal (
  IVinilVariable  *Variable
  )
{
  VINIL_VARIABLE_IMPL  *VariableImpl;

  if (Variable == NULL) {
    return NULL;
  }

  VariableImpl = (VINIL_VARIABLE_IMPL *)Variable;
  return VariableImpl->Type;
}
