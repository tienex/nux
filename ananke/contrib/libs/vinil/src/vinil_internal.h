/** @file
  VINIL Internal Function Declarations

  Internal functions shared between VINIL implementation files.
  NOT part of public API.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#pragma once
#include <vinil/vinil.h>
#include <vinil/il.h>
#include <vinil/memory.h>
#include <vinil/types.h>

//
// Instruction Storage (exposed for interpreter)
//

typedef struct _VINIL_INSTRUCTION_NODE {
  struct _VINIL_INSTRUCTION_NODE  *Next;
  VINIL_OPCODE                    Opcode;
  IVinilVariable                  *Dst;
  IVinilVariable                  *Src[3];  /* Max 3 sources */
} VINIL_INSTRUCTION_NODE;

//
// Program Implementation (exposed for interpreter)
//

typedef struct _VINIL_PROGRAM_IMPL {
  IVinilProgramVtbl       *lpVtbl;
  UINT32                  RefCount;
  VINIL_EXECUTION_MODE    Mode;
  IVinilMemoryPool        *MemoryPool;
  VINIL_INSTRUCTION_NODE  *FirstInstruction;
  VINIL_INSTRUCTION_NODE  *LastInstruction;
  UINT32                  InstructionCount;
} VINIL_PROGRAM_IMPL;

//
// Program Internal Functions
//

/**
  Create a VINIL program (internal).

  @param[in]   Mode        Execution mode (graphics/compute).
  @param[in]   MemoryPool  Memory pool for allocations.
  @param[out]  Program     Created program interface.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
VinilProgramCreate (
  VINIL_EXECUTION_MODE  Mode,
  IVinilMemoryPool      *MemoryPool,
  IVinilProgram         **Program
  );

/**
  Add instruction to program (internal).

  @param[in]  Program  Program to add instruction to.
  @param[in]  Opcode   Instruction opcode.
  @param[in]  Dst      Destination variable (can be NULL).
  @param[in]  Src0     First source variable (can be NULL).
  @param[in]  Src1     Second source variable (can be NULL).
  @param[in]  Src2     Third source variable (can be NULL).

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
VinilProgramAddInstruction (
  IVinilProgram   *Program,
  VINIL_OPCODE    Opcode,
  IVinilVariable  *Dst,
  IVinilVariable  *Src0,
  IVinilVariable  *Src1,
  IVinilVariable  *Src2
  );

//
// Variable Internal Functions
//

/**
  Create a variable (internal).

  @param[in]   Type      Variable type.
  @param[in]   Name      Variable name (can be NULL).
  @param[in]   Id        Variable ID.
  @param[out]  Variable  Created variable interface.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
VinilVariableCreate (
  IVinilType      *Type,
  CONST CHAR8     *Name,
  UINT32          Id,
  IVinilVariable  **Variable
  );

/**
  Create an instruction (internal).

  @param[in]   Opcode       Instruction opcode.
  @param[in]   Precision    Precision hint.
  @param[in]   Dst          Destination variable (can be NULL).
  @param[in]   Src0         First source variable (can be NULL).
  @param[in]   Src1         Second source variable (can be NULL).
  @param[in]   Src2         Third source variable (can be NULL).
  @param[out]  Instruction  Created instruction interface.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
VinilInstructionCreate (
  VINIL_OPCODE       Opcode,
  VINIL_PRECISION    Precision,
  IVinilVariable     *Dst,
  IVinilVariable     *Src0,
  IVinilVariable     *Src1,
  IVinilVariable     *Src2,
  IVinilInstruction  **Instruction
  );

/**
  Get variable's internal type (internal - not exposed by COM interface).

  @param[in]  Variable  Variable interface.

  @return  IVinilType pointer, or NULL if invalid.
**/
IVinilType *
VinilVariableGetTypeInternal (
  IVinilVariable  *Variable
  );

//
// Block Internal Functions
//

/**
  Create a basic block (internal).

  @param[in]   Id     Block ID.
  @param[out]  Block  Created block interface.

  @retval  S_OK           Success.
  @retval  E_POINTER      Invalid pointer.
  @retval  E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
VinilBlockCreate (
  UINT32       Id,
  IVinilBlock  **Block
  );

//
// Execution State (shared between interpreter and JIT)
//

typedef struct {
  union {
    float    f[4];    /* Float vector (up to 4 components) */
    INT32    i[4];    /* Int vector */
    UINT32   u[4];    /* Uint vector */
    BOOLEAN  b[4];    /* Bool vector */
  };
} VINIL_REGISTER_VALUE;

#define MAX_REGISTERS     256
#define MAX_CONTROL_FLOW  32

/* Control flow stack entry types */
typedef enum {
  VINIL_CF_IF,
  VINIL_CF_ELSE,
  VINIL_CF_LOOP
} VINIL_CF_TYPE;

/* Control flow stack entry */
typedef struct {
  VINIL_CF_TYPE  Type;
  UINT32         TargetIP;      /* Jump target (ELSE/ENDIF/ENDLOOP instruction) */
  UINT32         LoopStartIP;   /* Loop start IP (for CONTINUE) */
  BOOLEAN        Executing;     /* Whether this branch is executing */
} VINIL_CF_ENTRY;

typedef struct _VINIL_EXECUTION_STATE {
  VINIL_REGISTER_VALUE  Registers[MAX_REGISTERS];
  VOID                  *Inputs;        /* Graphics mode inputs */
  VOID                  *Outputs;       /* Graphics mode outputs */
  UINT32                GlobalId[3];    /* Compute mode work-item ID */
  UINT32                LocalId[3];
  UINT32                GroupId[3];
  UINT32                GlobalSize[3];
  UINT32                LocalSize[3];
  UINT32                NumGroups[3];
  BOOLEAN               Discarded;      /* Fragment discard flag */
  BOOLEAN               Returned;       /* Return flag */

  /* Control flow tracking */
  VINIL_CF_ENTRY        ControlFlowStack[MAX_CONTROL_FLOW];
  UINT32                ControlFlowDepth;
  BOOLEAN               ConditionResult;  /* Last condition evaluation */

  /* Backend operation sink (optional, provided by graphics backend) */
  struct IVinilTextureSampler      *TextureSampler;

  /* Shared memory buffer for LOAD/STORE operations */
  VOID                             *SharedMemory;
  UINTN                            SharedMemorySize;

  /* Stack-based bytecode execution support */
  UINT8                            *Stack;           /* General-purpose stack (4KB) */
  UINTN                            StackSize;        /* Stack size in bytes */
  UINTN                            SP;               /* Stack pointer (grows down) */
} VINIL_EXECUTION_STATE;

//
// Backend Execution Functions
//

/**
  Execute program using interpreter backend (internal).

  @param[in]  Program  IL program to execute.
  @param[in]  Inputs   Input data array.
  @param[in]  Outputs  Output data array.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Execution failed.
**/
HRESULT
VinilInterpreterExecute (
  IVinilProgram  *Program,
  VOID           *Inputs,
  VOID           *Outputs
  );

/**
  Execute program using JIT backend (internal).

  @param[in]  Program  IL program to execute.
  @param[in]  Inputs   Input data array.
  @param[in]  Outputs  Output data array.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Execution failed.
**/
HRESULT
VinilJitExecute (
  IVinilProgram  *Program,
  VOID           *Inputs,
  VOID           *Outputs
  );

/**
  Compile program to native code using JIT (internal).

  @param[in]   Program   IL program to compile.
  @param[out]  CodePtr   Pointer to generated code.
  @param[out]  CodeSize  Size of generated code in bytes.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Compilation failed.
**/
HRESULT
VinilJitCompileProgram (
  IVinilProgram  *Program,
  VOID           **CodePtr,
  UINTN          *CodeSize
  );
