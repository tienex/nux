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
#include <vinil/backend_ops.h>
#include "vinil_internal.h"

/* SLJIT configuration */
#define SLJIT_CONFIG_AUTO 1
#define SLJIT_VERBOSE 0
#include "../../sljit/sljit_src/sljitLir.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

//
// Extended SLJIT argument macros (5-6 arguments)
//

#ifndef SLJIT_ARGS5
#define SLJIT_ARGS5(ret, arg1, arg2, arg3, arg4, arg5) \
  (SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_##ret) \
   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_##arg1, 1) \
   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_##arg2, 2) \
   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_##arg3, 3) \
   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_##arg4, 4) \
   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_##arg5, 5))
#endif

#ifndef SLJIT_ARGS6
#define SLJIT_ARGS6(ret, arg1, arg2, arg3, arg4, arg5, arg6) \
  (SLJIT_ARG_RETURN(SLJIT_ARG_TYPE_##ret) \
   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_##arg1, 1) \
   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_##arg2, 2) \
   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_##arg3, 3) \
   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_##arg4, 4) \
   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_##arg5, 5) \
   | SLJIT_ARG_VALUE(SLJIT_ARG_TYPE_##arg6, 6))
#endif

//
// NTRTL Atomic Operations (from ananke/libs/ntrtl/arch/*/interlocked.S)
//

extern INT32  RtlAtomicFetchAdd32 (volatile INT32 *Ptr, INT32 Value);
extern INT32  RtlAtomicFetchSub32 (volatile INT32 *Ptr, INT32 Value);
extern UINT32 RtlAtomicFetchOr32  (volatile UINT32 *Ptr, UINT32 Value);
extern UINT32 RtlAtomicFetchAnd32 (volatile UINT32 *Ptr, UINT32 Value);
extern UINT32 RtlAtomicFetchXor32 (volatile UINT32 *Ptr, UINT32 Value);
extern UINT32 RtlAtomicExchange32 (volatile UINT32 *Ptr, UINT32 Value);
extern UINT32 RtlAtomicCompareExchange32 (volatile UINT32 *Ptr, UINT32 Expected, UINT32 Value);

//
// Memory Barrier (GCC builtin)
//

extern void __sync_synchronize (void);

//
// Texture Sampler Wrapper Functions (for >4 arg calls)
//

/* Wrapper for SampleLod (5 args) */
static
HRESULT
TexSampleLodWrapper (
  struct IVinilTextureSampler *This,
  UINT32                      Unit,
  CONST float                 *Coords,
  float                       Lod,
  float                       *Color
  )
{
  return This->lpVtbl->SampleLod (This, Unit, Coords, Lod, Color);
}

/* Wrapper for SampleBias (5 args) */
static
HRESULT
TexSampleBiasWrapper (
  struct IVinilTextureSampler *This,
  UINT32                      Unit,
  CONST float                 *Coords,
  float                       Bias,
  float                       *Color
  )
{
  return This->lpVtbl->SampleBias (This, Unit, Coords, Bias, Color);
}

/* Wrapper for SampleGrad (6 args) */
static
HRESULT
TexSampleGradWrapper (
  struct IVinilTextureSampler *This,
  UINT32                      Unit,
  CONST float                 *Coords,
  CONST float                 *DDx,
  CONST float                 *DDy,
  float                       *Color
  )
{
  return This->lpVtbl->SampleGrad (This, Unit, Coords, DDx, DDy, Color);
}

/* Wrapper for Fetch (5 args) */
static
HRESULT
TexFetchWrapper (
  struct IVinilTextureSampler *This,
  UINT32                      Unit,
  CONST INT32                 *Coords,
  INT32                       Lod,
  float                       *Color
  )
{
  return This->lpVtbl->Fetch (This, Unit, Coords, Lod, Color);
}

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

/* Control flow stack entry for JIT */
typedef struct {
  UINT32 Type;  /* VINIL_CF_IF, VINIL_CF_ELSE, VINIL_CF_LOOP */
  struct sljit_jump *IfFalseJump;  /* Jump when IF condition is false */
  struct sljit_jump *ElseSkipJump;  /* Jump to skip ELSE block */
  struct sljit_label *LoopStart;   /* Label for loop start */
  struct sljit_jump *BreakJumps[8];  /* Jumps for BREAK statements */
  UINT32 BreakCount;  /* Number of break jumps */
} VINIL_JIT_CF_ENTRY;

typedef struct {
  struct sljit_compiler *Compiler;
  VINIL_PROGRAM_IMPL *Program;

  /* Register to variable mapping */
  UINT32 VarToOffset[256];  /* Maps variable ID to register offset */

  /* Control flow stack for JIT compilation */
  VINIL_JIT_CF_ENTRY ControlFlowStack[32];
  UINT32 ControlFlowDepth;

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

/* ROUND: dst = round(src) */
static
HRESULT
JitGenRound (
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

  /* Round 4 floats component-wise using C library roundf via icall */
  for (i = 0; i < 4; i++) {
    /* Load src[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Call roundf(FR0) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(roundf));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* EXP2: dst = exp2(src) = 2^src */
static
HRESULT
JitGenExp2 (
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

  /* Exponential base 2: 4 floats component-wise using exp2f via icall */
  for (i = 0; i < 4; i++) {
    /* Load src[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Call exp2f(FR0) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(exp2f));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* LOG2: dst = log2(src) */
static
HRESULT
JitGenLog2 (
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

  /* Logarithm base 2: 4 floats component-wise using log2f via icall */
  for (i = 0; i < 4; i++) {
    /* Load src[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Call log2f(FR0) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(log2f));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* AND: dst = src1 & src2 (bitwise) */
static
HRESULT
JitGenAnd (
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

  /* Bitwise AND 4 words component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] as integer */
    sljit_emit_op1 (C, SLJIT_MOV,
      REG_TMP1, 0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    /* Load src2[i] as integer */
    sljit_emit_op1 (C, SLJIT_MOV,
      REG_TMP2, 0,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* AND operation */
    sljit_emit_op2 (C, SLJIT_AND,
      REG_TMP1, 0,
      REG_TMP1, 0,
      REG_TMP2, 0);

    /* Store result */
    sljit_emit_op1 (C, SLJIT_MOV,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4,
      REG_TMP1, 0);
  }

  return S_OK;
}

/* OR: dst = src1 | src2 (bitwise) */
static
HRESULT
JitGenOr (
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

  /* Bitwise OR 4 words component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] as integer */
    sljit_emit_op1 (C, SLJIT_MOV,
      REG_TMP1, 0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    /* Load src2[i] as integer */
    sljit_emit_op1 (C, SLJIT_MOV,
      REG_TMP2, 0,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* OR operation */
    sljit_emit_op2 (C, SLJIT_OR,
      REG_TMP1, 0,
      REG_TMP1, 0,
      REG_TMP2, 0);

    /* Store result */
    sljit_emit_op1 (C, SLJIT_MOV,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4,
      REG_TMP1, 0);
  }

  return S_OK;
}

/* XOR: dst = src1 ^ src2 (bitwise) */
static
HRESULT
JitGenXor (
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

  /* Bitwise XOR 4 words component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] as integer */
    sljit_emit_op1 (C, SLJIT_MOV,
      REG_TMP1, 0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    /* Load src2[i] as integer */
    sljit_emit_op1 (C, SLJIT_MOV,
      REG_TMP2, 0,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* XOR operation */
    sljit_emit_op2 (C, SLJIT_XOR,
      REG_TMP1, 0,
      REG_TMP1, 0,
      REG_TMP2, 0);

    /* Store result */
    sljit_emit_op1 (C, SLJIT_MOV,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4,
      REG_TMP1, 0);
  }

  return S_OK;
}

/* NOT: dst = ~src (bitwise) */
static
HRESULT
JitGenNot (
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

  /* Bitwise NOT 4 words component-wise */
  for (i = 0; i < 4; i++) {
    /* Load src[i] as integer */
    sljit_emit_op1 (C, SLJIT_MOV,
      REG_TMP1, 0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* NOT operation */
    sljit_emit_op2 (C, SLJIT_XOR,
      REG_TMP1, 0,
      REG_TMP1, 0,
      SLJIT_IMM, 0xFFFFFFFF);

    /* Store result */
    sljit_emit_op1 (C, SLJIT_MOV,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4,
      REG_TMP1, 0);
  }

  return S_OK;
}

/* SIN: dst = sin(src) */
static
HRESULT
JitGenSin (
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

  /* Sine 4 floats component-wise using sinf via icall */
  for (i = 0; i < 4; i++) {
    /* Load src[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Call sinf(FR0) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(sinf));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* COS: dst = cos(src) */
static
HRESULT
JitGenCos (
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

  /* Cosine 4 floats component-wise using cosf via icall */
  for (i = 0; i < 4; i++) {
    /* Load src[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Call cosf(FR0) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(cosf));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* POW: dst = pow(src1, src2) */
static
HRESULT
JitGenPow (
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

  /* Power 4 floats component-wise using powf via icall */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] (base) into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    /* Load src2[i] (exponent) into FR1 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Call powf(FR0, FR1) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS2(F32, F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(powf));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* DP2: dst = dot(src1.xy, src2.xy) */
static
HRESULT
JitGenDp2 (
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

  /* Accumulate Y */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR1,
    SLJIT_MEM1(REG_STATE), Src1Offset + 4);

  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR2,
    SLJIT_MEM1(REG_STATE), Src2Offset + 4);

  sljit_emit_fop2 (C, SLJIT_MUL_F32,
    SLJIT_FR1, 0,
    SLJIT_FR1, 0,
    SLJIT_FR2, 0);

  sljit_emit_fop2 (C, SLJIT_ADD_F32,
    SLJIT_FR0, 0,
    SLJIT_FR0, 0,
    SLJIT_FR1, 0);

  /* Store result in all 4 components */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* TAN: dst = tan(src) */
static
HRESULT
JitGenTan (
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

  /* Component-wise: dst[i] = tanf(src[i]) */
  for (i = 0; i < 4; i++) {
    /* Load src[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Call tanf */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32),
      SLJIT_IMM, SLJIT_FUNC_ADDR(tanf));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* ASIN: dst = asin(src) */
static
HRESULT
JitGenAsin (
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

  /* Component-wise: dst[i] = asinf(src[i]) */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32),
      SLJIT_IMM, SLJIT_FUNC_ADDR(asinf));
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* ACOS: dst = acos(src) */
static
HRESULT
JitGenAcos (
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

  /* Component-wise: dst[i] = acosf(src[i]) */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32),
      SLJIT_IMM, SLJIT_FUNC_ADDR(acosf));
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* ATAN: dst = atan(src) */
static
HRESULT
JitGenAtan (
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

  /* Component-wise: dst[i] = atanf(src[i]) */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32),
      SLJIT_IMM, SLJIT_FUNC_ADDR(atanf));
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* EXP: dst = exp(src) - natural exponential */
static
HRESULT
JitGenExp (
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

  /* Component-wise: dst[i] = expf(src[i]) */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32),
      SLJIT_IMM, SLJIT_FUNC_ADDR(expf));
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* LOG: dst = log(src) - natural logarithm */
static
HRESULT
JitGenLog (
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

  /* Component-wise: dst[i] = logf(src[i]) */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32),
      SLJIT_IMM, SLJIT_FUNC_ADDR(logf));
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* SELECT: dst = (src1 != 0) ? src2 : src3 - component-wise ternary select */
static
HRESULT
JitGenSelect (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset, Src3Offset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump_zero, *jump_end;
  union { float f; sljit_u32 u; } zero = { 0.0f };

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[2], &Src3Offset);
  if (FAILED (Result)) return Result;

  /* Component-wise: dst[i] = (src1[i] != 0.0) ? src2[i] : src3[i] */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] (condition) */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    /* Load 0.0 into FR1 */
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, zero.u);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), 0, SLJIT_R0, 0);
    sljit_emit_fmem (C, SLJIT_MOV_F32, SLJIT_FR1, SLJIT_MEM1(SLJIT_SP), 0);

    /* Compare: is src1[i] == 0.0? */
    sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_F_EQUAL, SLJIT_FR0, 0, SLJIT_FR1, 0);

    /* If equal to zero, jump to load src3 */
    jump_zero = sljit_emit_jump (C, SLJIT_F_EQUAL);

    /* Not zero: load src2[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);
    jump_end = sljit_emit_jump (C, SLJIT_JUMP);

    /* Zero: load src3[i] */
    sljit_set_label (jump_zero, sljit_emit_label (C));
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), Src3Offset + i * 4);

    /* Store result */
    sljit_set_label (jump_end, sljit_emit_label (C));
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* ATAN2: dst = atan2(src1, src2) - two-argument arc tangent */
static
HRESULT
JitGenAtan2 (
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

  /* Component-wise: dst[i] = atan2f(src1[i], src2[i]) */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] into FR0 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    /* Load src2[i] into FR1 */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1, SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Call atan2f(FR0, FR1) - result in FR0 */
    sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS2(F32, F32, F32),
      SLJIT_IMM, SLJIT_FUNC_ADDR(atan2f));

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* NOP: No operation */
static
HRESULT
JitGenNop (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  (void)Context;
  (void)Inst;

  /* NOP - do nothing, just return success */
  return S_OK;
}

/* MOVA: dst = src - move address (alias for MOV) */
static
HRESULT
JitGenMova (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  /* MOVA is identical to MOV in our implementation */
  return JitGenMov (Context, Inst);
}

/* SHUFFLE: dst[i] = src0[src1[i]] - component shuffle based on indices */
static
HRESULT
JitGenShuffle (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset, IdxOffset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump_cases[4];
  struct sljit_label *label_end;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &IdxOffset);
  if (FAILED (Result)) return Result;

  /* For each destination component, load index and shuffle */
  for (i = 0; i < 4; i++) {
    /* Load index[i] as integer */
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R0, 0,
      SLJIT_MEM1(REG_STATE), IdxOffset + i * 4);

    /* Mask to 0-3: R0 = R0 & 3 */
    sljit_emit_op2 (C, SLJIT_AND, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 3);

    /* Switch on index value to load correct component using conditional jumps */
    /* Compare with 0 */
    sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R1, 0, SLJIT_R0, 0, SLJIT_IMM, 0);
    jump_cases[0] = sljit_emit_jump (C, SLJIT_EQUAL);

    /* Compare with 1 */
    sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R1, 0, SLJIT_R0, 0, SLJIT_IMM, 1);
    jump_cases[1] = sljit_emit_jump (C, SLJIT_EQUAL);

    /* Compare with 2 */
    sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R1, 0, SLJIT_R0, 0, SLJIT_IMM, 2);
    jump_cases[2] = sljit_emit_jump (C, SLJIT_EQUAL);

    /* Default case 3 - load src[3] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset + 3 * 4);
    jump_cases[3] = sljit_emit_jump (C, SLJIT_JUMP);

    /* Case 2: load src[2] */
    sljit_set_label (jump_cases[2], sljit_emit_label (C));
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset + 2 * 4);
    struct sljit_jump *jump_to_end2 = sljit_emit_jump (C, SLJIT_JUMP);

    /* Case 1: load src[1] */
    sljit_set_label (jump_cases[1], sljit_emit_label (C));
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset + 1 * 4);
    struct sljit_jump *jump_to_end1 = sljit_emit_jump (C, SLJIT_JUMP);

    /* Case 0: load src[0] */
    sljit_set_label (jump_cases[0], sljit_emit_label (C));
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset + 0 * 4);

    /* End label */
    label_end = sljit_emit_label (C);
    sljit_set_label (jump_cases[3], label_end);
    sljit_set_label (jump_to_end2, label_end);
    sljit_set_label (jump_to_end1, label_end);

    /* Store result to dst[i] */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* DISCARD: Mark fragment for discard */
static
HRESULT
JitGenDiscard (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  (void)Inst;

  /*
   * Set State->Discarded = TRUE
   * Discarded is a BOOLEAN field in VINIL_EXECUTION_STATE
   * Offset calculation:
   * - Registers[256]: 256 * 16 = 4096 bytes
   * - Inputs pointer: 8 bytes
   * - Outputs pointer: 8 bytes
   * - GlobalId[3]: 12 bytes
   * - LocalId[3]: 12 bytes
   * - GroupId[3]: 12 bytes
   * - GlobalSize[3]: 12 bytes
   * - LocalSize[3]: 12 bytes
   * - NumGroups[3]: 12 bytes
   * - Discarded: BOOLEAN (1 byte)
   * Total offset: 4096 + 8 + 8 + 12*6 = 4184
   */
  #define DISCARDED_OFFSET  4184

  /* Store TRUE (1) to State->Discarded */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R0, 0, SLJIT_IMM, 1);
  sljit_emit_op1 (C, SLJIT_MOV_U8,
    SLJIT_MEM1(REG_STATE), DISCARDED_OFFSET, SLJIT_R0, 0);

  return S_OK;
}

/* GET_GLOBAL_ID: dst = GlobalId[src[0]] - get global work-item ID */
static
HRESULT
JitGenGetGlobalId (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  /*
   * Offset calculation for GlobalId[3]:
   * - Registers[256]: 4096 bytes
   * - Inputs pointer: 8 bytes
   * - Outputs pointer: 8 bytes
   * Total: 4112 bytes
   */
  #define GLOBAL_ID_OFFSET  4112

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Load dimension index from src[0] as float, convert to int */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset);

  /* Convert float to int (R0 = (int)FR0) */
  sljit_emit_fop1 (C, SLJIT_CONV_S32_FROM_F32, SLJIT_R0, 0, SLJIT_FR0, 0);

  /* Compute offset: GLOBAL_ID_OFFSET + Dim * 4 */
  sljit_emit_op2 (C, SLJIT_SHL, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 2); /* *4 */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, GLOBAL_ID_OFFSET);

  /* Load UINT32 value from State->GlobalId[dim] */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0,
    SLJIT_MEM2(REG_STATE, SLJIT_R0), 0);

  /* Convert UINT32 to float */
  sljit_emit_fop1 (C, SLJIT_CONV_F32_FROM_U32, SLJIT_FR0, 0, SLJIT_R1, 0);

  /* Broadcast to all 4 components */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* GET_LOCAL_ID: dst = LocalId[src[0]] - get local work-item ID */
static
HRESULT
JitGenGetLocalId (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  #define LOCAL_ID_OFFSET  4124

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Load dimension index from src[0] as float, convert to int */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_fop1 (C, SLJIT_CONV_S32_FROM_F32, SLJIT_R0, 0, SLJIT_FR0, 0);

  /* Compute offset */
  sljit_emit_op2 (C, SLJIT_SHL, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 2);
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, LOCAL_ID_OFFSET);

  /* Load and convert */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0, SLJIT_MEM2(REG_STATE, SLJIT_R0), 0);
  sljit_emit_fop1 (C, SLJIT_CONV_F32_FROM_U32, SLJIT_FR0, 0, SLJIT_R1, 0);

  /* Broadcast */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* GET_GROUP_ID: dst = GroupId[src[0]] - get work-group ID */
static
HRESULT
JitGenGetGroupId (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  #define GROUP_ID_OFFSET  4136

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_fop1 (C, SLJIT_CONV_S32_FROM_F32, SLJIT_R0, 0, SLJIT_FR0, 0);
  sljit_emit_op2 (C, SLJIT_SHL, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 2);
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, GROUP_ID_OFFSET);
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0, SLJIT_MEM2(REG_STATE, SLJIT_R0), 0);
  sljit_emit_fop1 (C, SLJIT_CONV_F32_FROM_U32, SLJIT_FR0, 0, SLJIT_R1, 0);

  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* GET_GLOBAL_SIZE: dst = GlobalSize[src[0]] - get global work size */
static
HRESULT
JitGenGetGlobalSize (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  #define GLOBAL_SIZE_OFFSET  4148

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_fop1 (C, SLJIT_CONV_S32_FROM_F32, SLJIT_R0, 0, SLJIT_FR0, 0);
  sljit_emit_op2 (C, SLJIT_SHL, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 2);
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, GLOBAL_SIZE_OFFSET);
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0, SLJIT_MEM2(REG_STATE, SLJIT_R0), 0);
  sljit_emit_fop1 (C, SLJIT_CONV_F32_FROM_U32, SLJIT_FR0, 0, SLJIT_R1, 0);

  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* GET_LOCAL_SIZE: dst = LocalSize[src[0]] - get local work size */
static
HRESULT
JitGenGetLocalSize (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  #define LOCAL_SIZE_OFFSET  4160

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_fop1 (C, SLJIT_CONV_S32_FROM_F32, SLJIT_R0, 0, SLJIT_FR0, 0);
  sljit_emit_op2 (C, SLJIT_SHL, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 2);
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, LOCAL_SIZE_OFFSET);
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0, SLJIT_MEM2(REG_STATE, SLJIT_R0), 0);
  sljit_emit_fop1 (C, SLJIT_CONV_F32_FROM_U32, SLJIT_FR0, 0, SLJIT_R1, 0);

  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* GET_NUM_GROUPS: dst = (GlobalSize[src[0]] + LocalSize[src[0]] - 1) / LocalSize[src[0]] */
static
HRESULT
JitGenGetNumGroups (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;

  #define NUM_GROUPS_OFFSET  4172

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Load dimension index */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_fop1 (C, SLJIT_CONV_S32_FROM_F32, SLJIT_R0, 0, SLJIT_FR0, 0);

  /* Calculate offset for arrays (dim * 4) */
  sljit_emit_op2 (C, SLJIT_SHL, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 2);

  /* Load GlobalSize[dim] into R1 */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, GLOBAL_SIZE_OFFSET);
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0, SLJIT_MEM2(REG_STATE, SLJIT_R2), 0);

  /* Load LocalSize[dim] into R2 */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, LOCAL_SIZE_OFFSET);
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R2, 0, SLJIT_MEM2(REG_STATE, SLJIT_R2), 0);

  /* Calculate: (GlobalSize + LocalSize - 1) / LocalSize */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R2, 0);
  sljit_emit_op2 (C, SLJIT_SUB, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 1);
  sljit_emit_op2 (C, SLJIT_DIV_UW, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R2, 0);

  /* Convert result to float */
  sljit_emit_fop1 (C, SLJIT_CONV_F32_FROM_U32, SLJIT_FR0, 0, SLJIT_R1, 0);

  /* Broadcast to all components */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* LOAD: dst = SharedMemory[src[0]] - load single float from shared memory */
static
HRESULT
JitGenLoad (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;

  /*
   * SharedMemory pointer offset in VINIL_EXECUTION_STATE:
   * - Registers[256]: 4096 bytes
   * - Pointers (Inputs, Outputs): 16 bytes
   * - Work-item IDs (6 arrays of 3 UINT32): 72 bytes
   * - Flags (Discarded, Returned): 2 bytes + 2 padding
   * - ControlFlowStack[32]: 512 bytes
   * - ControlFlowDepth: 4 bytes
   * - ConditionResult: 1 byte + 3 padding
   * - TextureSampler: 8 bytes
   * - SharedMemory: 8 bytes
   * Total: 4096 + 16 + 72 + 4 + 512 + 4 + 4 + 8 = 4716
   */
  #define SHARED_MEMORY_PTR_OFFSET  4716

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Load offset from src[0] (address register, first component as integer) */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), SrcOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer: R1 = SharedMemory + offset */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);

  /* Load float from memory */
  sljit_emit_fmem (C, SLJIT_MOV_F32, SLJIT_FR0, SLJIT_MEM1(SLJIT_R1), 0);

  /* Store to dst[0] only */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset);

  return S_OK;
}

/* STORE: SharedMemory[src[0]] = src[1] - store single float to shared memory */
static
HRESULT
JitGenStore (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw AddrOffset, ValOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &AddrOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &ValOffset);
  if (FAILED (Result)) return Result;

  /* Load offset from src[0] (address, first component as integer) */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), AddrOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);

  /* Load value from src[1][0] */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0, SLJIT_MEM1(REG_STATE), ValOffset);

  /* Store to memory */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE,
    SLJIT_FR0, SLJIT_MEM1(SLJIT_R1), 0);

  return S_OK;
}

/* LOAD_VEC: dst = SharedMemory[src[0]] - load vec4 (16 bytes) from shared memory */
static
HRESULT
JitGenLoadVec (
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

  /* Load offset from src[0] */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), SrcOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);

  /* Load 4 floats (16 bytes) from memory */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32, SLJIT_FR0, SLJIT_MEM1(SLJIT_R1), i * 4);
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* STORE_VEC: SharedMemory[src[0]] = src[1] - store vec4 (16 bytes) to shared memory */
static
HRESULT
JitGenStoreVec (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw AddrOffset, ValOffset;
  HRESULT Result;
  sljit_s32 i;

  Result = GetVariableOffset (Context, Inst->Src[0], &AddrOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &ValOffset);
  if (FAILED (Result)) return Result;

  /* Load offset from src[0] */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), AddrOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_R0, 0);

  /* Store 4 floats (16 bytes) to memory */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0, SLJIT_MEM1(REG_STATE), ValOffset + i * 4);
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE,
      SLJIT_FR0, SLJIT_MEM1(SLJIT_R1), i * 4);
  }

  return S_OK;
}

/* ATOMIC_ADD: dst = old_value; *addr += value (returns old value before add) */
static
HRESULT
JitGenAtomicAdd (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, AddrOffset, ValOffset;
  HRESULT Result;

  #define SHARED_MEMORY_PTR_OFFSET  4716

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &AddrOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &ValOffset);
  if (FAILED (Result)) return Result;

  /* Load address offset from src[0] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), AddrOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R2, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer to get final pointer in R0 */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R2, 0, SLJIT_R0, 0);

  /* Load value from src[1] first component into R1 */
  sljit_emit_op1 (C, SLJIT_MOV_S32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), ValOffset);

  /* Call RtlAtomicFetchAdd32(ptr, value) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS2(W, P, W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(RtlAtomicFetchAdd32));

  /* Return value is in R0, store to dst first component */
  sljit_emit_op1 (C, SLJIT_MOV,
    SLJIT_MEM1(REG_STATE), DstOffset, SLJIT_R0, 0);

  return S_OK;
}

/* ATOMIC_SUB: dst = old_value; *addr -= value (returns old value before sub) */
static
HRESULT
JitGenAtomicSub (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, AddrOffset, ValOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &AddrOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &ValOffset);
  if (FAILED (Result)) return Result;

  /* Load address offset from src[0] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), AddrOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R2, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R2, 0, SLJIT_R0, 0);

  /* Load value from src[1] first component */
  sljit_emit_op1 (C, SLJIT_MOV_S32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), ValOffset);

  /* Call RtlAtomicFetchSub32(ptr, value) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS2(W, P, W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(RtlAtomicFetchSub32));

  /* Return value in R0, store to dst */
  sljit_emit_op1 (C, SLJIT_MOV,
    SLJIT_MEM1(REG_STATE), DstOffset, SLJIT_R0, 0);

  return S_OK;
}

/* ATOMIC_AND: dst = old_value; *addr &= value (returns old value before AND) */
static
HRESULT
JitGenAtomicAnd (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, AddrOffset, ValOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &AddrOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &ValOffset);
  if (FAILED (Result)) return Result;

  /* Load address offset from src[0] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), AddrOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R2, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R2, 0, SLJIT_R0, 0);

  /* Load value from src[1] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), ValOffset);

  /* Call RtlAtomicFetchAnd32(ptr, value) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS2(W, P, W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(RtlAtomicFetchAnd32));

  /* Return value in R0, store to dst */
  sljit_emit_op1 (C, SLJIT_MOV,
    SLJIT_MEM1(REG_STATE), DstOffset, SLJIT_R0, 0);

  return S_OK;
}

/* ATOMIC_OR: dst = old_value; *addr |= value (returns old value before OR) */
static
HRESULT
JitGenAtomicOr (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, AddrOffset, ValOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &AddrOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &ValOffset);
  if (FAILED (Result)) return Result;

  /* Load address offset from src[0] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), AddrOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R2, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R2, 0, SLJIT_R0, 0);

  /* Load value from src[1] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), ValOffset);

  /* Call RtlAtomicFetchOr32(ptr, value) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS2(W, P, W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(RtlAtomicFetchOr32));

  /* Return value in R0, store to dst */
  sljit_emit_op1 (C, SLJIT_MOV,
    SLJIT_MEM1(REG_STATE), DstOffset, SLJIT_R0, 0);

  return S_OK;
}

/* ATOMIC_XOR: dst = old_value; *addr ^= value (returns old value before XOR) */
static
HRESULT
JitGenAtomicXor (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, AddrOffset, ValOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &AddrOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &ValOffset);
  if (FAILED (Result)) return Result;

  /* Load address offset from src[0] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), AddrOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R2, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R2, 0, SLJIT_R0, 0);

  /* Load value from src[1] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), ValOffset);

  /* Call RtlAtomicFetchXor32(ptr, value) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS2(W, P, W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(RtlAtomicFetchXor32));

  /* Return value in R0, store to dst */
  sljit_emit_op1 (C, SLJIT_MOV,
    SLJIT_MEM1(REG_STATE), DstOffset, SLJIT_R0, 0);

  return S_OK;
}

/* ATOMIC_XCHG: dst = old_value; *addr = value (exchange, returns old value) */
static
HRESULT
JitGenAtomicXchg (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, AddrOffset, ValOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &AddrOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &ValOffset);
  if (FAILED (Result)) return Result;

  /* Load address offset from src[0] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), AddrOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R2, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R2, 0, SLJIT_R0, 0);

  /* Load value from src[1] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), ValOffset);

  /* Call RtlAtomicExchange32(ptr, value) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS2(W, P, W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(RtlAtomicExchange32));

  /* Return value in R0, store to dst */
  sljit_emit_op1 (C, SLJIT_MOV,
    SLJIT_MEM1(REG_STATE), DstOffset, SLJIT_R0, 0);

  return S_OK;
}

/* ATOMIC_CAS: dst = old_value; if (*addr == compare) *addr = value (returns old value) */
static
HRESULT
JitGenAtomicCas (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, AddrOffset, CmpOffset, ValOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &AddrOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &CmpOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[2], &ValOffset);
  if (FAILED (Result)) return Result;

  /* Load address offset from src[0] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), AddrOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R3, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R0, 0, SLJIT_R3, 0, SLJIT_R0, 0);

  /* Load compare value from src[1] first component into R1 */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), CmpOffset);

  /* Load new value from src[2] first component into R2 */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R2, 0,
    SLJIT_MEM1(REG_STATE), ValOffset);

  /* Call RtlAtomicCompareExchange32(ptr, expected, value) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS3(W, P, W, W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(RtlAtomicCompareExchange32));

  /* Return value in R0, store to dst */
  sljit_emit_op1 (C, SLJIT_MOV,
    SLJIT_MEM1(REG_STATE), DstOffset, SLJIT_R0, 0);

  return S_OK;
}

/* ATOMIC_MIN: dst = old_value; *addr = min(*addr, value) using CAS loop */
static
HRESULT
JitGenAtomicMin (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, AddrOffset, ValOffset;
  HRESULT Result;
  struct sljit_label *loop_start;
  struct sljit_jump *jump_retry;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &AddrOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &ValOffset);
  if (FAILED (Result)) return Result;

  /* Load address offset from src[0] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R3, 0,
    SLJIT_MEM1(REG_STATE), AddrOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R4, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer - final pointer in R3 */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R3, 0, SLJIT_R4, 0, SLJIT_R3, 0);

  /* Load value from src[1] first component into R4 */
  sljit_emit_op1 (C, SLJIT_MOV_S32, SLJIT_R4, 0,
    SLJIT_MEM1(REG_STATE), ValOffset);

  /* Loop: do { old = *ptr; new = min(old, value); } while (CAS(ptr, old, new) != old) */
  loop_start = sljit_emit_label (C);

  /* Load old value from memory into R5 */
  sljit_emit_op1 (C, SLJIT_MOV_S32, SLJIT_R5, 0, SLJIT_MEM1(SLJIT_R3), 0);

  /* Calculate new = min(old, value): if (R4 < R5) new = R4 else new = R5 */
  /* Compare R4 (value) with R5 (old) */
  sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_SIG_LESS, SLJIT_R0, 0, SLJIT_R4, 0, SLJIT_R5, 0);

  /* Select: if (value < old) then value else old */
  sljit_emit_select (C, SLJIT_SIG_LESS, SLJIT_R2, SLJIT_R4, 0, SLJIT_R5);

  /* Call RtlAtomicCompareExchange32(ptr=R0, expected=R1, value=R2) */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R0, 0, SLJIT_R3, 0);  /* ptr */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R1, 0, SLJIT_R5, 0);  /* expected (old) */
  /* R2 already has new value from select */

  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS3(W, P, W, W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(RtlAtomicCompareExchange32));

  /* Compare return value (R0) with expected (R5) */
  sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R1, 0, SLJIT_R0, 0, SLJIT_R5, 0);

  /* If not equal, retry */
  jump_retry = sljit_emit_jump (C, SLJIT_NOT_EQUAL);
  sljit_set_label (jump_retry, loop_start);

  /* Store old value (in R5) to destination */
  sljit_emit_op1 (C, SLJIT_MOV,
    SLJIT_MEM1(REG_STATE), DstOffset, SLJIT_R5, 0);

  return S_OK;
}

/* ATOMIC_MAX: dst = old_value; *addr = max(*addr, value) using CAS loop */
static
HRESULT
JitGenAtomicMax (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, AddrOffset, ValOffset;
  HRESULT Result;
  struct sljit_label *loop_start;
  struct sljit_jump *jump_retry;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &AddrOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &ValOffset);
  if (FAILED (Result)) return Result;

  /* Load address offset from src[0] first component */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R3, 0,
    SLJIT_MEM1(REG_STATE), AddrOffset);

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R4, 0,
    SLJIT_MEM1(REG_STATE), SHARED_MEMORY_PTR_OFFSET);

  /* Add offset to base pointer - final pointer in R3 */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R3, 0, SLJIT_R4, 0, SLJIT_R3, 0);

  /* Load value from src[1] first component into R4 */
  sljit_emit_op1 (C, SLJIT_MOV_S32, SLJIT_R4, 0,
    SLJIT_MEM1(REG_STATE), ValOffset);

  /* Loop: do { old = *ptr; new = max(old, value); } while (CAS(ptr, old, new) != old) */
  loop_start = sljit_emit_label (C);

  /* Load old value from memory into R5 */
  sljit_emit_op1 (C, SLJIT_MOV_S32, SLJIT_R5, 0, SLJIT_MEM1(SLJIT_R3), 0);

  /* Calculate new = max(old, value): if (R4 > R5) new = R4 else new = R5 */
  /* Compare R4 (value) with R5 (old) */
  sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_SIG_GREATER, SLJIT_R0, 0, SLJIT_R4, 0, SLJIT_R5, 0);

  /* Select: if (value > old) then value else old */
  sljit_emit_select (C, SLJIT_SIG_GREATER, SLJIT_R2, SLJIT_R4, 0, SLJIT_R5);

  /* Call RtlAtomicCompareExchange32(ptr=R0, expected=R1, value=R2) */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R0, 0, SLJIT_R3, 0);  /* ptr */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R1, 0, SLJIT_R5, 0);  /* expected (old) */
  /* R2 already has new value from select */

  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS3(W, P, W, W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(RtlAtomicCompareExchange32));

  /* Compare return value (R0) with expected (R5) */
  sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R1, 0, SLJIT_R0, 0, SLJIT_R5, 0);

  /* If not equal, retry */
  jump_retry = sljit_emit_jump (C, SLJIT_NOT_EQUAL);
  sljit_set_label (jump_retry, loop_start);

  /* Store old value (in R5) to destination */
  sljit_emit_op1 (C, SLJIT_MOV,
    SLJIT_MEM1(REG_STATE), DstOffset, SLJIT_R5, 0);

  return S_OK;
}

/* BARRIER: Full memory barrier for work-group synchronization */
static
HRESULT
JitGenBarrier (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;

  (VOID)Inst;

  /* Emit call to __sync_synchronize() for full memory barrier */
  /* Function returns void, but we specify W (word) return and ignore it */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS0(W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(__sync_synchronize));

  return S_OK;
}

/* FENCE: Memory fence (sequential consistency) */
static
HRESULT
JitGenFence (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;

  (VOID)Inst;

  /* Emit call to __sync_synchronize() for memory fence */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS0(W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(__sync_synchronize));

  return S_OK;
}

/* MEM_FENCE: Memory fence (sequential consistency) */
static
HRESULT
JitGenMemFence (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;

  (VOID)Inst;

  /* Emit call to __sync_synchronize() for memory fence */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS0(W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(__sync_synchronize));

  return S_OK;
}

/* READ_FENCE: Acquire fence (read barrier) */
static
HRESULT
JitGenReadFence (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;

  (VOID)Inst;

  /* Use full barrier for safety in JIT context */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS0(W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(__sync_synchronize));

  return S_OK;
}

/* WRITE_FENCE: Release fence (write barrier) */
static
HRESULT
JitGenWriteFence (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;

  (VOID)Inst;

  /* Use full barrier for safety in JIT context */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS0(W),
    SLJIT_IMM, SLJIT_FUNC_ADDR(__sync_synchronize));

  return S_OK;
}

/* TEX: Sample texture with COM vtable dispatch */
/* dst = Sample(TextureSampler, unit, coords) */
static
HRESULT
JitGenTex (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, CoordOffset, UnitOffset;
  HRESULT Result;
  struct sljit_jump *skip_if_null;

  /*
   * Execution state layout (64-bit pointers):
   * Registers[256]: 0-4095 (4096 bytes)
   * Inputs: 4096-4103 (8 bytes)
   * Outputs: 4104-4111 (8 bytes)
   * GlobalId[3], LocalId[3], GroupId[3]: 4112-4147 (36 bytes)
   * GlobalSize[3], LocalSize[3], NumGroups[3]: 4148-4183 (36 bytes)
   * Discarded, Returned: 4184-4187 (4 bytes with padding)
   * ControlFlowStack[32]: 4188-4699 (512 bytes)
   * ControlFlowDepth: 4700-4703 (4 bytes)
   * ConditionResult: 4704-4707 (4 bytes with padding)
   * TextureSampler: 4708-4715 (8 bytes)
   * SharedMemory: 4716-4723 (8 bytes)
   */
  #define TEXTURE_SAMPLER_PTR_OFFSET  4708

  /* vtable offsets (64-bit pointers, 8 bytes each) */
  #define VTBL_SAMPLE_OFFSET  24  /* 3rd method after QueryInterface, AddRef, Release */

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &CoordOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &UnitOffset);
  if (FAILED (Result)) return Result;

  /* Load TextureSampler pointer into R0 */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), TEXTURE_SAMPLER_PTR_OFFSET);

  /* Check if TextureSampler is NULL - skip if so */
  sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R5, 0,
    SLJIT_R0, 0, SLJIT_IMM, 0);
  skip_if_null = sljit_emit_jump (C, SLJIT_EQUAL);

  /* Load vtable pointer from TextureSampler (first field, offset 0) */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_R0), 0);

  /* Load Sample function pointer from vtable (offset 24) */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R2, 0,
    SLJIT_MEM1(SLJIT_R1), VTBL_SAMPLE_OFFSET);

  /* Prepare arguments for Sample(this, unit, coords, color) */
  /* R0: this (TextureSampler) - already loaded */

  /* R1: unit (UINT32 from src1.x) */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), UnitOffset);

  /* R2: coords pointer (float *) - point to src0 in state */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R2, 0,
    REG_STATE, 0, SLJIT_IMM, CoordOffset);

  /* R3: color pointer (float *) - point to dst in state */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R3, 0,
    REG_STATE, 0, SLJIT_IMM, DstOffset);

  /* Load function pointer back into R4 for call */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R4, 0, SLJIT_MEM1(SLJIT_R1), VTBL_SAMPLE_OFFSET);

  /* Call Sample(TextureSampler, Unit, Coords, Color) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS4(W, P, W, P, P),
    SLJIT_R4, 0);

  /* Skip target for NULL TextureSampler */
  sljit_set_label (skip_if_null, sljit_emit_label (C));

  return S_OK;
}

/* TXL: Sample texture with LOD */
/* dst = SampleLod(TextureSampler, unit, coords, lod) */
static
HRESULT
JitGenTxl (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, CoordOffset, UnitOffset, LodOffset;
  HRESULT Result;
  struct sljit_jump *skip_if_null;

  #define TEXTURE_SAMPLER_PTR_OFFSET  4708
  #define VTBL_SAMPLELOD_OFFSET  32  /* 4th method */

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &CoordOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &UnitOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[2], &LodOffset);
  if (FAILED (Result)) return Result;

  /* Load TextureSampler pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), TEXTURE_SAMPLER_PTR_OFFSET);

  /* Skip if NULL */
  sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R5, 0,
    SLJIT_R0, 0, SLJIT_IMM, 0);
  skip_if_null = sljit_emit_jump (C, SLJIT_EQUAL);

  /* Prepare arguments: TexSampleLodWrapper(this, unit, coords, lod, color) */
  /* R0: this (already loaded) */

  /* R1: unit */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), UnitOffset);

  /* R2: coords pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R2, 0,
    REG_STATE, 0, SLJIT_IMM, CoordOffset);

  /* FR0: lod (float) */
  sljit_emit_fop1 (C, SLJIT_MOV_F32, SLJIT_FR0, 0,
    SLJIT_MEM1(REG_STATE), LodOffset);

  /* R3: color pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R3, 0,
    REG_STATE, 0, SLJIT_IMM, DstOffset);

  /* Call wrapper: TexSampleLodWrapper(this=R0, unit=R1, coords=R2, lod=FR0, color=R3) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS5(W, P, W, P, F32, P),
    SLJIT_IMM, SLJIT_FUNC_ADDR(TexSampleLodWrapper));

  sljit_set_label (skip_if_null, sljit_emit_label (C));

  return S_OK;
}

/* TXB: Sample texture with bias */
/* dst = SampleBias(TextureSampler, unit, coords, bias) */
static
HRESULT
JitGenTxb (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, CoordOffset, UnitOffset, BiasOffset;
  HRESULT Result;
  struct sljit_jump *skip_if_null;

  #define TEXTURE_SAMPLER_PTR_OFFSET  4708
  #define VTBL_SAMPLEBIAS_OFFSET  40  /* 5th method */

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &CoordOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &UnitOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[2], &BiasOffset);
  if (FAILED (Result)) return Result;

  /* Load TextureSampler pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), TEXTURE_SAMPLER_PTR_OFFSET);

  /* Skip if NULL */
  sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R5, 0,
    SLJIT_R0, 0, SLJIT_IMM, 0);
  skip_if_null = sljit_emit_jump (C, SLJIT_EQUAL);

  /* Prepare arguments: TexSampleBiasWrapper(this, unit, coords, bias, color) */
  /* R0: this (already loaded) */

  /* R1: unit */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), UnitOffset);

  /* R2: coords pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R2, 0,
    REG_STATE, 0, SLJIT_IMM, CoordOffset);

  /* FR0: bias (float) */
  sljit_emit_fop1 (C, SLJIT_MOV_F32, SLJIT_FR0, 0,
    SLJIT_MEM1(REG_STATE), BiasOffset);

  /* R3: color pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R3, 0,
    REG_STATE, 0, SLJIT_IMM, DstOffset);

  /* Call wrapper: TexSampleBiasWrapper(this=R0, unit=R1, coords=R2, bias=FR0, color=R3) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS5(W, P, W, P, F32, P),
    SLJIT_IMM, SLJIT_FUNC_ADDR(TexSampleBiasWrapper));

  sljit_set_label (skip_if_null, sljit_emit_label (C));

  return S_OK;
}

/* TXP: Sample texture with projection */
/* dst = SampleProj(TextureSampler, unit, coords) */
static
HRESULT
JitGenTxp (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, CoordOffset, UnitOffset;
  HRESULT Result;
  struct sljit_jump *skip_if_null;

  #define TEXTURE_SAMPLER_PTR_OFFSET  4708
  #define VTBL_SAMPLEPROJ_OFFSET  48  /* 6th method */

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &CoordOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &UnitOffset);
  if (FAILED (Result)) return Result;

  /* Load TextureSampler pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), TEXTURE_SAMPLER_PTR_OFFSET);

  /* Skip if NULL */
  sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R5, 0,
    SLJIT_R0, 0, SLJIT_IMM, 0);
  skip_if_null = sljit_emit_jump (C, SLJIT_EQUAL);

  /* Load vtable */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R1, 0, SLJIT_MEM1(SLJIT_R0), 0);

  /* Prepare arguments: SampleProj(this, unit, coords, color) */
  /* R0: this (already loaded) */

  /* R1: unit */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), UnitOffset);

  /* R2: coords pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R2, 0,
    REG_STATE, 0, SLJIT_IMM, CoordOffset);

  /* R3: color pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R3, 0,
    REG_STATE, 0, SLJIT_IMM, DstOffset);

  /* Load function pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R4, 0,
    SLJIT_MEM1(SLJIT_R1), VTBL_SAMPLEPROJ_OFFSET);

  /* Call SampleProj(TextureSampler, Unit, Coords, Color) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS4(W, P, W, P, P),
    SLJIT_R4, 0);

  sljit_set_label (skip_if_null, sljit_emit_label (C));

  return S_OK;
}

/* TXD: Sample texture with gradients */
/* dst = SampleGrad(TextureSampler, unit, coords, ddx, ddy) */
/* Note: unit is packed in coords.w component */
static
HRESULT
JitGenTxd (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, CoordOffset, DDxOffset, DDyOffset;
  HRESULT Result;
  struct sljit_jump *skip_if_null;

  #define TEXTURE_SAMPLER_PTR_OFFSET  4708
  #define VTBL_SAMPLEGRAD_OFFSET  56  /* 7th method */

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &CoordOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &DDxOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[2], &DDyOffset);
  if (FAILED (Result)) return Result;

  /* Load TextureSampler pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), TEXTURE_SAMPLER_PTR_OFFSET);

  /* Skip if NULL */
  sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R5, 0,
    SLJIT_R0, 0, SLJIT_IMM, 0);
  skip_if_null = sljit_emit_jump (C, SLJIT_EQUAL);

  /* Extract unit from coords.w (4th component, offset +12) and convert to UINT32 */
  sljit_emit_fop1 (C, SLJIT_MOV_F32, SLJIT_FR0, 0,
    SLJIT_MEM1(REG_STATE), CoordOffset + 12);
  sljit_emit_fop1 (C, SLJIT_CONV_S32_FROM_F32, SLJIT_R1, 0, SLJIT_FR0, 0);

  /* Prepare arguments: TexSampleGradWrapper(this, unit, coords, ddx, ddy, color) */
  /* R0: this (already loaded) */
  /* R1: unit (already loaded) */

  /* R2: coords pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R2, 0,
    REG_STATE, 0, SLJIT_IMM, CoordOffset);

  /* R3: ddx pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R3, 0,
    REG_STATE, 0, SLJIT_IMM, DDxOffset);

  /* Save R4 and R5 for additional args (ddy, color) */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R4, 0,
    REG_STATE, 0, SLJIT_IMM, DDyOffset);

  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R5, 0,
    REG_STATE, 0, SLJIT_IMM, DstOffset);

  /* Call wrapper: TexSampleGradWrapper(this=R0, unit=R1, coords=R2, ddx=R3, ddy=R4, color=R5) */
  /* Note: On x86-64, first 6 args go in registers RDI,RSI,RDX,RCX,R8,R9 */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS6(W, P, W, P, P, P, P),
    SLJIT_IMM, SLJIT_FUNC_ADDR(TexSampleGradWrapper));

  sljit_set_label (skip_if_null, sljit_emit_label (C));

  return S_OK;
}

/* TXF: Fetch texel at integer coordinates */
/* dst = Fetch(TextureSampler, unit, int_coords, int_lod) */
static
HRESULT
JitGenTxf (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, CoordOffset, UnitOffset, LodOffset;
  HRESULT Result;
  struct sljit_jump *skip_if_null;
  sljit_s32 i;

  #define TEXTURE_SAMPLER_PTR_OFFSET  4708
  #define VTBL_FETCH_OFFSET  64  /* 8th method */

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &CoordOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &UnitOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[2], &LodOffset);
  if (FAILED (Result)) return Result;

  /* Allocate 16 bytes on stack for INT32 coords[4] array */
  sljit_emit_op2 (C, SLJIT_SUB, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 16);

  /* Convert float coords to INT32 and store on stack */
  for (i = 0; i < 4; i++) {
    /* Load float component */
    sljit_emit_fop1 (C, SLJIT_MOV_F32, SLJIT_FR0, 0,
      SLJIT_MEM1(REG_STATE), CoordOffset + (i * 4));
    /* Convert to INT32 */
    sljit_emit_fop1 (C, SLJIT_CONV_S32_FROM_F32, SLJIT_R5, 0, SLJIT_FR0, 0);
    /* Store to stack */
    sljit_emit_op1 (C, SLJIT_MOV_S32, SLJIT_MEM1(SLJIT_SP), i * 4, SLJIT_R5, 0);
  }

  /* Load TextureSampler pointer */
  sljit_emit_op1 (C, SLJIT_MOV_P, SLJIT_R0, 0,
    SLJIT_MEM1(REG_STATE), TEXTURE_SAMPLER_PTR_OFFSET);

  /* Skip if NULL */
  sljit_emit_op2 (C, SLJIT_SUB | SLJIT_SET_Z, SLJIT_R5, 0,
    SLJIT_R0, 0, SLJIT_IMM, 0);
  skip_if_null = sljit_emit_jump (C, SLJIT_EQUAL);

  /* Prepare arguments: Fetch(this, unit, coords, lod, color) */
  /* R0: this */

  /* R1: unit */
  sljit_emit_op1 (C, SLJIT_MOV_U32, SLJIT_R1, 0,
    SLJIT_MEM1(REG_STATE), UnitOffset);

  /* R2: coords pointer (stack pointer) */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R2, 0, SLJIT_SP, 0);

  /* R3: lod (INT32 from float) */
  sljit_emit_fop1 (C, SLJIT_MOV_F32, SLJIT_FR0, 0,
    SLJIT_MEM1(REG_STATE), LodOffset);
  sljit_emit_fop1 (C, SLJIT_CONV_S32_FROM_F32, SLJIT_R3, 0, SLJIT_FR0, 0);

  /* R4: color pointer */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_R4, 0,
    REG_STATE, 0, SLJIT_IMM, DstOffset);

  /* Call wrapper: TexFetchWrapper(this=R0, unit=R1, coords=R2, lod=R3, color=R4) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS5(W, P, W, P, W, P),
    SLJIT_IMM, SLJIT_FUNC_ADDR(TexFetchWrapper));

  sljit_set_label (skip_if_null, sljit_emit_label (C));

  /* Restore stack (deallocate coords array) */
  sljit_emit_op2 (C, SLJIT_ADD, SLJIT_SP, 0, SLJIT_SP, 0, SLJIT_IMM, 16);

  return S_OK;
}

/* IF: Begin conditional block */
/* Evaluates condition and jumps to ELSE/ENDIF if false */
static
HRESULT
JitGenIf (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw CondOffset;
  HRESULT Result;
  struct sljit_jump *jump_if_false;

  if (Context->ControlFlowDepth >= 32) {
    return E_FAIL;  /* Control flow stack overflow */
  }

  Result = GetVariableOffset (Context, Inst->Src[0], &CondOffset);
  if (FAILED (Result)) return Result;

  /* Load condition from first component (can be float, int, or bool) */
  /* Check if condition != 0.0f */
  sljit_emit_fop1 (C, SLJIT_MOV_F32, SLJIT_FR0, 0,
    SLJIT_MEM1(REG_STATE), CondOffset);
  sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_ORDERED_EQUAL, SLJIT_FR0, 0,
    SLJIT_MEM0(), (sljit_sw)&(float){0.0f});

  /* Jump to ELSE/ENDIF if condition is false (== 0.0f) */
  jump_if_false = sljit_emit_jump (C, SLJIT_ORDERED_EQUAL);

  /* Push to control flow stack */
  Context->ControlFlowStack[Context->ControlFlowDepth].Type = VINIL_CF_IF;
  Context->ControlFlowStack[Context->ControlFlowDepth].IfFalseJump = jump_if_false;
  Context->ControlFlowStack[Context->ControlFlowDepth].ElseSkipJump = NULL;
  Context->ControlFlowStack[Context->ControlFlowDepth].BreakCount = 0;
  Context->ControlFlowDepth++;

  return S_OK;
}

/* ELSE: Begin else block */
/* Patches IF jump, creates jump to skip else block */
static
HRESULT
JitGenElse (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  struct sljit_jump *skip_else;
  struct sljit_label *else_label;

  (VOID)Inst;

  if (Context->ControlFlowDepth == 0) {
    return E_FAIL;  /* ELSE without IF */
  }

  if (Context->ControlFlowStack[Context->ControlFlowDepth - 1].Type != VINIL_CF_IF) {
    return E_FAIL;  /* ELSE without matching IF */
  }

  /* Create jump to skip else block (when then-block was executed) */
  skip_else = sljit_emit_jump (C, SLJIT_JUMP);

  /* Mark this location as the else block start */
  else_label = sljit_emit_label (C);

  /* Patch the IF's false jump to point here */
  sljit_set_label (Context->ControlFlowStack[Context->ControlFlowDepth - 1].IfFalseJump, else_label);

  /* Update stack entry */
  Context->ControlFlowStack[Context->ControlFlowDepth - 1].Type = VINIL_CF_ELSE;
  Context->ControlFlowStack[Context->ControlFlowDepth - 1].ElseSkipJump = skip_else;

  return S_OK;
}

/* ENDIF: End conditional block */
/* Patches all pending jumps and pops control flow stack */
static
HRESULT
JitGenEndif (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  struct sljit_label *endif_label;
  VINIL_JIT_CF_ENTRY *entry;

  (VOID)Inst;

  if (Context->ControlFlowDepth == 0) {
    return E_FAIL;  /* ENDIF without IF */
  }

  entry = &Context->ControlFlowStack[Context->ControlFlowDepth - 1];

  if (entry->Type != VINIL_CF_IF && entry->Type != VINIL_CF_ELSE) {
    return E_FAIL;  /* ENDIF without matching IF/ELSE */
  }

  /* Mark this location as the endif */
  endif_label = sljit_emit_label (C);

  /* Patch jumps depending on whether there was an ELSE */
  if (entry->Type == VINIL_CF_ELSE) {
    /* Patch the else-skip jump to point here */
    sljit_set_label (entry->ElseSkipJump, endif_label);
  } else {
    /* No ELSE - patch the if-false jump to point here */
    sljit_set_label (entry->IfFalseJump, endif_label);
  }

  /* Pop control flow stack */
  Context->ControlFlowDepth--;

  return S_OK;
}

/* LOOP: Begin loop block */
/* Creates a label for the loop start */
static
HRESULT
JitGenLoop (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  struct sljit_label *loop_start;

  (VOID)Inst;

  if (Context->ControlFlowDepth >= 32) {
    return E_FAIL;  /* Control flow stack overflow */
  }

  /* Mark this location as the loop start */
  loop_start = sljit_emit_label (C);

  /* Push to control flow stack */
  Context->ControlFlowStack[Context->ControlFlowDepth].Type = VINIL_CF_LOOP;
  Context->ControlFlowStack[Context->ControlFlowDepth].LoopStart = loop_start;
  Context->ControlFlowStack[Context->ControlFlowDepth].BreakCount = 0;
  Context->ControlFlowStack[Context->ControlFlowDepth].IfFalseJump = NULL;
  Context->ControlFlowStack[Context->ControlFlowDepth].ElseSkipJump = NULL;
  Context->ControlFlowDepth++;

  return S_OK;
}

/* ENDLOOP: End loop block */
/* Jumps back to loop start and patches break jumps */
static
HRESULT
JitGenEndloop (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  struct sljit_jump *jump_to_start;
  struct sljit_label *endloop_label;
  VINIL_JIT_CF_ENTRY *entry;
  UINT32 i;

  (VOID)Inst;

  if (Context->ControlFlowDepth == 0) {
    return E_FAIL;  /* ENDLOOP without LOOP */
  }

  entry = &Context->ControlFlowStack[Context->ControlFlowDepth - 1];

  if (entry->Type != VINIL_CF_LOOP) {
    return E_FAIL;  /* ENDLOOP without matching LOOP */
  }

  /* Unconditional jump back to loop start */
  jump_to_start = sljit_emit_jump (C, SLJIT_JUMP);
  sljit_set_label (jump_to_start, entry->LoopStart);

  /* Mark this location as end of loop (for break jumps) */
  endloop_label = sljit_emit_label (C);

  /* Patch all break jumps to point here */
  for (i = 0; i < entry->BreakCount; i++) {
    sljit_set_label (entry->BreakJumps[i], endloop_label);
  }

  /* Pop control flow stack */
  Context->ControlFlowDepth--;

  return S_OK;
}

/* BREAK: Break from loop */
/* Creates a forward jump to be patched by ENDLOOP */
static
HRESULT
JitGenBreak (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  struct sljit_jump *break_jump;
  VINIL_JIT_CF_ENTRY *entry;
  UINT32 i;

  (VOID)Inst;

  /* Find the innermost loop */
  for (i = Context->ControlFlowDepth; i > 0; i--) {
    if (Context->ControlFlowStack[i - 1].Type == VINIL_CF_LOOP) {
      entry = &Context->ControlFlowStack[i - 1];

      /* Check if we have space for more breaks */
      if (entry->BreakCount >= 8) {
        return E_FAIL;  /* Too many breaks in one loop */
      }

      /* Create jump to end of loop */
      break_jump = sljit_emit_jump (C, SLJIT_JUMP);

      /* Add to break list (will be patched by ENDLOOP) */
      entry->BreakJumps[entry->BreakCount] = break_jump;
      entry->BreakCount++;

      return S_OK;
    }
  }

  return E_FAIL;  /* BREAK without enclosing LOOP */
}

/* CONTINUE: Continue to next loop iteration */
/* Jumps back to loop start */
static
HRESULT
JitGenContinue (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  struct sljit_jump *continue_jump;
  VINIL_JIT_CF_ENTRY *entry;
  UINT32 i;

  (VOID)Inst;

  /* Find the innermost loop */
  for (i = Context->ControlFlowDepth; i > 0; i--) {
    if (Context->ControlFlowStack[i - 1].Type == VINIL_CF_LOOP) {
      entry = &Context->ControlFlowStack[i - 1];

      /* Create jump back to loop start */
      continue_jump = sljit_emit_jump (C, SLJIT_JUMP);
      sljit_set_label (continue_jump, entry->LoopStart);

      return S_OK;
    }
  }

  return E_FAIL;  /* CONTINUE without enclosing LOOP */
}

/* SHL: dst = src1 << src2 (logical left shift) */
static
HRESULT
JitGenShl (
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

  /* Component-wise shift left: dst[i] = src1[i] << src2[i] */
  for (i = 0; i < 4; i++) {
    /* Load src1[i] as integer */
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R0, 0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);

    /* Load src2[i] as integer (shift amount) */
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R1, 0,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);

    /* Shift left: R0 = R0 << R1 */
    sljit_emit_op2 (C, SLJIT_SHL, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);

    /* Store result */
    sljit_emit_op1 (C, SLJIT_MOV,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4, SLJIT_R0, 0);
  }

  return S_OK;
}

/* SHR: dst = src1 >> src2 (logical right shift) */
static
HRESULT
JitGenShr (
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

  /* Component-wise unsigned shift right: dst[i] = src1[i] >> src2[i] */
  for (i = 0; i < 4; i++) {
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R0, 0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R1, 0,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);
    sljit_emit_op2 (C, SLJIT_LSHR, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    sljit_emit_op1 (C, SLJIT_MOV,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4, SLJIT_R0, 0);
  }

  return S_OK;
}

/* SAR: dst = src1 >> src2 (arithmetic right shift, sign-extended) */
static
HRESULT
JitGenSar (
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

  /* Component-wise signed shift right: dst[i] = src1[i] >> src2[i] (signed) */
  for (i = 0; i < 4; i++) {
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R0, 0,
      SLJIT_MEM1(REG_STATE), Src1Offset + i * 4);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_R1, 0,
      SLJIT_MEM1(REG_STATE), Src2Offset + i * 4);
    sljit_emit_op2 (C, SLJIT_ASHR, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    sljit_emit_op1 (C, SLJIT_MOV,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4, SLJIT_R0, 0);
  }

  return S_OK;
}

/* CRS: dst = cross(src1, src2) - 3D cross product */
static
HRESULT
JitGenCrs (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;
  union {
    float f;
    sljit_u32 u;
  } zero;

  zero.f = 0.0f;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  /* Cross product: a x b = (a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x, 0) */

  /* X component: a.y*b.z - a.z*b.y */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0,
    SLJIT_MEM1(REG_STATE), Src1Offset + 4);  /* a.y */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR1,
    SLJIT_MEM1(REG_STATE), Src2Offset + 8);  /* b.z */
  sljit_emit_fop2 (C, SLJIT_MUL_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);

  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR1,
    SLJIT_MEM1(REG_STATE), Src1Offset + 8);  /* a.z */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR2,
    SLJIT_MEM1(REG_STATE), Src2Offset + 4);  /* b.y */
  sljit_emit_fop2 (C, SLJIT_MUL_F32, SLJIT_FR1, 0, SLJIT_FR1, 0, SLJIT_FR2, 0);
  sljit_emit_fop2 (C, SLJIT_SUB_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0,
    SLJIT_MEM1(REG_STATE), DstOffset + 0);

  /* Y component: a.z*b.x - a.x*b.z */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0,
    SLJIT_MEM1(REG_STATE), Src1Offset + 8);  /* a.z */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR1,
    SLJIT_MEM1(REG_STATE), Src2Offset + 0);  /* b.x */
  sljit_emit_fop2 (C, SLJIT_MUL_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);

  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR1,
    SLJIT_MEM1(REG_STATE), Src1Offset + 0);  /* a.x */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR2,
    SLJIT_MEM1(REG_STATE), Src2Offset + 8);  /* b.z */
  sljit_emit_fop2 (C, SLJIT_MUL_F32, SLJIT_FR1, 0, SLJIT_FR1, 0, SLJIT_FR2, 0);
  sljit_emit_fop2 (C, SLJIT_SUB_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0,
    SLJIT_MEM1(REG_STATE), DstOffset + 4);

  /* Z component: a.x*b.y - a.y*b.x */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0,
    SLJIT_MEM1(REG_STATE), Src1Offset + 0);  /* a.x */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR1,
    SLJIT_MEM1(REG_STATE), Src2Offset + 4);  /* b.y */
  sljit_emit_fop2 (C, SLJIT_MUL_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);

  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR1,
    SLJIT_MEM1(REG_STATE), Src1Offset + 4);  /* a.y */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR2,
    SLJIT_MEM1(REG_STATE), Src2Offset + 0);  /* b.x */
  sljit_emit_fop2 (C, SLJIT_MUL_F32, SLJIT_FR1, 0, SLJIT_FR1, 0, SLJIT_FR2, 0);
  sljit_emit_fop2 (C, SLJIT_SUB_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0,
    SLJIT_MEM1(REG_STATE), DstOffset + 8);

  /* W component: 0.0 */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, zero.u);
  sljit_emit_op1 (C, SLJIT_MOV,
    SLJIT_MEM1(REG_STATE), DstOffset + 12,
    REG_TMP1, 0);

  return S_OK;
}

/* LEN: dst = length(src) = sqrt(dot(src, src)) */
static
HRESULT
JitGenLen (
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

  /* Calculate length: sqrt(x*x + y*y + z*z + w*w) */

  /* X*X */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0,
    SLJIT_MEM1(REG_STATE), SrcOffset + 0);
  sljit_emit_fop2 (C, SLJIT_MUL_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR0, 0);

  /* Y*Y + accumulate */
  for (i = 1; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);
    sljit_emit_fop2 (C, SLJIT_MUL_F32, SLJIT_FR1, 0, SLJIT_FR1, 0, SLJIT_FR1, 0);
    sljit_emit_fop2 (C, SLJIT_ADD_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
  }

  /* sqrt(sum) */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(sqrtf));

  /* Store result in all 4 components */
  for (i = 0; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR0,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  return S_OK;
}

/* NRM: dst = normalize(src) = src / length(src) */
static
HRESULT
JitGenNrm (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;
  sljit_s32 i;
  struct sljit_jump *jump_nonzero, *jump_end;
  union {
    float f;
    sljit_u32 u;
  } zero;

  zero.f = 0.0f;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Calculate length: sqrt(x*x + y*y + z*z + w*w) */

  /* X*X */
  sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
    SLJIT_FR0,
    SLJIT_MEM1(REG_STATE), SrcOffset + 0);
  sljit_emit_fop2 (C, SLJIT_MUL_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR0, 0);

  /* Y*Y, Z*Z, W*W + accumulate */
  for (i = 1; i < 4; i++) {
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);
    sljit_emit_fop2 (C, SLJIT_MUL_F32, SLJIT_FR1, 0, SLJIT_FR1, 0, SLJIT_FR1, 0);
    sljit_emit_fop2 (C, SLJIT_ADD_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);
  }

  /* sqrt(sum) - length in FR0 */
  sljit_emit_icall (C, SLJIT_CALL, SLJIT_ARGS1(F32, F32), SLJIT_IMM, SLJIT_FUNC_ADDR(sqrtf));

  /* Check if length > 0 */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, zero.u);
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(SLJIT_SP), -16, REG_TMP1, 0);
  sljit_emit_fmem (C, SLJIT_MOV_F32,
    SLJIT_FR1,
    SLJIT_MEM1(SLJIT_SP), -16);

  sljit_emit_fop1 (C, SLJIT_CMP_F32 | SLJIT_SET_GREATER,
    SLJIT_FR0, 0,
    SLJIT_FR1, 0);

  jump_nonzero = sljit_emit_jump (C, SLJIT_GREATER);

  /* Length == 0, set all components to 0.0 */
  for (i = 0; i < 4; i++) {
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_IMM, zero.u);
    sljit_emit_op1 (C, SLJIT_MOV,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4,
      REG_TMP1, 0);
  }

  jump_end = sljit_emit_jump (C, SLJIT_JUMP);

  /* Length > 0, normalize: divide each component by length */
  sljit_set_label (jump_nonzero, sljit_emit_label (C));

  for (i = 0; i < 4; i++) {
    /* Load src component */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);

    /* Divide by length */
    sljit_emit_fop2 (C, SLJIT_DIV_F32,
      SLJIT_FR1, 0,
      SLJIT_FR1, 0,
      SLJIT_FR0, 0);

    /* Store result */
    sljit_emit_fmem (C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16,
      SLJIT_FR1,
      SLJIT_MEM1(REG_STATE), DstOffset + i * 4);
  }

  sljit_set_label (jump_end, sljit_emit_label (C));

  return S_OK;
}

//
// Bytecode Extension Opcodes - JIT Generators
//

/* PUSH: Push register value onto stack */
static
HRESULT
JitGenPush (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw SrcOffset;
  HRESULT Result;
  sljit_sw SPOffset = offsetof(VINIL_EXECUTION_STATE, SP);
  sljit_sw StackOffset = offsetof(VINIL_EXECUTION_STATE, Stack);

  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Load SP */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SPOffset);

  /* SP -= sizeof(VINIL_REGISTER_VALUE) = 16 */
  sljit_emit_op2 (C, SLJIT_SUB, REG_TMP1, 0, REG_TMP1, 0, SLJIT_IMM, 16);

  /* Store updated SP */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), SPOffset, REG_TMP1, 0);

  /* Copy 16 bytes from Src to Stack[SP] */
  /* REG_TMP1 = SP, REG_TMP2 = Stack base address + SP */
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP2, 0, SLJIT_IMM, StackOffset, REG_TMP1, 0);
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP2, 0, REG_STATE, 0, REG_TMP2, 0);

  /* Copy 4 words (16 bytes) */
  for (int i = 0; i < 4; i++) {
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP3, 0, SLJIT_MEM1(REG_STATE), SrcOffset + i * 4);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_TMP2), i * 4, REG_TMP3, 0);
  }

  return S_OK;
}

/* POP: Pop value from stack into register */
static
HRESULT
JitGenPop (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset;
  HRESULT Result;
  sljit_sw SPOffset = offsetof(VINIL_EXECUTION_STATE, SP);
  sljit_sw StackOffset = offsetof(VINIL_EXECUTION_STATE, Stack);

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;

  /* Load SP */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SPOffset);

  /* REG_TMP2 = Stack base + SP */
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP2, 0, SLJIT_IMM, StackOffset, REG_TMP1, 0);
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP2, 0, REG_STATE, 0, REG_TMP2, 0);

  /* Copy 16 bytes from Stack[SP] to Dst */
  for (int i = 0; i < 4; i++) {
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP3, 0, SLJIT_MEM1(REG_TMP2), i * 4);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset + i * 4, REG_TMP3, 0);
  }

  /* SP += 16 */
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP1, 0, REG_TMP1, 0, SLJIT_IMM, 16);
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), SPOffset, REG_TMP1, 0);

  return S_OK;
}

/* DUP: Duplicate top of stack */
static
HRESULT
JitGenDup (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw SPOffset = offsetof(VINIL_EXECUTION_STATE, SP);
  sljit_sw StackOffset = offsetof(VINIL_EXECUTION_STATE, Stack);

  (void)Inst;

  /* Load SP */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SPOffset);

  /* SP -= 16 */
  sljit_emit_op2 (C, SLJIT_SUB, REG_TMP1, 0, REG_TMP1, 0, SLJIT_IMM, 16);

  /* Store new SP */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), SPOffset, REG_TMP1, 0);

  /* REG_TMP2 = Stack base + SP */
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP2, 0, SLJIT_IMM, StackOffset, REG_TMP1, 0);
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP2, 0, REG_STATE, 0, REG_TMP2, 0);

  /* Copy from [SP+16] to [SP] */
  for (int i = 0; i < 4; i++) {
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP3, 0, SLJIT_MEM1(REG_TMP2), 16 + i * 4);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_TMP2), i * 4, REG_TMP3, 0);
  }

  return S_OK;
}

/* SWAP: Swap top two stack values */
static
HRESULT
JitGenSwap (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw SPOffset = offsetof(VINIL_EXECUTION_STATE, SP);
  sljit_sw StackOffset = offsetof(VINIL_EXECUTION_STATE, Stack);

  (void)Inst;

  /* Load SP */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SPOffset);

  /* REG_TMP2 = Stack base + SP */
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP2, 0, SLJIT_IMM, StackOffset, REG_TMP1, 0);
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP2, 0, REG_STATE, 0, REG_TMP2, 0);

  /* Swap: exchange [SP] <-> [SP+16] */
  for (int i = 0; i < 4; i++) {
    /* Load both values */
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_TMP2), i * 4);
    sljit_emit_op1 (C, SLJIT_MOV, REG_TMP3, 0, SLJIT_MEM1(REG_TMP2), 16 + i * 4);
    /* Store swapped */
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_TMP2), i * 4, REG_TMP3, 0);
    sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_TMP2), 16 + i * 4, REG_TMP1, 0);
  }

  return S_OK;
}

/* ZEXT8: Zero extend 8-bit to 32-bit */
static
HRESULT
JitGenZext8 (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Load source, mask to 8 bits, store */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_op2 (C, SLJIT_AND, REG_TMP1, 0, REG_TMP1, 0, SLJIT_IMM, 0xFF);
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  return S_OK;
}

/* ZEXT16: Zero extend 16-bit to 32-bit */
static
HRESULT
JitGenZext16 (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_op2 (C, SLJIT_AND, REG_TMP1, 0, REG_TMP1, 0, SLJIT_IMM, 0xFFFF);
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  return S_OK;
}

/* ZEXT32: Zero extend 32-bit to 64-bit */
static
HRESULT
JitGenZext32 (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Copy 32-bit value to dst[0], zero dst[1] */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset + 4, SLJIT_IMM, 0);

  return S_OK;
}

/* SEXT8: Sign extend 8-bit to 32-bit */
static
HRESULT
JitGenSext8 (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Load, sign-extend from 8-bit */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_op1 (C, SLJIT_MOV_S8, REG_TMP1, 0, REG_TMP1, 0);
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  return S_OK;
}

/* SEXT16: Sign extend 16-bit to 32-bit */
static
HRESULT
JitGenSext16 (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_op1 (C, SLJIT_MOV_S16, REG_TMP1, 0, REG_TMP1, 0);
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  return S_OK;
}

/* SEXT32: Sign extend 32-bit to 64-bit */
static
HRESULT
JitGenSext32 (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Load 32-bit value */
  sljit_emit_op1 (C, SLJIT_MOV_S32, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SrcOffset);

  /* Store low 32 bits */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  /* Arithmetic shift right 31 bits to get sign extension */
  sljit_emit_op2 (C, SLJIT_ASHR, REG_TMP1, 0, REG_TMP1, 0, SLJIT_IMM, 31);

  /* Store high 32 bits (all 0s or all 1s) */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset + 4, REG_TMP1, 0);

  return S_OK;
}

/* TRUNC8: Truncate to 8-bit */
static
HRESULT
JitGenTrunc8 (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_op2 (C, SLJIT_AND, REG_TMP1, 0, REG_TMP1, 0, SLJIT_IMM, 0xFF);
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  return S_OK;
}

/* TRUNC16: Truncate to 16-bit */
static
HRESULT
JitGenTrunc16 (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_op2 (C, SLJIT_AND, REG_TMP1, 0, REG_TMP1, 0, SLJIT_IMM, 0xFFFF);
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  return S_OK;
}

/* TRUNC32: Truncate 64-bit to 32-bit */
static
HRESULT
JitGenTrunc32 (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, SrcOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Just copy low 32 bits */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SrcOffset);
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  return S_OK;
}

/* MULU: Unsigned multiply */
static
HRESULT
JitGenMulu (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), Src1Offset);
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP2, 0, SLJIT_MEM1(REG_STATE), Src2Offset);
  sljit_emit_op0 (C, SLJIT_LMUL_UW);  /* REG_TMP1 * REG_TMP2 -> result in REG_TMP1 */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  return S_OK;
}

/* DIVU: Unsigned divide */
static
HRESULT
JitGenDivu (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), Src1Offset);
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP2, 0, SLJIT_MEM1(REG_STATE), Src2Offset);
  sljit_emit_op0 (C, SLJIT_DIVMOD_UW);  /* REG_TMP1 / REG_TMP2 -> quotient in REG_TMP1 */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  return S_OK;
}

/* MODU: Unsigned modulo */
static
HRESULT
JitGenModu (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, Src1Offset, Src2Offset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &Src1Offset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &Src2Offset);
  if (FAILED (Result)) return Result;

  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), Src1Offset);
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP2, 0, SLJIT_MEM1(REG_STATE), Src2Offset);
  sljit_emit_op0 (C, SLJIT_DIVMOD_UW);  /* REG_TMP1 / REG_TMP2 -> remainder in REG_TMP2 */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP2, 0);

  return S_OK;
}

/* LOAD_INDEXED: Load from memory with base + index */
static
HRESULT
JitGenLoadIndexed (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, BaseOffset, IndexOffset;
  HRESULT Result;
  sljit_sw SharedMemOffset = offsetof(VINIL_EXECUTION_STATE, SharedMemory);

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &BaseOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &IndexOffset);
  if (FAILED (Result)) return Result;

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SharedMemOffset);

  /* Load base + index */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP2, 0, SLJIT_MEM1(REG_STATE), BaseOffset);
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP3, 0, SLJIT_MEM1(REG_STATE), IndexOffset);
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP2, 0, REG_TMP2, 0, REG_TMP3, 0);

  /* REG_TMP1 = SharedMemory + offset */
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP1, 0, REG_TMP1, 0, REG_TMP2, 0);

  /* Load value from memory */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_TMP1), 0);

  /* Store to destination */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  return S_OK;
}

/* STORE_INDEXED: Store to memory with base + index */
static
HRESULT
JitGenStoreIndexed (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw BaseOffset, IndexOffset, SrcOffset;
  HRESULT Result;
  sljit_sw SharedMemOffset = offsetof(VINIL_EXECUTION_STATE, SharedMemory);

  Result = GetVariableOffset (Context, Inst->Src[0], &BaseOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &IndexOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[2], &SrcOffset);
  if (FAILED (Result)) return Result;

  /* Load SharedMemory pointer */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), SharedMemOffset);

  /* Load base + index */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP2, 0, SLJIT_MEM1(REG_STATE), BaseOffset);
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP3, 0, SLJIT_MEM1(REG_STATE), IndexOffset);
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP2, 0, REG_TMP2, 0, REG_TMP3, 0);

  /* REG_TMP1 = SharedMemory + offset */
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP1, 0, REG_TMP1, 0, REG_TMP2, 0);

  /* Load value to store */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP2, 0, SLJIT_MEM1(REG_STATE), SrcOffset);

  /* Store to memory */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_TMP1), 0, REG_TMP2, 0);

  return S_OK;
}

/* LEA: Load effective address (base + offset) */
static
HRESULT
JitGenLea (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;
  sljit_sw DstOffset, BaseOffset, OffsetOffset;
  HRESULT Result;

  Result = GetVariableOffset (Context, Inst->Dst, &DstOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[0], &BaseOffset);
  if (FAILED (Result)) return Result;
  Result = GetVariableOffset (Context, Inst->Src[1], &OffsetOffset);
  if (FAILED (Result)) return Result;

  /* Load base and offset */
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP1, 0, SLJIT_MEM1(REG_STATE), BaseOffset);
  sljit_emit_op1 (C, SLJIT_MOV, REG_TMP2, 0, SLJIT_MEM1(REG_STATE), OffsetOffset);

  /* Add them */
  sljit_emit_op2 (C, SLJIT_ADD, REG_TMP1, 0, REG_TMP1, 0, REG_TMP2, 0);

  /* Store result */
  sljit_emit_op1 (C, SLJIT_MOV, SLJIT_MEM1(REG_STATE), DstOffset, REG_TMP1, 0);

  return S_OK;
}

/* TRAP: Software trap/breakpoint */
static
HRESULT
JitGenTrap (
  VINIL_JIT_CONTEXT       *Context,
  VINIL_INSTRUCTION_NODE  *Inst
  )
{
  struct sljit_compiler *C = Context->Compiler;

  (void)Inst;

  /* Emit a breakpoint instruction */
  sljit_emit_op0 (C, SLJIT_BREAKPOINT);

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

    case VINIL_OP_MOVA:
      return JitGenMova (Context, Inst);

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

    case VINIL_OP_ROUND:
      return JitGenRound (Context, Inst);

    case VINIL_OP_EXP2:
      return JitGenExp2 (Context, Inst);

    case VINIL_OP_LOG2:
      return JitGenLog2 (Context, Inst);

    case VINIL_OP_AND:
      return JitGenAnd (Context, Inst);

    case VINIL_OP_OR:
      return JitGenOr (Context, Inst);

    case VINIL_OP_XOR:
      return JitGenXor (Context, Inst);

    case VINIL_OP_NOT:
      return JitGenNot (Context, Inst);

    case VINIL_OP_SHL:
      return JitGenShl (Context, Inst);

    case VINIL_OP_SHR:
      return JitGenShr (Context, Inst);

    case VINIL_OP_SAR:
      return JitGenSar (Context, Inst);

    case VINIL_OP_SIN:
      return JitGenSin (Context, Inst);

    case VINIL_OP_COS:
      return JitGenCos (Context, Inst);

    case VINIL_OP_POW:
      return JitGenPow (Context, Inst);

    case VINIL_OP_TAN:
      return JitGenTan (Context, Inst);

    case VINIL_OP_ASIN:
      return JitGenAsin (Context, Inst);

    case VINIL_OP_ACOS:
      return JitGenAcos (Context, Inst);

    case VINIL_OP_ATAN:
      return JitGenAtan (Context, Inst);

    case VINIL_OP_ATAN2:
      return JitGenAtan2 (Context, Inst);

    case VINIL_OP_EXP:
      return JitGenExp (Context, Inst);

    case VINIL_OP_LOG:
      return JitGenLog (Context, Inst);

    case VINIL_OP_DP2:
      return JitGenDp2 (Context, Inst);

    case VINIL_OP_DP3:
      return JitGenDp3 (Context, Inst);

    case VINIL_OP_DP4:
      return JitGenDp4 (Context, Inst);

    case VINIL_OP_CRS:
      return JitGenCrs (Context, Inst);

    case VINIL_OP_LEN:
      return JitGenLen (Context, Inst);

    case VINIL_OP_NRM:
      return JitGenNrm (Context, Inst);

    case VINIL_OP_SELECT:
      return JitGenSelect (Context, Inst);

    case VINIL_OP_NOP:
      return JitGenNop (Context, Inst);

    case VINIL_OP_SHUFFLE:
      return JitGenShuffle (Context, Inst);

    case VINIL_OP_DISCARD:
      return JitGenDiscard (Context, Inst);

    case VINIL_OP_GET_GLOBAL_ID:
      return JitGenGetGlobalId (Context, Inst);

    case VINIL_OP_GET_LOCAL_ID:
      return JitGenGetLocalId (Context, Inst);

    case VINIL_OP_GET_GROUP_ID:
      return JitGenGetGroupId (Context, Inst);

    case VINIL_OP_GET_GLOBAL_SIZE:
      return JitGenGetGlobalSize (Context, Inst);

    case VINIL_OP_GET_LOCAL_SIZE:
      return JitGenGetLocalSize (Context, Inst);

    case VINIL_OP_GET_NUM_GROUPS:
      return JitGenGetNumGroups (Context, Inst);

    case VINIL_OP_LOAD:
      return JitGenLoad (Context, Inst);

    case VINIL_OP_STORE:
      return JitGenStore (Context, Inst);

    case VINIL_OP_LOAD_VEC:
      return JitGenLoadVec (Context, Inst);

    case VINIL_OP_STORE_VEC:
      return JitGenStoreVec (Context, Inst);

    case VINIL_OP_ATOMIC_ADD:
      return JitGenAtomicAdd (Context, Inst);

    case VINIL_OP_ATOMIC_SUB:
      return JitGenAtomicSub (Context, Inst);

    case VINIL_OP_ATOMIC_MIN:
      return JitGenAtomicMin (Context, Inst);

    case VINIL_OP_ATOMIC_MAX:
      return JitGenAtomicMax (Context, Inst);

    case VINIL_OP_ATOMIC_AND:
      return JitGenAtomicAnd (Context, Inst);

    case VINIL_OP_ATOMIC_OR:
      return JitGenAtomicOr (Context, Inst);

    case VINIL_OP_ATOMIC_XOR:
      return JitGenAtomicXor (Context, Inst);

    case VINIL_OP_ATOMIC_XCHG:
      return JitGenAtomicXchg (Context, Inst);

    case VINIL_OP_ATOMIC_CAS:
      return JitGenAtomicCas (Context, Inst);

    case VINIL_OP_BARRIER:
      return JitGenBarrier (Context, Inst);

    case VINIL_OP_FENCE:
      return JitGenFence (Context, Inst);

    case VINIL_OP_MEM_FENCE:
      return JitGenMemFence (Context, Inst);

    case VINIL_OP_READ_FENCE:
      return JitGenReadFence (Context, Inst);

    case VINIL_OP_WRITE_FENCE:
      return JitGenWriteFence (Context, Inst);

    case VINIL_OP_TEX:
      return JitGenTex (Context, Inst);

    case VINIL_OP_TXL:
      return JitGenTxl (Context, Inst);

    case VINIL_OP_TXB:
      return JitGenTxb (Context, Inst);

    case VINIL_OP_TXP:
      return JitGenTxp (Context, Inst);

    case VINIL_OP_TXD:
      return JitGenTxd (Context, Inst);

    case VINIL_OP_TXF:
      return JitGenTxf (Context, Inst);

    case VINIL_OP_IF:
      return JitGenIf (Context, Inst);

    case VINIL_OP_ELSE:
      return JitGenElse (Context, Inst);

    case VINIL_OP_ENDIF:
      return JitGenEndif (Context, Inst);

    case VINIL_OP_LOOP:
      return JitGenLoop (Context, Inst);

    case VINIL_OP_ENDLOOP:
      return JitGenEndloop (Context, Inst);

    case VINIL_OP_BREAK:
      return JitGenBreak (Context, Inst);

    case VINIL_OP_CONTINUE:
      return JitGenContinue (Context, Inst);

    case VINIL_OP_RET:
      return JitGenRet (Context, Inst);

    /* Bytecode Extension Opcodes */
    case VINIL_OP_PUSH:
      return JitGenPush (Context, Inst);

    case VINIL_OP_POP:
      return JitGenPop (Context, Inst);

    case VINIL_OP_DUP:
      return JitGenDup (Context, Inst);

    case VINIL_OP_SWAP:
      return JitGenSwap (Context, Inst);

    case VINIL_OP_ZEXT8:
      return JitGenZext8 (Context, Inst);

    case VINIL_OP_ZEXT16:
      return JitGenZext16 (Context, Inst);

    case VINIL_OP_ZEXT32:
      return JitGenZext32 (Context, Inst);

    case VINIL_OP_SEXT8:
      return JitGenSext8 (Context, Inst);

    case VINIL_OP_SEXT16:
      return JitGenSext16 (Context, Inst);

    case VINIL_OP_SEXT32:
      return JitGenSext32 (Context, Inst);

    case VINIL_OP_TRUNC8:
      return JitGenTrunc8 (Context, Inst);

    case VINIL_OP_TRUNC16:
      return JitGenTrunc16 (Context, Inst);

    case VINIL_OP_TRUNC32:
      return JitGenTrunc32 (Context, Inst);

    case VINIL_OP_MULU:
      return JitGenMulu (Context, Inst);

    case VINIL_OP_DIVU:
      return JitGenDivu (Context, Inst);

    case VINIL_OP_MODU:
      return JitGenModu (Context, Inst);

    case VINIL_OP_LOAD_INDEXED:
      return JitGenLoadIndexed (Context, Inst);

    case VINIL_OP_STORE_INDEXED:
      return JitGenStoreIndexed (Context, Inst);

    case VINIL_OP_LEA:
      return JitGenLea (Context, Inst);

    case VINIL_OP_TRAP:
      return JitGenTrap (Context, Inst);

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
