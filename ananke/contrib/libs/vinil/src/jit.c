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
    SLJIT_ENTER_FLOAT(2),  /* 0 int scratch + 2 float scratch (FR0, FR1) */
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
