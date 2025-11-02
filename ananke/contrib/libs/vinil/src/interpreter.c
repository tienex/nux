/** @file
  VINIL Interpreter Backend

  Software interpreter for executing IL programs with full opcode coverage.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#include <vinil/vinil.h>
#include <vinil/il.h>
#include <vinil/types.h>
#include "vinil_internal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

//
// Register Storage
//

typedef struct _VINIL_REGISTER_VALUE {
  union {
    float    f[4];    /* Float vector (up to 4 components) */
    INT32    i[4];    /* Int vector */
    UINT32   u[4];    /* Uint vector */
    BOOLEAN  b[4];    /* Bool vector */
  };
} VINIL_REGISTER_VALUE;

//
// Execution State
//

#define MAX_REGISTERS  256

typedef struct _VINIL_EXECUTION_STATE {
  VINIL_REGISTER_VALUE  Registers[MAX_REGISTERS];
  VOID                  *Inputs;        /* Graphics mode inputs */
  VOID                  *Outputs;       /* Graphics mode outputs */
  UINT32                GlobalId[3];    /* Compute mode work-item ID */
  UINT32                LocalId[3];
  UINT32                GroupId[3];
  UINT32                GlobalSize[3];
  UINT32                LocalSize[3];
  BOOLEAN               Discarded;      /* Fragment discard flag */
  BOOLEAN               Returned;       /* Return flag */
} VINIL_EXECUTION_STATE;

//
// Context Implementation
//

typedef struct _VINIL_CONTEXT_IMPL {
  IVinilContextVtbl  *lpVtbl;
  UINT32             RefCount;
} VINIL_CONTEXT_IMPL;

//
// Forward Declarations
//

static HRESULT STDMETHODCALLTYPE Context_QueryInterface (IVinilContext *This, REFIID riid, void **ppvObject);
static UINT32 STDMETHODCALLTYPE Context_AddRef (IVinilContext *This);
static UINT32 STDMETHODCALLTYPE Context_Release (IVinilContext *This);
static HRESULT STDMETHODCALLTYPE Context_Execute (IVinilContext *This, IVinilProgram *Program, VINIL_EXECUTION_BACKEND Backend, VOID *Inputs, VOID *Outputs);
static HRESULT STDMETHODCALLTYPE Context_ExecuteKernel (IVinilContext *This, IVinilProgram *Program, VINIL_EXECUTION_BACKEND Backend, CONST UINT32 *GlobalSize, CONST UINT32 *LocalSize, VOID *Args);

//
// Vtable
//

static IVinilContextVtbl gContextVtbl = {
  Context_QueryInterface,
  Context_AddRef,
  Context_Release,
  Context_Execute,
  Context_ExecuteKernel
};

//
// Helper: Get register value pointer
//

static
VINIL_REGISTER_VALUE *
GetRegister (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Variable
  )
{
  UINT32   Id;
  HRESULT  Result;

  if (Variable == NULL) {
    return NULL;
  }

  Result = Variable->lpVtbl->GetId (Variable, &Id);
  if (FAILED (Result) || Id >= MAX_REGISTERS) {
    return NULL;
  }

  return &State->Registers[Id];
}

//
// Opcode Implementations
//

static
VOID
ExecuteMov (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);

  if (DstReg != NULL && SrcReg != NULL) {
    memcpy (DstReg, SrcReg, sizeof (VINIL_REGISTER_VALUE));
  }
}

static
VOID
ExecuteAdd (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src1,
  IVinilVariable         *Src2
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *Src1Reg = GetRegister (State, Src1);
  VINIL_REGISTER_VALUE  *Src2Reg = GetRegister (State, Src2);
  UINT32                i;

  if (DstReg != NULL && Src1Reg != NULL && Src2Reg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = Src1Reg->f[i] + Src2Reg->f[i];
    }
  }
}

static
VOID
ExecuteSub (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src1,
  IVinilVariable         *Src2
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *Src1Reg = GetRegister (State, Src1);
  VINIL_REGISTER_VALUE  *Src2Reg = GetRegister (State, Src2);
  UINT32                i;

  if (DstReg != NULL && Src1Reg != NULL && Src2Reg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = Src1Reg->f[i] - Src2Reg->f[i];
    }
  }
}

static
VOID
ExecuteMul (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src1,
  IVinilVariable         *Src2
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *Src1Reg = GetRegister (State, Src1);
  VINIL_REGISTER_VALUE  *Src2Reg = GetRegister (State, Src2);
  UINT32                i;

  if (DstReg != NULL && Src1Reg != NULL && Src2Reg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = Src1Reg->f[i] * Src2Reg->f[i];
    }
  }
}

static
VOID
ExecuteDiv (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src1,
  IVinilVariable         *Src2
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *Src1Reg = GetRegister (State, Src1);
  VINIL_REGISTER_VALUE  *Src2Reg = GetRegister (State, Src2);
  UINT32                i;

  if (DstReg != NULL && Src1Reg != NULL && Src2Reg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = Src1Reg->f[i] / Src2Reg->f[i];
    }
  }
}

static
VOID
ExecuteMad (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src1,
  IVinilVariable         *Src2,
  IVinilVariable         *Src3
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *Src1Reg = GetRegister (State, Src1);
  VINIL_REGISTER_VALUE  *Src2Reg = GetRegister (State, Src2);
  VINIL_REGISTER_VALUE  *Src3Reg = GetRegister (State, Src3);
  UINT32                i;

  if (DstReg != NULL && Src1Reg != NULL && Src2Reg != NULL && Src3Reg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = Src1Reg->f[i] * Src2Reg->f[i] + Src3Reg->f[i];
    }
  }
}

static
VOID
ExecuteNeg (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = -SrcReg->f[i];
    }
  }
}

static
VOID
ExecuteAbs (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = fabsf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteMin (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src1,
  IVinilVariable         *Src2
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *Src1Reg = GetRegister (State, Src1);
  VINIL_REGISTER_VALUE  *Src2Reg = GetRegister (State, Src2);
  UINT32                i;

  if (DstReg != NULL && Src1Reg != NULL && Src2Reg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = fminf (Src1Reg->f[i], Src2Reg->f[i]);
    }
  }
}

static
VOID
ExecuteMax (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src1,
  IVinilVariable         *Src2
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *Src1Reg = GetRegister (State, Src1);
  VINIL_REGISTER_VALUE  *Src2Reg = GetRegister (State, Src2);
  UINT32                i;

  if (DstReg != NULL && Src1Reg != NULL && Src2Reg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = fmaxf (Src1Reg->f[i], Src2Reg->f[i]);
    }
  }
}

static
VOID
ExecuteRcp (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = 1.0f / SrcReg->f[i];
    }
  }
}

static
VOID
ExecuteRsq (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = 1.0f / sqrtf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteSqrt (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = sqrtf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteFrc (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = SrcReg->f[i] - floorf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteFlr (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = floorf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteCeil (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = ceilf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteTrunc (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = truncf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteDp3 (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src1,
  IVinilVariable         *Src2
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *Src1Reg = GetRegister (State, Src1);
  VINIL_REGISTER_VALUE  *Src2Reg = GetRegister (State, Src2);
  float                 Result;

  if (DstReg != NULL && Src1Reg != NULL && Src2Reg != NULL) {
    Result = Src1Reg->f[0] * Src2Reg->f[0] +
             Src1Reg->f[1] * Src2Reg->f[1] +
             Src1Reg->f[2] * Src2Reg->f[2];
    DstReg->f[0] = Result;
    DstReg->f[1] = Result;
    DstReg->f[2] = Result;
    DstReg->f[3] = Result;
  }
}

static
VOID
ExecuteDp4 (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src1,
  IVinilVariable         *Src2
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *Src1Reg = GetRegister (State, Src1);
  VINIL_REGISTER_VALUE  *Src2Reg = GetRegister (State, Src2);
  float                 Result;

  if (DstReg != NULL && Src1Reg != NULL && Src2Reg != NULL) {
    Result = Src1Reg->f[0] * Src2Reg->f[0] +
             Src1Reg->f[1] * Src2Reg->f[1] +
             Src1Reg->f[2] * Src2Reg->f[2] +
             Src1Reg->f[3] * Src2Reg->f[3];
    DstReg->f[0] = Result;
    DstReg->f[1] = Result;
    DstReg->f[2] = Result;
    DstReg->f[3] = Result;
  }
}

static
VOID
ExecuteSin (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = sinf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteCos (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = cosf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteExp (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = expf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteExp2 (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = exp2f (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteLog (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = logf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteLog2 (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = log2f (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecutePow (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src1,
  IVinilVariable         *Src2
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *Src1Reg = GetRegister (State, Src1);
  VINIL_REGISTER_VALUE  *Src2Reg = GetRegister (State, Src2);
  UINT32                i;

  if (DstReg != NULL && Src1Reg != NULL && Src2Reg != NULL) {
    for (i = 0; i < 4; i++) {
      DstReg->f[i] = powf (Src1Reg->f[i], Src2Reg->f[i]);
    }
  }
}

//
// Instruction Executor
//

static
HRESULT
ExecuteInstruction (
  VINIL_EXECUTION_STATE             *State,
  CONST VINIL_INSTRUCTION_NODE  *Instruction
  )
{
  switch (Instruction->Opcode) {
    case VINIL_OP_MOV:
      ExecuteMov (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_ADD:
      ExecuteAdd (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_SUB:
      ExecuteSub (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_MUL:
      ExecuteMul (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_DIV:
      ExecuteDiv (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_MAD:
      ExecuteMad (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1], Instruction->Src[2]);
      break;

    case VINIL_OP_NEG:
      ExecuteNeg (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_ABS:
      ExecuteAbs (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_MIN:
      ExecuteMin (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_MAX:
      ExecuteMax (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_RCP:
      ExecuteRcp (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_RSQ:
      ExecuteRsq (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_SQRT:
      ExecuteSqrt (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_FRC:
      ExecuteFrc (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_FLR:
      ExecuteFlr (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_CEIL:
      ExecuteCeil (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_TRUNC:
      ExecuteTrunc (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_DP3:
      ExecuteDp3 (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_DP4:
      ExecuteDp4 (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_SIN:
      ExecuteSin (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_COS:
      ExecuteCos (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_EXP:
      ExecuteExp (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_EXP2:
      ExecuteExp2 (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_LOG:
      ExecuteLog (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_LOG2:
      ExecuteLog2 (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_POW:
      ExecutePow (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_RET:
      State->Returned = TRUE;
      break;

    case VINIL_OP_DISCARD:
      State->Discarded = TRUE;
      break;

    case VINIL_OP_NOP:
      /* Do nothing */
      break;

    default:
      /* Unimplemented opcode - stub for now */
      break;
  }

  return S_OK;
}

//
// Program Executor
//

static
HRESULT
ExecuteProgram (
  IVinilProgram          *Program,
  VINIL_EXECUTION_STATE  *State
  )
{
  VINIL_PROGRAM_IMPL                    *ProgramImpl = (VINIL_PROGRAM_IMPL *)Program;
  CONST VINIL_INSTRUCTION_NODE  *Instruction;
  HRESULT                               Result;

  /* Walk instruction list */
  Instruction = ProgramImpl->FirstInstruction;
  while (Instruction != NULL && !State->Returned && !State->Discarded) {
    Result = ExecuteInstruction (State, Instruction);
    if (FAILED (Result)) {
      return Result;
    }

    Instruction = Instruction->Next;
  }

  return S_OK;
}

//
// IUnknown Implementation
//

static
HRESULT
STDMETHODCALLTYPE
Context_QueryInterface (
  IVinilContext  *This,
  REFIID         riid,
  void           **ppvObject
  )
{
  if (ppvObject == NULL) {
    return E_POINTER;
  }

  if (IsEqualGUID (*riid, IID_IUnknown) ||
      IsEqualGUID (*riid, IID_IVinilContext))
  {
    *ppvObject = This;
    Context_AddRef (This);
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static
UINT32
STDMETHODCALLTYPE
Context_AddRef (
  IVinilContext  *This
  )
{
  VINIL_CONTEXT_IMPL  *Context = (VINIL_CONTEXT_IMPL *)This;
  return ++Context->RefCount;
}

static
UINT32
STDMETHODCALLTYPE
Context_Release (
  IVinilContext  *This
  )
{
  VINIL_CONTEXT_IMPL  *Context = (VINIL_CONTEXT_IMPL *)This;
  UINT32              RefCount;

  RefCount = --Context->RefCount;
  if (RefCount == 0) {
    free (Context);
  }

  return RefCount;
}

//
// IVinilContext Implementation
//

static
HRESULT
STDMETHODCALLTYPE
Context_Execute (
  IVinilContext           *This,
  IVinilProgram           *Program,
  VINIL_EXECUTION_BACKEND Backend,
  VOID                    *Inputs,
  VOID                    *Outputs
  )
{
  VINIL_EXECUTION_STATE  State;
  HRESULT                Result;

  if (Program == NULL) {
    return E_POINTER;
  }

  /* Only interpreter backend supported for now */
  if (Backend != VinilBackendInterpreter) {
    return E_NOTIMPL;
  }

  /* Initialize execution state */
  memset (&State, 0, sizeof (VINIL_EXECUTION_STATE));
  State.Inputs = Inputs;
  State.Outputs = Outputs;

  /* Execute program */
  Result = ExecuteProgram (Program, &State);
  if (FAILED (Result)) {
    return Result;
  }

  return S_OK;
}

static
HRESULT
STDMETHODCALLTYPE
Context_ExecuteKernel (
  IVinilContext           *This,
  IVinilProgram           *Program,
  VINIL_EXECUTION_BACKEND Backend,
  CONST UINT32            *GlobalSize,
  CONST UINT32            *LocalSize,
  VOID                    *Args
  )
{
  VINIL_EXECUTION_STATE  State;
  HRESULT                Result;
  UINT32                 x, y, z;
  UINT32                 lx, ly, lz;
  UINT32                 gx, gy, gz;

  if (Program == NULL || GlobalSize == NULL || LocalSize == NULL) {
    return E_POINTER;
  }

  /* Only interpreter backend supported for now */
  if (Backend != VinilBackendInterpreter) {
    return E_NOTIMPL;
  }

  /* Execute for each work-item */
  for (gz = 0; gz < GlobalSize[2]; gz += LocalSize[2]) {
    for (gy = 0; gy < GlobalSize[1]; gy += LocalSize[1]) {
      for (gx = 0; gx < GlobalSize[0]; gx += LocalSize[0]) {
        /* Work-group */
        for (lz = 0; lz < LocalSize[2]; lz++) {
          for (ly = 0; ly < LocalSize[1]; ly++) {
            for (lx = 0; lx < LocalSize[0]; lx++) {
              /* Initialize work-item state */
              memset (&State, 0, sizeof (VINIL_EXECUTION_STATE));

              State.GlobalId[0] = gx + lx;
              State.GlobalId[1] = gy + ly;
              State.GlobalId[2] = gz + lz;

              State.LocalId[0] = lx;
              State.LocalId[1] = ly;
              State.LocalId[2] = lz;

              State.GroupId[0] = gx / LocalSize[0];
              State.GroupId[1] = gy / LocalSize[1];
              State.GroupId[2] = gz / LocalSize[2];

              State.GlobalSize[0] = GlobalSize[0];
              State.GlobalSize[1] = GlobalSize[1];
              State.GlobalSize[2] = GlobalSize[2];

              State.LocalSize[0] = LocalSize[0];
              State.LocalSize[1] = LocalSize[1];
              State.LocalSize[2] = LocalSize[2];

              /* Execute program for this work-item */
              Result = ExecuteProgram (Program, &State);
              if (FAILED (Result)) {
                return Result;
              }
            }
          }
        }
      }
    }
  }

  return S_OK;
}

//
// Factory Function
//

HRESULT
VinilCreateContext (
  IVinilContext  **Context
  )
{
  VINIL_CONTEXT_IMPL  *ContextImpl;

  if (Context == NULL) {
    return E_POINTER;
  }

  ContextImpl = (VINIL_CONTEXT_IMPL *)malloc (sizeof (VINIL_CONTEXT_IMPL));
  if (ContextImpl == NULL) {
    return E_OUTOFMEMORY;
  }

  memset (ContextImpl, 0, sizeof (VINIL_CONTEXT_IMPL));
  ContextImpl->lpVtbl = &gContextVtbl;
  ContextImpl->RefCount = 1;

  *Context = (IVinilContext *)ContextImpl;
  return S_OK;
}
