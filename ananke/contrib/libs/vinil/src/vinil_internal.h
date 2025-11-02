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
