/** @file
  VINIL JIT Backend using SLJIT

  Just-In-Time compiler for VINIL IL using the SLJIT library.
  Compiles IL instructions to native machine code at runtime.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#define COBJMACROS
#include <vinil/vinil.h>
#include <vinil/il.h>
#include "vinil_internal.h"

/* SLJIT configuration */
#define SLJIT_CONFIG_AUTO 1
#define SLJIT_VERBOSE 0
#include "../../sljit/sljit_src/sljitLir.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

//
// Register Allocation
//

#define REG_STATE       SLJIT_S0    /* Execution state pointer (preserved) */
#define REG_TMP1        SLJIT_R0    /* Temporary register 1 */
#define REG_TMP2        SLJIT_R1    /* Temporary register 2 */
#define REG_TMP3        SLJIT_R2    /* Temporary register 3 */

//
// JIT Compilation Context
//

typedef struct {
  struct sljit_compiler *Compiler;
  VINIL_PROGRAM_IMPL *Program;

  /* Register to variable mapping */
  UINT32 VarToOffset[256];  /* Maps variable ID to register offset */

  /* Generated code */
  VOID *CodePtr;
  UINTN CodeSize;
} VINIL_JIT_CONTEXT;

//
// Forward Declarations
//

static HRESULT JitGeneratePrologue (VINIL_JIT_CONTEXT *Context);
static HRESULT JitGenerateEpilogue (VINIL_JIT_CONTEXT *Context);
static HRESULT JitCompileInstruction (VINIL_JIT_CONTEXT *Context, VINIL_INSTRUCTION_NODE *Inst);

//
// Helper: Get register memory offset for a variable
//

static
HRESULT
GetVariableOffset (
  VINIL_JIT_CONTEXT  *Context,
  IVinilVariable     *Variable,
  sljit_sw           *Offset
  )
{
  UINT32   VarId;
  HRESULT  Result;

  if (Variable == NULL) {
    return E_POINTER;
  }

  Result = IVinilVariable_GetId (Variable, &VarId);
  if (FAILED (Result) || VarId >= 256) {
    return E_FAIL;
  }

  /* Registers are at State->Registers[VarId] */
  /* Each register is 16 bytes (4 floats) */
  *Offset = offsetof (VINIL_EXECUTION_STATE, Registers) + (VarId * 16);

  return S_OK;
}

//
// Prologue: Function entry
//

static
HRESULT
JitGeneratePrologue (
  VINIL_JIT_CONTEXT  *Context
  )
{
  struct sljit_compiler *C = Context->Compiler;

  /* Function signature: void Execute(VINIL_EXECUTION_STATE *State) */
  sljit_emit_enter (C, 0,
    SLJIT_ARGS1V(P),  /* void func(void *state) - V suffix for void return */
    SLJIT_ENTER_FLOAT(3),  /* 0 int scratch + 3 float scratch (FR0, FR1, FR2) */
    1,  /* 1 saved register (S0) */
    0); /* 0 local stack size */

  /* REG_STATE (S0) = first argument (already set by emit_enter) */

  return S_OK;
}

//
// Epilogue: Function return
//

static
HRESULT
JitGenerateEpilogue (
  VINIL_JIT_CONTEXT  *Context
  )
{
  struct sljit_compiler *C = Context->Compiler;

  /* Return from function (void return) */
  sljit_emit_return_void (C);

  return S_OK;
}

//
// Opcode Generators
//

/* MOV: dst = src */
static
HRESULT
JitGenMov (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Copy 4 floats (16 bytes) */
  for (i = 0; i < 4; i++) {
    /* Load float from source */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Store float to destination */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* ADD: dst = src1 + src2 */
static
HRESULT
JitGenAdd (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* Add 4 floats component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    /* Load src2[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Add FR0 = FR0 + FR1 */
    sljit_emit_fop2 (C, SLJIT_ADD_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* SUB: dst = src1 - src2 */
static
HRESULT
JitGenSub (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* Subtract 4 floats component-wise */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    sljit_emit_fop2 (C, SLJIT_SUB_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* MUL: dst = src1 * src2 */
static
HRESULT
JitGenMul (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* Multiply 4 floats component-wise */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    sljit_emit_fop2 (C, SLJIT_MUL_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* RET: Return from program */
static
HRESULT
JitGenRet (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  /* Already handled by epilogue */
  (void)Context;
  (void)Inst;
  return S_OK;
}

/* DIV: dst = src1 / src2 */
static
HRESULT
JitGenDiv (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* Divide 4 floats component-wise */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    sljit_emit_fop2 (C, SLJIT_DIV_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* MAD: dst = src1 * src2 + src3 */
static
HRESULT
JitGenMad (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset, Src3Offset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[2], &Src3Offset);
  if (FAILED (Result)) return Result;

  /* MAD 4 floats component-wise: dst = src1 * src2 + src3 */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    /* Load src2[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* FR0 = FR0 * FR1 */
    sljit_emit_fop2 (C, SLJIT_MUL_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    /* Load src3[i] into FR1 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src3Offset + i * 4);

    /* FR0 = FR0 + FR1 */
    sljit_emit_fop2 (C, SLJIT_ADD_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* NEG: dst = -src */
static
HRESULT
JitGenNeg (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Negate 4 floats component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* FR0 = -FR0 */
    sljit_emit_fop1 (C, SLJIT_NEG_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* ABS: dst = abs(src) */
static
HRESULT
JitGenAbs (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* ABS 4 floats component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* FR0 = abs(FR0) */
    sljit_emit_fop1 (C, SLJIT_ABS_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* MIN: dst = min(src1, src2) */
static
HRESULT
JitGenMin (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* MIN 4 floats component-wise */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Compare: if FR0 < FR1, keep FR0, else use FR1 */
    sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_LESS,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    jump = sljit_emit_jump (C, SLJIT_LESS);

    /* FR0 >= FR1, use FR1 as minimum */
    sljit_emit_fop1 (C, SLJIT_MOV_F32,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    sljit_set_label (jump, sljit_emit_label (C));

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* MAX: dst = max(src1, src2) */
static
HRESULT
JitGenMax (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* MAX 4 floats component-wise */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Compare: if FR0 > FR1, keep FR0, else use FR1 */
    sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_GREATER,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    jump = sljit_emit_jump (C, SLJIT_GREATER);

    /* FR0 <= FR1, use FR1 as maximum */
    sljit_emit_fop1 (C, SLJIT_MOV_F32,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    sljit_set_label (jump, sljit_emit_label (C));

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* DP3: dst = dot(src1.xyz, src2.xyz) */
static
HRESULT
JitGenDp3 (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* First component: FR0 = src1.x * src2.x */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0,
    SLJIT_MEM1(REG_STATE), Src1Offset + 0);

  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR1,
    SLJIT_MEM1(REG_STATE), Src2Offset + 0);

  sljit_emit_fop2 (C, SLJIT_MUL_F32,
    SLJIT_FR0, 0,
    SLJIT_FR0, 0,
    SLJIT_FR1, 0);

  /* Accumulate Y and Z */
  for (i = 1; i < 3; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR2,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    sljit_emit_fop2 (C, SLJIT_MUL_F32,
      SLJIT_FR1, 0,
      SLJIT_FR1, 0,
      SLJIT_FR2, 0);

    sljit_emit_fop2 (C, SLJIT_ADD_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);
  }

  /* Store result in all 4 components */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* DP4: dst = dot(src1, src2) */
static
HRESULT
JitGenDp4 (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* First component: FR0 = src1.x * src2.x */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0,
    SLJIT_MEM1(REG_STATE), Src1Offset + 0);

  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR1,
    SLJIT_MEM1(REG_STATE), Src2Offset + 0);

  sljit_emit_fop2 (C, SLJIT_MUL_F32,
    SLJIT_FR0, 0,
    SLJIT_FR0, 0,
    SLJIT_FR1, 0);

  /* Accumulate Y, Z, W */
  for (i = 1; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR2,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    sljit_emit_fop2 (C, SLJIT_MUL_F32,
      SLJIT_FR1, 0,
      SLJIT_FR1, 0,
      SLJIT_FR2, 0);

    sljit_emit_fop2 (C, SLJIT_ADD_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);
  }

  /* Store result in all 4 components */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* RCP: dst = 1.0 / src */
static
HRESULT
JitGenRcp (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;
  union {
    float f;
    sljit_u32 u;
  } one;

  one.f = 1.0f;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Reciprocal 4 floats component-wise: dst = 1.0 / src */
  for (i = 0; i < 4; i++) {
    /* Load constant 1.0 into FR0 */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, one.u);
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    /* Load src[i] into FR1 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* FR0 = FR0 / FR1 = 1.0 / src */
    sljit_emit_fop2 (C, SLJIT_DIV_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* FLR: dst = floor(src) */
static
HRESULT
JitGenFlr (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Floor 4 floats component-wise using C library floorf via icall */
  for (i = 0; i < 4; i++) {
    /* Load src[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Call floorf(FR0) - FR0 is first float arg, result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(floorf));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* FRC: dst = frac(src) = src - floor(src) */
static
HRESULT
JitGenFrc (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Fractional 4 floats component-wise: dst = src - floor(src) */
  for (i = 0; i < 4; i++) {
    /* Load src[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Save original value to FR1 */
    sljit_emit_fop1 (C, SLJIT_MOV_F32,
      SLJIT_FR1, 0,
      SLJIT_FR0, 0);

    /* Call floorf(FR0) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(floorf));

    /* FR0 = FR1 - FR0 = src - floor(src) */
    sljit_emit_fop2 (C, SLJIT_SUB_F32,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0,
      SLJIT_FR0, 0);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* RSQ: dst = 1.0 / sqrt(src) */
static
HRESULT
JitGenRsq (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;
  union {
    float f;
    sljit_u32 u;
  } one;

  one.f = 1.0f;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Reciprocal square root 4 floats component-wise: dst = 1.0 / sqrt(src) */
  for (i = 0; i < 4; i++) {
    /* Load src[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Call sqrtf(FR0) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(sqrtf));

    /* Save sqrt result to FR1 */
    sljit_emit_fop1 (C, SLJIT_MOV_F32,
      SLJIT_FR1, 0,
      SLJIT_FR0, 0);

    /* Load constant 1.0 into FR0 */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, one.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    /* FR0 = FR0 / FR1 = 1.0 / sqrt(src) */
    sljit_emit_fop2 (C, SLJIT_DIV_F32,
      SLJIT_FR0, 0,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* CEIL: dst = ceil(src) */
static
HRESULT
JitGenCeil (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Ceiling 4 floats component-wise using C library ceilf via icall */
  for (i = 0; i < 4; i++) {
    /* Load src[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Call ceilf(FR0) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(ceilf));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* SQRT: dst = sqrt(src) */
static
HRESULT
JitGenSqrt (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Square root 4 floats component-wise using sqrtf via icall */
  for (i = 0; i < 4; i++) {
    /* Load src[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Call sqrtf(FR0) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(sqrtf));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* SLT: dst = (src1 < src2) ? 1.0 : 0.0 */
static
HRESULT
JitGenSlt (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump;
  union {
    float f;
    sljit_u32 u;
  } zero, one;

  zero.f = 0.0f;
  one.f = 1.0f;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* Set if less than: component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] and src2[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Compare: if FR0 < FR1, set result to 1.0, else 0.0 */
    sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_LESS,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    jump = sljit_emit_jump (C, SLJIT_LESS);

    /* FR0 >= FR1, load 0.0 */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, zero.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    sljit_set_label (jump, sljit_emit_label (C));

    /* FR0 < FR1, load 1.0 (jump target) */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, one.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* SGE: dst = (src1 >= src2) ? 1.0 : 0.0 */
static
HRESULT
JitGenSge (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump;
  union {
    float f;
    sljit_u32 u;
  } zero, one;

  zero.f = 0.0f;
  one.f = 1.0f;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* Set if greater or equal: component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] and src2[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Compare: if FR0 >= FR1, set result to 1.0, else 0.0 */
    sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_GREATER_EQUAL,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    jump = sljit_emit_jump (C, SLJIT_GREATER_EQUAL);

    /* FR0 < FR1, load 0.0 */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, zero.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    sljit_set_label (jump, sljit_emit_label (C));

    /* FR0 >= FR1, load 1.0 (jump target) */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, one.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* SEQ: dst = (src1 == src2) ? 1.0 : 0.0 */
static
HRESULT
JitGenSeq (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump;
  union {
    float f;
    sljit_u32 u;
  } zero, one;

  zero.f = 0.0f;
  one.f = 1.0f;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* Set if equal: component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] and src2[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Compare: if FR0 == FR1, set result to 1.0, else 0.0 */
    sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_F_EQUAL,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    jump = sljit_emit_jump (C, SLJIT_F_EQUAL);

    /* FR0 != FR1, load 0.0 */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, zero.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    sljit_set_label (jump, sljit_emit_label (C));

    /* FR0 == FR1, load 1.0 (jump target) */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, one.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* SNE: dst = (src1 != src2) ? 1.0 : 0.0 */
static
HRESULT
JitGenSne (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump;
  union {
    float f;
    sljit_u32 u;
  } zero, one;

  zero.f = 0.0f;
  one.f = 1.0f;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* Set if not equal: component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] and src2[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Compare: if FR0 != FR1, set result to 1.0, else 0.0 */
    sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_F_NOT_EQUAL,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    jump = sljit_emit_jump (C, SLJIT_F_NOT_EQUAL);

    /* FR0 == FR1, load 0.0 */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, zero.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    sljit_set_label (jump, sljit_emit_label (C));

    /* FR0 != FR1, load 1.0 (jump target) */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, one.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* SGT: dst = (src1 > src2) ? 1.0 : 0.0 */
static
HRESULT
JitGenSgt (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump;
  union {
    float f;
    sljit_u32 u;
  } zero, one;

  zero.f = 0.0f;
  one.f = 1.0f;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* Set if greater than: component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] and src2[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Compare: if FR0 > FR1, set result to 1.0, else 0.0 */
    sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_GREATER,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    jump = sljit_emit_jump (C, SLJIT_GREATER);

    /* FR0 <= FR1, load 0.0 */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, zero.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    sljit_set_label (jump, sljit_emit_label (C));

    /* FR0 > FR1, load 1.0 (jump target) */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, one.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* SLE: dst = (src1 <= src2) ? 1.0 : 0.0 */
static
HRESULT
JitGenSle (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump;
  union {
    float f;
    sljit_u32 u;
  } zero, one;

  zero.f = 0.0f;
  one.f = 1.0f;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* Set if less or equal: component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] and src2[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Compare: if FR0 <= FR1, set result to 1.0, else 0.0 */
    sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_LESS_EQUAL,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    jump = sljit_emit_jump (C, SLJIT_LESS_EQUAL);

    /* FR0 > FR1, load 0.0 */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, zero.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    sljit_set_label (jump, sljit_emit_label (C));

    /* FR0 <= FR1, load 1.0 (jump target) */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, one.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32,
      SLJIT_FR0,
      SLJIT_MEM1(SLJIT_SP), -16);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* CLAMP: dst = clamp(src1, src2, src3) = min(max(src1, src2), src3) */
static
HRESULT
JitGenClamp (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset, Src3Offset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump1, *jump2;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[2], &Src3Offset);
  if (FAILED (Result)) return Result;

  /* CLAMP 4 floats component-wise: dst = min(max(src1, src2), src3) */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    /* Load src2[i] (min value) into FR1 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Step 1: MAX(src1, src2) - if FR0 < FR1, use FR1 */
    sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_LESS,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    jump1 = sljit_emit_jump (C, SLJIT_LESS);

    /* FR0 >= FR1, keep FR0 - jump to after assignment */
    jump2 = sljit_emit_jump (C, SLJIT_JUMP);

    /* FR0 < FR1, use FR1 */
    sljit_set_label (jump1, sljit_emit_label (C));
    sljit_emit_fop1 (C, SLJIT_MOV_F32,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    /* Both paths converge here */
    sljit_set_label (jump2, sljit_emit_label (C));

    /* Load src3[i] (max value) into FR1 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src3Offset + i * 4);

    /* Step 2: MIN(result, src3) - if FR0 > FR1, use FR1 */
    sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_GREATER,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    jump1 = sljit_emit_jump (C, SLJIT_GREATER);

    /* FR0 <= FR1, keep FR0 - jump to store */
    jump2 = sljit_emit_jump (C, SLJIT_JUMP);

    /* FR0 > FR1, use FR1 */
    sljit_set_label (jump1, sljit_emit_label (C));
    sljit_emit_fop1 (C, SLJIT_MOV_F32,
      SLJIT_FR0, 0,
      SLJIT_FR1, 0);

    /* Both paths converge at store */
    sljit_set_label (jump2, sljit_emit_label (C));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* TRUNC: dst = trunc(src) - truncate towards zero */
static
HRESULT
JitGenTrunc (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Truncate 4 floats component-wise using C library truncf via icall */
  for (i = 0; i < 4; i++) {
    /* Load src[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Call truncf(FR0) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(truncf));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

//
// Main Instruction Compiler
//

static
HRESULT
JitCompileInstruction (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  switch (Inst->Opcode) {
    case VINIL_OP_MOV:
      return JitGenMov (Context, Inst);

    case VINIL_OP_ADD:
      return JitGenAdd (Context, Inst);

    case VINIL_OP_SUB:
      return JitGenSub (Context, Inst);

    case VINIL_OP_MUL:
      return JitGenMul (Context, Inst);

    case VINIL_OP_DIV:
      return JitGenDiv (Context, Inst);

    case VINIL_OP_MAD:
      return JitGenMad (Context, Inst);

    case VINIL_OP_NEG:
      return JitGenNeg (Context, Inst);

    case VINIL_OP_ABS:
      return JitGenAbs (Context, Inst);

    case VINIL_OP_MIN:
      return JitGenMin (Context, Inst);

    case VINIL_OP_MAX:
      return JitGenMax (Context, Inst);

    case VINIL_OP_RCP:
      return JitGenRcp (Context, Inst);

    case VINIL_OP_RSQ:
      return JitGenRsq (Context, Inst);

    case VINIL_OP_FLR:
      return JitGenFlr (Context, Inst);

    case VINIL_OP_FRC:
      return JitGenFrc (Context, Inst);

    case VINIL_OP_CEIL:
      return JitGenCeil (Context, Inst);

    case VINIL_OP_SQRT:
      return JitGenSqrt (Context, Inst);

    case VINIL_OP_SLT:
      return JitGenSlt (Context, Inst);

    case VINIL_OP_SGE:
      return JitGenSge (Context, Inst);

    case VINIL_OP_SEQ:
      return JitGenSeq (Context, Inst);

    case VINIL_OP_SNE:
      return JitGenSne (Context, Inst);

    case VINIL_OP_SGT:
      return JitGenSgt (Context, Inst);

    case VINIL_OP_SLE:
      return JitGenSle (Context, Inst);

    case VINIL_OP_CLAMP:
      return JitGenClamp (Context, Inst);

    case VINIL_OP_TRUNC:
      return JitGenTrunc (Context, Inst);

    case VINIL_OP_DP3:
      return JitGenDp3 (Context, Inst);

    case VINIL_OP_DP4:
      return JitGenDp4 (Context, Inst);

    case VINIL_OP_RET:
      return JitGenRet (Context, Inst);

    default:
      /* Unsupported opcode - fall back to interpreter */
      return E_NOTIMPL;
  }
}

//
// Public JIT Compilation Function
//

HRESULT
VinilJitCompileProgram (
  IVinilProgram  *Program,
  VOID           **CodePtr,
  UINTN          *CodeSize
  )
{
  VINIL_JIT_CONTEXT Context;
  VINIL_PROGRAM_IMPL *ProgramImpl = (VINIL_PROGRAM_IMPL *)Program;
  VINIL_INSTRUCTION_NODE *Inst;
  HRESULT Result;

  if (Program == NULL || CodePtr == NULL || CodeSize == NULL) {
    return E_POINTER;
  }

  /* Initialize JIT context */
  memset (&Context, 0, sizeof (Context));
  Context.Compiler = sljit_create_compiler (NULL);
  if (Context.Compiler == NULL) {
    return E_OUTOFMEMORY;
  }

  Context.Program = ProgramImpl;

  /* Generate prologue */
  Result = JitGeneratePrologue (&Context);
  if (FAILED (Result)) {
    sljit_free_compiler (Context.Compiler);
    return Result;
  }

  /* Compile all instructions */
  Inst = ProgramImpl->FirstInstruction;
  while (Inst != NULL) {
    Result = JitCompileInstruction (&Context, Inst);
    if (FAILED (Result)) {
      sljit_free_compiler (Context.Compiler);
      return Result;
    }
    Inst = Inst->Next;
  }

  /* Generate epilogue */
  Result = JitGenerateEpilogue (&Context);
  if (FAILED (Result)) {
    sljit_free_compiler (Context.Compiler);
    return Result;
  }

  /* Generate final code */
  Context.CodePtr = sljit_generate_code (Context.Compiler, 0, NULL);
  if (Context.CodePtr == NULL) {
    sljit_free_compiler (Context.Compiler);
    return E_FAIL;
  }

  Context.CodeSize = sljit_get_generated_code_size (Context.Compiler);

  /* Return results */
  *CodePtr = Context.CodePtr;
  *CodeSize = Context.CodeSize;

  /* Free compiler (but keep generated code) */
  sljit_free_compiler (Context.Compiler);

  return S_OK;
}

//
// Execute JIT-compiled code
//

HRESULT
VinilJitExecute (
  IVinilProgram  *Program,
  VOID           *Inputs,
  VOID           *Outputs
  )
{
  typedef void (*JIT_FUNCTION)(VINIL_EXECUTION_STATE *State);

  VINIL_EXECUTION_STATE State;
  VOID *CodePtr = NULL;
  UINTN CodeSize = 0;
  HRESULT Result;
  JIT_FUNCTION Execute;

  (void)Inputs;
  (void)Outputs;

  /* Compile program to native code */
  Result = VinilJitCompileProgram (Program, &CodePtr, &CodeSize);
  if (FAILED (Result)) {
    return Result;
  }

  /* Initialize execution state */
  memset (&State, 0, sizeof (State));
  State.Inputs = Inputs;
  State.Outputs = Outputs;

  /* Execute compiled code */
  Execute = (JIT_FUNCTION)CodePtr;
  Execute (&State);

  /* Free compiled code */
  sljit_free_code (CodePtr, NULL);

  return S_OK;
}
