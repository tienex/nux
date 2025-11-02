/** @file
  VINIL IL Disassembler

  Disassembles VINIL intermediate language to human-readable format.
  Useful for debugging IL generation and understanding program flow.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#ifndef __vinil_disasm_h__
#define __vinil_disasm_h__ 1

#include <ananke/types.h>
#include <ananke/hresult.h>

//
// Disassembly Options
//

typedef enum _VINIL_DISASM_FLAGS {
    VinilDisasmNone           = 0,
    VinilDisasmShowAddresses  = (1 << 0),  /* Show instruction addresses */
    VinilDisasmShowTypes      = (1 << 1),  /* Show operand types */
    VinilDisasmShowPrecision  = (1 << 2),  /* Show precision hints */
    VinilDisasmVerbose        = (1 << 3),  /* Verbose output */
} VINIL_DISASM_FLAGS;

//
// Opcode Categories
//

typedef enum _VINIL_OPCODE_CATEGORY {
    VinilOpcatArithmetic = 0,
    VinilOpcatVector,
    VinilOpcatTranscendental,
    VinilOpcatComparison,
    VinilOpcatControlFlow,
    VinilOpcatTexture,
    VinilOpcatMemory,
    VinilOpcatWorkItem,
    VinilOpcatSync,
    VinilOpcatAtomic,
} VINIL_OPCODE_CATEGORY;

//
// Opcode Information
//

typedef struct _VINIL_OPCODE_INFO {
    UINT32                  Opcode;
    CONST CHAR8             *Name;
    CONST CHAR8             *Description;
    VINIL_OPCODE_CATEGORY   Category;
    UINT32                  NumSources;      /* Number of source operands */
    BOOLEAN                 HasDestination;  /* TRUE if writes destination */
    BOOLEAN                 IsGraphics;      /* Available in graphics mode */
    BOOLEAN                 IsCompute;       /* Available in compute mode */
} VINIL_OPCODE_INFO;

//
// Disassembler Functions
//

/**
  Get opcode information.

  @param[in]  Opcode  Opcode value.

  @return  Pointer to opcode info structure, or NULL if invalid.
**/
CONST VINIL_OPCODE_INFO *
VinilGetOpcodeInfo (
    UINT32  Opcode
    );

/**
  Get opcode name.

  @param[in]  Opcode  Opcode value.

  @return  Opcode name string, or "UNKNOWN" if invalid.
**/
CONST CHAR8 *
VinilGetOpcodeName (
    UINT32  Opcode
    );

/**
  Check if opcode is valid.

  @param[in]  Opcode  Opcode value.

  @return  TRUE if valid, FALSE otherwise.
**/
BOOLEAN
VinilIsValidOpcode (
    UINT32  Opcode
    );

/**
  Disassemble a single instruction to string.

  @param[in]   Instruction  Instruction to disassemble.
  @param[in]   Flags        Disassembly flags.
  @param[out]  Buffer       Output buffer.
  @param[in]   BufferSize   Size of output buffer.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Disassembly failed.
**/
HRESULT
VinilDisasmInstruction (
    CONST VOID          *Instruction,
    VINIL_DISASM_FLAGS  Flags,
    CHAR8               *Buffer,
    UINTN               BufferSize
    );

/**
  Disassemble an entire program to string.

  @param[in]   Program     Program to disassemble.
  @param[in]   Flags       Disassembly flags.
  @param[out]  Buffer      Output buffer.
  @param[in]   BufferSize  Size of output buffer.

  @retval  S_OK       Success.
  @retval  E_POINTER  Invalid pointer.
  @retval  E_FAIL     Disassembly failed.
**/
HRESULT
VinilDisasmProgram (
    CONST VOID          *Program,
    VINIL_DISASM_FLAGS  Flags,
    CHAR8               *Buffer,
    UINTN               BufferSize
    );

#endif // __vinil_disasm_h__
