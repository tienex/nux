/** @file
  VINIL Interpreter Backend

  Software interpreter for executing IL programs with full opcode coverage.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#define COBJMACROS
#include <vinil/vinil.h>
#include <vinil/il.h>
#include <vinil/types.h>
#include "vinil_internal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Execution state now defined in vinil_internal.h

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
static HRESULT STDMETHODCALLTYPE Context_Execute (void *This, IVinilProgram *Program, VINIL_EXECUTION_BACKEND Backend, VOID *Inputs, VOID *Outputs);
static HRESULT STDMETHODCALLTYPE Context_ExecuteKernel (void *This, IVinilProgram *Program, VINIL_EXECUTION_BACKEND Backend, CONST UINT32 *GlobalSize, CONST UINT32 *LocalSize, VOID *Args);

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

  Result = IVinilVariable_GetId (Variable, &Id);
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
ExecuteDp2 (
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
             Src1Reg->f[1] * Src2Reg->f[1];
    DstReg->f[0] = Result;
    DstReg->f[1] = Result;
    DstReg->f[2] = Result;
    DstReg->f[3] = Result;
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

static
VOID
ExecuteTan (
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
      DstReg->f[i] = tanf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteAsin (
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
      DstReg->f[i] = asinf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteAcos (
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
      DstReg->f[i] = acosf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteAtan (
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
      DstReg->f[i] = atanf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteAtan2 (
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
      DstReg->f[i] = atan2f (Src1Reg->f[i], Src2Reg->f[i]);
    }
  }
}

static
VOID
ExecuteClamp (
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
      DstReg->f[i] = fminf (fmaxf (Src1Reg->f[i], Src2Reg->f[i]), Src3Reg->f[i]);
    }
  }
}

static
VOID
ExecuteRound (
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
      DstReg->f[i] = roundf (SrcReg->f[i]);
    }
  }
}

static
VOID
ExecuteCrs (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src1,
  IVinilVariable         *Src2
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *Src1Reg = GetRegister (State, Src1);
  VINIL_REGISTER_VALUE  *Src2Reg = GetRegister (State, Src2);

  if (DstReg != NULL && Src1Reg != NULL && Src2Reg != NULL) {
    /* Cross product: a x b = (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x) */
    DstReg->f[0] = Src1Reg->f[1] * Src2Reg->f[2] - Src1Reg->f[2] * Src2Reg->f[1];
    DstReg->f[1] = Src1Reg->f[2] * Src2Reg->f[0] - Src1Reg->f[0] * Src2Reg->f[2];
    DstReg->f[2] = Src1Reg->f[0] * Src2Reg->f[1] - Src1Reg->f[1] * Src2Reg->f[0];
    DstReg->f[3] = 0.0f;
  }
}

static
VOID
ExecuteLen (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  float                 Length;

  if (DstReg != NULL && SrcReg != NULL) {
    Length = sqrtf (SrcReg->f[0] * SrcReg->f[0] +
                    SrcReg->f[1] * SrcReg->f[1] +
                    SrcReg->f[2] * SrcReg->f[2] +
                    SrcReg->f[3] * SrcReg->f[3]);
    DstReg->f[0] = Length;
    DstReg->f[1] = Length;
    DstReg->f[2] = Length;
    DstReg->f[3] = Length;
  }
}

static
VOID
ExecuteNrm (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);
  float                 Length;
  UINT32                i;

  if (DstReg != NULL && SrcReg != NULL) {
    Length = sqrtf (SrcReg->f[0] * SrcReg->f[0] +
                    SrcReg->f[1] * SrcReg->f[1] +
                    SrcReg->f[2] * SrcReg->f[2] +
                    SrcReg->f[3] * SrcReg->f[3]);

    if (Length > 0.0f) {
      for (i = 0; i < 4; i++) {
        DstReg->f[i] = SrcReg->f[i] / Length;
      }
    } else {
      for (i = 0; i < 4; i++) {
        DstReg->f[i] = 0.0f;
      }
    }
  }
}

static
VOID
ExecuteSeq (
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
      DstReg->f[i] = (Src1Reg->f[i] == Src2Reg->f[i]) ? 1.0f : 0.0f;
    }
  }
}

static
VOID
ExecuteSne (
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
      DstReg->f[i] = (Src1Reg->f[i] != Src2Reg->f[i]) ? 1.0f : 0.0f;
    }
  }
}

static
VOID
ExecuteSlt (
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
      DstReg->f[i] = (Src1Reg->f[i] < Src2Reg->f[i]) ? 1.0f : 0.0f;
    }
  }
}

static
VOID
ExecuteSle (
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
      DstReg->f[i] = (Src1Reg->f[i] <= Src2Reg->f[i]) ? 1.0f : 0.0f;
    }
  }
}

static
VOID
ExecuteSgt (
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
      DstReg->f[i] = (Src1Reg->f[i] > Src2Reg->f[i]) ? 1.0f : 0.0f;
    }
  }
}

static
VOID
ExecuteSge (
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
      DstReg->f[i] = (Src1Reg->f[i] >= Src2Reg->f[i]) ? 1.0f : 0.0f;
    }
  }
}

static
VOID
ExecuteAnd (
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
      DstReg->u[i] = Src1Reg->u[i] & Src2Reg->u[i];
    }
  }
}

static
VOID
ExecuteOr (
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
      DstReg->u[i] = Src1Reg->u[i] | Src2Reg->u[i];
    }
  }
}

static
VOID
ExecuteXor (
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
      DstReg->u[i] = Src1Reg->u[i] ^ Src2Reg->u[i];
    }
  }
}

static
VOID
ExecuteNot (
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
      DstReg->u[i] = ~SrcReg->u[i];
    }
  }
}

static
VOID
ExecuteShl (
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
      DstReg->u[i] = Src1Reg->u[i] << Src2Reg->u[i];
    }
  }
}

static
VOID
ExecuteShr (
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
      DstReg->u[i] = Src1Reg->u[i] >> Src2Reg->u[i];
    }
  }
}

static
VOID
ExecuteSar (
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
      DstReg->i[i] = Src1Reg->i[i] >> Src2Reg->u[i];
    }
  }
}

static
VOID
ExecuteSelect (
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
      DstReg->f[i] = (Src1Reg->f[i] != 0.0f) ? Src2Reg->f[i] : Src3Reg->f[i];
    }
  }
}

static
VOID
ExecuteGetGlobalId (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);

  if (DstReg != NULL && SrcReg != NULL) {
    UINT32 Dim = (UINT32)SrcReg->f[0];
    if (Dim < 3) {
      DstReg->f[0] = (float)State->GlobalId[Dim];
      DstReg->f[1] = (float)State->GlobalId[Dim];
      DstReg->f[2] = (float)State->GlobalId[Dim];
      DstReg->f[3] = (float)State->GlobalId[Dim];
    }
  }
}

static
VOID
ExecuteGetLocalId (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);

  if (DstReg != NULL && SrcReg != NULL) {
    UINT32 Dim = (UINT32)SrcReg->f[0];
    if (Dim < 3) {
      DstReg->f[0] = (float)State->LocalId[Dim];
      DstReg->f[1] = (float)State->LocalId[Dim];
      DstReg->f[2] = (float)State->LocalId[Dim];
      DstReg->f[3] = (float)State->LocalId[Dim];
    }
  }
}

static
VOID
ExecuteGetGroupId (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);

  if (DstReg != NULL && SrcReg != NULL) {
    UINT32 Dim = (UINT32)SrcReg->f[0];
    if (Dim < 3) {
      DstReg->f[0] = (float)State->GroupId[Dim];
      DstReg->f[1] = (float)State->GroupId[Dim];
      DstReg->f[2] = (float)State->GroupId[Dim];
      DstReg->f[3] = (float)State->GroupId[Dim];
    }
  }
}

static
VOID
ExecuteGetGlobalSize (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);

  if (DstReg != NULL && SrcReg != NULL) {
    UINT32 Dim = (UINT32)SrcReg->f[0];
    if (Dim < 3) {
      DstReg->f[0] = (float)State->GlobalSize[Dim];
      DstReg->f[1] = (float)State->GlobalSize[Dim];
      DstReg->f[2] = (float)State->GlobalSize[Dim];
      DstReg->f[3] = (float)State->GlobalSize[Dim];
    }
  }
}

static
VOID
ExecuteGetLocalSize (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);

  if (DstReg != NULL && SrcReg != NULL) {
    UINT32 Dim = (UINT32)SrcReg->f[0];
    if (Dim < 3) {
      DstReg->f[0] = (float)State->LocalSize[Dim];
      DstReg->f[1] = (float)State->LocalSize[Dim];
      DstReg->f[2] = (float)State->LocalSize[Dim];
      DstReg->f[3] = (float)State->LocalSize[Dim];
    }
  }
}

static
VOID
ExecuteGetNumGroups (
  VINIL_EXECUTION_STATE  *State,
  IVinilVariable         *Dst,
  IVinilVariable         *Src
  )
{
  VINIL_REGISTER_VALUE  *DstReg = GetRegister (State, Dst);
  VINIL_REGISTER_VALUE  *SrcReg = GetRegister (State, Src);

  if (DstReg != NULL && SrcReg != NULL) {
    UINT32 Dim = (UINT32)SrcReg->f[0];
    if (Dim < 3 && State->LocalSize[Dim] > 0) {
      UINT32 NumGroups = (State->GlobalSize[Dim] + State->LocalSize[Dim] - 1) / State->LocalSize[Dim];
      DstReg->f[0] = (float)NumGroups;
      DstReg->f[1] = (float)NumGroups;
      DstReg->f[2] = (float)NumGroups;
      DstReg->f[3] = (float)NumGroups;
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
    case VINIL_OP_MOVA:
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

    case VINIL_OP_DP2:
      ExecuteDp2 (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
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

    case VINIL_OP_TAN:
      ExecuteTan (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_ASIN:
      ExecuteAsin (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_ACOS:
      ExecuteAcos (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_ATAN:
      ExecuteAtan (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_ATAN2:
      ExecuteAtan2 (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_CLAMP:
      ExecuteClamp (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1], Instruction->Src[2]);
      break;

    case VINIL_OP_ROUND:
      ExecuteRound (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_CRS:
      ExecuteCrs (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_LEN:
      ExecuteLen (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_NRM:
      ExecuteNrm (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_SEQ:
      ExecuteSeq (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_SNE:
      ExecuteSne (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_SLT:
      ExecuteSlt (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_SLE:
      ExecuteSle (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_SGT:
      ExecuteSgt (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_SGE:
      ExecuteSge (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_AND:
      ExecuteAnd (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_OR:
      ExecuteOr (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_XOR:
      ExecuteXor (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_NOT:
      ExecuteNot (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_SHL:
      ExecuteShl (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_SHR:
      ExecuteShr (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_SAR:
      ExecuteSar (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1]);
      break;

    case VINIL_OP_SELECT:
      ExecuteSelect (State, Instruction->Dst, Instruction->Src[0], Instruction->Src[1], Instruction->Src[2]);
      break;

    case VINIL_OP_GET_GLOBAL_ID:
      ExecuteGetGlobalId (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_GET_LOCAL_ID:
      ExecuteGetLocalId (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_GET_GROUP_ID:
      ExecuteGetGroupId (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_GET_GLOBAL_SIZE:
      ExecuteGetGlobalSize (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_GET_LOCAL_SIZE:
      ExecuteGetLocalSize (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_GET_NUM_GROUPS:
      ExecuteGetNumGroups (State, Instruction->Dst, Instruction->Src[0]);
      break;

    case VINIL_OP_RET:
      State->Returned = TRUE;
      break;

    case VINIL_OP_DISCARD:
      State->Discarded = TRUE;
      break;

    case VINIL_OP_SHUFFLE:
      /* Vector shuffle using Src1 as component indices */
      if (Instruction->Dst != NULL && Instruction->Src[0] != NULL && Instruction->Src[1] != NULL) {
        VINIL_REGISTER_VALUE *DstReg = GetRegister (State, Instruction->Dst);
        VINIL_REGISTER_VALUE *SrcReg = GetRegister (State, Instruction->Src[0]);
        VINIL_REGISTER_VALUE *IdxReg = GetRegister (State, Instruction->Src[1]);

        if (DstReg != NULL && SrcReg != NULL && IdxReg != NULL) {
          /* Shuffle components based on indices */
          for (UINT32 i = 0; i < 4; i++) {
            UINT32 Idx = IdxReg->u[i] & 3; /* Mask to 0-3 */
            DstReg->f[i] = SrcReg->f[Idx];
          }
        }
      }
      break;

    case VINIL_OP_NOP:
      /* Do nothing */
      break;

    /* Control Flow Opcodes */
    case VINIL_OP_IF:
      /* Evaluate condition from first component of Src0 */
      if (Instruction->Src[0] != NULL) {
        VINIL_REGISTER_VALUE *Cond = GetRegister (State, Instruction->Src[0]);
        if (Cond != NULL) {
          /* Non-zero means true */
          State->ConditionResult = (Cond->f[0] != 0.0f) || (Cond->i[0] != 0) || Cond->b[0];
        } else {
          State->ConditionResult = FALSE;
        }
      } else {
        State->ConditionResult = FALSE;
      }
      break;

    case VINIL_OP_ELSE:
    case VINIL_OP_ENDIF:
    case VINIL_OP_LOOP:
    case VINIL_OP_ENDLOOP:
    case VINIL_OP_BREAK:
    case VINIL_OP_CONTINUE:
      /* Handled by ExecuteProgram */
      break;

    case VINIL_OP_CALL:
      /* TODO: Implement function calls with call stack */
      /* For now, this is a no-op */
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

/* Helper: Find matching control flow instruction (ELSE/ENDIF/ENDLOOP) */
static
CONST VINIL_INSTRUCTION_NODE *
FindControlFlowTarget (
  CONST VINIL_INSTRUCTION_NODE  *Start,
  VINIL_OPCODE                  StartOp,
  VINIL_OPCODE                  TargetOp1,
  VINIL_OPCODE                  TargetOp2
  )
{
  CONST VINIL_INSTRUCTION_NODE  *Inst = Start->Next;
  UINT32                        Depth = 1;

  while (Inst != NULL && Depth > 0) {
    if (Inst->Opcode == StartOp) {
      Depth++;
    } else if (Inst->Opcode == TargetOp1 || Inst->Opcode == TargetOp2) {
      Depth--;
      if (Depth == 0) {
        return Inst;
      }
    }
    Inst = Inst->Next;
  }

  return NULL;
}

static
HRESULT
ExecuteProgram (
  IVinilProgram          *Program,
  VINIL_EXECUTION_STATE  *State
  )
{
  VINIL_PROGRAM_IMPL                    *ProgramImpl = (VINIL_PROGRAM_IMPL *)Program;
  CONST VINIL_INSTRUCTION_NODE  *Instruction;
  CONST VINIL_INSTRUCTION_NODE  *Target;
  HRESULT                               Result;
  BOOLEAN                               Executing;

  /* Walk instruction list */
  Instruction = ProgramImpl->FirstInstruction;
  while (Instruction != NULL && !State->Returned && !State->Discarded) {
    /* Determine if we should execute this instruction based on control flow */
    Executing = TRUE;
    for (UINT32 i = 0; i < State->ControlFlowDepth; i++) {
      if (!State->ControlFlowStack[i].Executing) {
        Executing = FALSE;
        break;
      }
    }

    /* Execute instruction if in active branch */
    if (Executing) {
      Result = ExecuteInstruction (State, Instruction);
      if (FAILED (Result)) {
        return Result;
      }
    }

    /* Handle control flow jumps */
    switch (Instruction->Opcode) {
      case VINIL_OP_IF:
        /* Push IF context - target will be set by ExecuteInstruction */
        if (State->ControlFlowDepth < MAX_CONTROL_FLOW) {
          State->ControlFlowStack[State->ControlFlowDepth].Type = VINIL_CF_IF;
          State->ControlFlowStack[State->ControlFlowDepth].Executing =
            Executing && State->ConditionResult;
          State->ControlFlowDepth++;
        }
        break;

      case VINIL_OP_ELSE:
        /* Toggle execution for current IF */
        if (State->ControlFlowDepth > 0 &&
            State->ControlFlowStack[State->ControlFlowDepth - 1].Type == VINIL_CF_IF) {
          State->ControlFlowStack[State->ControlFlowDepth - 1].Type = VINIL_CF_ELSE;
          State->ControlFlowStack[State->ControlFlowDepth - 1].Executing =
            Executing && !State->ControlFlowStack[State->ControlFlowDepth - 1].Executing;
        }
        break;

      case VINIL_OP_ENDIF:
        /* Pop IF/ELSE context */
        if (State->ControlFlowDepth > 0) {
          State->ControlFlowDepth--;
        }
        break;

      case VINIL_OP_LOOP:
        /* Push LOOP context */
        if (State->ControlFlowDepth < MAX_CONTROL_FLOW) {
          State->ControlFlowStack[State->ControlFlowDepth].Type = VINIL_CF_LOOP;
          State->ControlFlowStack[State->ControlFlowDepth].Executing = Executing;
          State->ControlFlowStack[State->ControlFlowDepth].LoopStartIP = 0; /* Store instruction ptr */
          State->ControlFlowDepth++;
        }
        break;

      case VINIL_OP_ENDLOOP:
        /* Jump back to LOOP start */
        if (State->ControlFlowDepth > 0 && Executing) {
          /* Find matching LOOP instruction */
          Target = FindControlFlowTarget (Instruction, VINIL_OP_ENDLOOP, VINIL_OP_LOOP, VINIL_OP_LOOP);
          if (Target != NULL) {
            Instruction = Target;
            continue; /* Skip Instruction = Instruction->Next */
          }
        }
        /* Pop LOOP context */
        if (State->ControlFlowDepth > 0) {
          State->ControlFlowDepth--;
        }
        break;

      case VINIL_OP_BREAK:
        /* Jump to after ENDLOOP */
        if (Executing) {
          Target = FindControlFlowTarget (Instruction, VINIL_OP_LOOP, VINIL_OP_ENDLOOP, VINIL_OP_ENDLOOP);
          if (Target != NULL) {
            Instruction = Target;
          }
        }
        break;

      case VINIL_OP_CONTINUE:
        /* Jump to LOOP start */
        if (Executing) {
          /* Find matching LOOP instruction by scanning backwards */
          /* For simplicity, CONTINUE will be handled by ENDLOOP logic */
          Target = FindControlFlowTarget (Instruction, VINIL_OP_LOOP, VINIL_OP_ENDLOOP, VINIL_OP_ENDLOOP);
          if (Target != NULL) {
            Instruction = Target;
            continue;
          }
        }
        break;

      default:
        break;
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

//
// Interpreter Backend Execute (exported for dispatch)
//

HRESULT
VinilInterpreterExecute (
  IVinilProgram  *Program,
  VOID           *Inputs,
  VOID           *Outputs
  )
{
  VINIL_EXECUTION_STATE  State;
  HRESULT                Result;

  if (Program == NULL) {
    return E_POINTER;
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
Context_Execute (
  void                    *This,
  IVinilProgram           *Program,
  VINIL_EXECUTION_BACKEND Backend,
  VOID                    *Inputs,
  VOID                    *Outputs
  )
{
  (void)This;

  if (Program == NULL) {
    return E_POINTER;
  }

  /* Dispatch to appropriate backend */
  switch (Backend) {
    case VinilBackendInterpreter:
      return VinilInterpreterExecute (Program, Inputs, Outputs);

    case VinilBackendJit:
      return VinilJitExecute (Program, Inputs, Outputs);

    case VinilBackendAot:
      /* AOT requires pre-compiled code */
      return E_NOTIMPL;

    default:
      return E_INVALIDARG;
  }
}

static
HRESULT
STDMETHODCALLTYPE
Context_ExecuteKernel (
  void                    *This,
  IVinilProgram           *Program,
  VINIL_EXECUTION_BACKEND Backend,
  CONST UINT32            *GlobalSize,
  CONST UINT32            *LocalSize,
  VOID                    *Args
  )
{
  VINIL_EXECUTION_STATE  State;
  HRESULT                Result;
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
