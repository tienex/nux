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
// Instruction Implementation
//

typedef struct _VINIL_INSTRUCTION_IMPL {
  IVinilInstructionVtbl  *lpVtbl;
  UINT32                 RefCount;
  VINIL_OPCODE           Opcode;
  VINIL_PRECISION        Precision;
  IVinilVariable         *Dst;
  IVinilVariable         *Src[3];
} VINIL_INSTRUCTION_IMPL;

//
// Block Implementation
//

typedef struct _VINIL_BLOCK_IMPL {
  IVinilBlockVtbl    *lpVtbl;
  UINT32             RefCount;
  UINT32             Id;
  IVinilInstruction  **Instructions;     /* Dynamic array of instruction pointers */
  UINT32             InstructionCount;
  UINT32             InstructionCapacity;
} VINIL_BLOCK_IMPL;

//
// Forward Declarations - Variable
//

static HRESULT STDMETHODCALLTYPE Variable_QueryInterface (IVinilVariable *This, REFIID riid, void **ppvObject);
static UINT32 STDMETHODCALLTYPE Variable_AddRef (IVinilVariable *This);
static UINT32 STDMETHODCALLTYPE Variable_Release (IVinilVariable *This);
static HRESULT STDMETHODCALLTYPE Variable_GetId (void *This, UINT32 *Id);
static HRESULT STDMETHODCALLTYPE Variable_GetName (void *This, CONST CHAR8 **Name, UINTN *NameLength);

//
// Forward Declarations - Instruction
//

static HRESULT STDMETHODCALLTYPE Instruction_QueryInterface (IVinilInstruction *This, REFIID riid, void **ppvObject);
static UINT32 STDMETHODCALLTYPE Instruction_AddRef (IVinilInstruction *This);
static UINT32 STDMETHODCALLTYPE Instruction_Release (IVinilInstruction *This);
static HRESULT STDMETHODCALLTYPE Instruction_GetOpcode (void *This, VINIL_OPCODE *Opcode);
static HRESULT STDMETHODCALLTYPE Instruction_GetPrecision (void *This, VINIL_PRECISION *Precision);
static HRESULT STDMETHODCALLTYPE Instruction_GetDestination (void *This, IVinilVariable **Destination);
static HRESULT STDMETHODCALLTYPE Instruction_GetSource (void *This, UINT32 Index, IVinilVariable **Source);

//
// Forward Declarations - Block
//

static HRESULT STDMETHODCALLTYPE Block_QueryInterface (IVinilBlock *This, REFIID riid, void **ppvObject);
static UINT32 STDMETHODCALLTYPE Block_AddRef (IVinilBlock *This);
static UINT32 STDMETHODCALLTYPE Block_Release (IVinilBlock *This);
static HRESULT STDMETHODCALLTYPE Block_GetInstructionCount (void *This, UINT32 *Count);
static HRESULT STDMETHODCALLTYPE Block_GetInstruction (void *This, UINT32 Index, IVinilInstruction **Instruction);
static HRESULT STDMETHODCALLTYPE Block_AppendInstruction (void *This, IVinilInstruction *Instruction);

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

static IVinilInstructionVtbl gInstructionVtbl = {
  Instruction_QueryInterface,
  Instruction_AddRef,
  Instruction_Release,
  Instruction_GetOpcode,
  Instruction_GetPrecision,
  Instruction_GetDestination,
  Instruction_GetSource
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
  void            *This,
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
  void            *This,
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
// IVinilInstruction Implementation
//

static
HRESULT
STDMETHODCALLTYPE
Instruction_QueryInterface (
  IVinilInstruction  *This,
  REFIID             riid,
  void               **ppvObject
  )
{
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  if (IsEqualGUID (*riid, IID_IUnknown) ||
      IsEqualGUID (*riid, IID_IVinilInstruction))
  {
    *ppvObject = This;
    Instruction_AddRef (This);
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static
UINT32
STDMETHODCALLTYPE
Instruction_AddRef (
  IVinilInstruction  *This
  )
{
  VINIL_INSTRUCTION_IMPL  *Inst = (VINIL_INSTRUCTION_IMPL *)This;
  return ++Inst->RefCount;
}

static
UINT32
STDMETHODCALLTYPE
Instruction_Release (
  IVinilInstruction  *This
  )
{
  VINIL_INSTRUCTION_IMPL  *Inst = (VINIL_INSTRUCTION_IMPL *)This;
  UINT32                  RefCount;
  UINT32                  i;

  RefCount = --Inst->RefCount;
  if (RefCount == 0) {
    /* Release variable references */
    if (Inst->Dst != NULL) {
      IVinilVariable_Release (Inst->Dst);
    }
    for (i = 0; i < 3; i++) {
      if (Inst->Src[i] != NULL) {
        IVinilVariable_Release (Inst->Src[i]);
      }
    }
    free (Inst);
  }

  return RefCount;
}

static
HRESULT
STDMETHODCALLTYPE
Instruction_GetOpcode (
  void           *This,
  VINIL_OPCODE   *Opcode
  )
{
  VINIL_INSTRUCTION_IMPL  *Inst = (VINIL_INSTRUCTION_IMPL *)This;

  if (Opcode == NULL) {
    return E_POINTER;
  }

  *Opcode = Inst->Opcode;
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Instruction_GetPrecision (
  void              *This,
  VINIL_PRECISION   *Precision
  )
{
  VINIL_INSTRUCTION_IMPL  *Inst = (VINIL_INSTRUCTION_IMPL *)This;

  if (Precision == NULL) {
    return E_POINTER;
  }

  *Precision = Inst->Precision;
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Instruction_GetDestination (
  void             *This,
  IVinilVariable   **Destination
  )
{
  VINIL_INSTRUCTION_IMPL  *Inst = (VINIL_INSTRUCTION_IMPL *)This;

  if (Destination == NULL) {
    return E_POINTER;
  }

  *Destination = Inst->Dst;
  if (*Destination != NULL) {
    IVinilVariable_AddRef (*Destination);
  }

  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Instruction_GetSource (
  void             *This,
  UINT32           Index,
  IVinilVariable   **Source
  )
{
  VINIL_INSTRUCTION_IMPL  *Inst = (VINIL_INSTRUCTION_IMPL *)This;

  if (Source == NULL) {
    return E_POINTER;
  }

  if (Index >= 3) {
    *Source = NULL;
    return E_INVALIDARG;
  }

  *Source = Inst->Src[Index];
  if (*Source != NULL) {
    IVinilVariable_AddRef (*Source);
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
  UINT32            i;

  RefCount = --Block->RefCount;
  if (RefCount == 0) {
    /* Release all instruction references */
    if (Block->Instructions != NULL) {
      for (i = 0; i < Block->InstructionCount; i++) {
        if (Block->Instructions[i] != NULL) {
          IVinilInstruction_Release (Block->Instructions[i]);
        }
      }
      free (Block->Instructions);
    }
    free (Block);
  }

  return RefCount;
}

static
HRESULT
STDMETHODCALLTYPE
Block_GetInstructionCount (
  void         *This,
  UINT32       *Count
  )
{
  VINIL_BLOCK_IMPL  *Block = (VINIL_BLOCK_IMPL *)This;

  if (Count == NULL) {
    return E_POINTER;
  }

  *Count = Block->InstructionCount;
  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Block_GetInstruction (
  void               *This,
  UINT32             Index,
  IVinilInstruction  **Instruction
  )
{
  VINIL_BLOCK_IMPL  *Block = (VINIL_BLOCK_IMPL *)This;

  if (Instruction == NULL) {
    return E_POINTER;
  }

  if (Index >= Block->InstructionCount) {
    *Instruction = NULL;
    return E_INVALIDARG;
  }

  *Instruction = Block->Instructions[Index];
  if (*Instruction != NULL) {
    IVinilInstruction_AddRef (*Instruction);
  }

  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Block_AppendInstruction (
  void               *This,
  IVinilInstruction  *Instruction
  )
{
  VINIL_BLOCK_IMPL    *Block = (VINIL_BLOCK_IMPL *)This;
  IVinilInstruction   **NewArray;
  UINT32              NewCapacity;

  if (Instruction == NULL) {
    return E_POINTER;
  }

  /* Grow array if needed */
  if (Block->InstructionCount >= Block->InstructionCapacity) {
    NewCapacity = Block->InstructionCapacity == 0 ? 8 : Block->InstructionCapacity * 2;
    NewArray = (IVinilInstruction **)realloc (Block->Instructions, NewCapacity * sizeof (IVinilInstruction *));
    if (NewArray == NULL) {
      return E_OUTOFMEMORY;
    }
    Block->Instructions = NewArray;
    Block->InstructionCapacity = NewCapacity;
  }

  /* Add instruction and increment refcount */
  Block->Instructions[Block->InstructionCount] = Instruction;
  IVinilInstruction_AddRef (Instruction);
  Block->InstructionCount++;

  return S_OK;
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
VinilInstructionCreate (
  VINIL_OPCODE       Opcode,
  VINIL_PRECISION    Precision,
  IVinilVariable     *Dst,
  IVinilVariable     *Src0,
  IVinilVariable     *Src1,
  IVinilVariable     *Src2,
  IVinilInstruction  **Instruction
  )
{
  VINIL_INSTRUCTION_IMPL  *InstImpl;

  if (Instruction == NULL) {
    return E_POINTER;
  }

  InstImpl = (VINIL_INSTRUCTION_IMPL *)malloc (sizeof (VINIL_INSTRUCTION_IMPL));
  if (InstImpl == NULL) {
    return E_OUTOFMEMORY;
  }

  memset (InstImpl, 0, sizeof (VINIL_INSTRUCTION_IMPL));
  InstImpl->lpVtbl = &gInstructionVtbl;
  InstImpl->RefCount = 1;
  InstImpl->Opcode = Opcode;
  InstImpl->Precision = Precision;
  InstImpl->Dst = Dst;
  InstImpl->Src[0] = Src0;
  InstImpl->Src[1] = Src1;
  InstImpl->Src[2] = Src2;

  /* AddRef all variables */
  if (Dst != NULL) {
    IVinilVariable_AddRef (Dst);
  }
  if (Src0 != NULL) {
    IVinilVariable_AddRef (Src0);
  }
  if (Src1 != NULL) {
    IVinilVariable_AddRef (Src1);
  }
  if (Src2 != NULL) {
    IVinilVariable_AddRef (Src2);
  }

  *Instruction = (IVinilInstruction *)InstImpl;
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
