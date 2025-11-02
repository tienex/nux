/*++
    Module Name:

        disasm.c

    Abstract:

        VINIL IL disassembler implementation using shared opcode table.

    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#include <vinil/disasm.h>
#include <vinil/il.h>
#include <stdio.h>
#include <string.h>

/* --------------------------------------------------------------- */
/*  Public API Implementation                                      */
/* --------------------------------------------------------------- */

CONST VINIL_OPCODE_INFO *
VinilGetOpcodeInfo (
    UINT32  Opcode
    )
{
    if (Opcode >= VINIL_OP_COUNT) {
        return NULL;
    }

    /* Check if opcode is defined in table */
    if (gVinilOpcodeTable[Opcode].Name == NULL) {
        return NULL;
    }

    return &gVinilOpcodeTable[Opcode];
}

CONST CHAR8 *
VinilGetOpcodeName (
    UINT32  Opcode
    )
{
    CONST VINIL_OPCODE_INFO *Info;

    Info = VinilGetOpcodeInfo (Opcode);
    if (Info == NULL) {
        return "UNKNOWN";
    }

    return Info->Name;
}

BOOLEAN
VinilIsValidOpcode (
    UINT32  Opcode
    )
{
    return (Opcode < VINIL_OP_COUNT && gVinilOpcodeTable[Opcode].Name != NULL);
}

HRESULT
VinilDisasmInstruction (
    CONST VOID          *Instruction,
    VINIL_DISASM_FLAGS  Flags,
    CHAR8               *Buffer,
    UINTN               BufferSize
    )
{
    /* TODO: Implement instruction disassembly */
    if (Buffer == NULL || Instruction == NULL) {
        return E_POINTER;
    }

    if (BufferSize == 0) {
        return E_INVALIDARG;
    }

    Buffer[0] = '\0';
    return E_NOTIMPL;
}

HRESULT
VinilDisasmProgram (
    CONST VOID          *Program,
    VINIL_DISASM_FLAGS  Flags,
    CHAR8               *Buffer,
    UINTN               BufferSize
    )
{
    /* TODO: Implement program disassembly */
    if (Buffer == NULL || Program == NULL) {
        return E_POINTER;
    }

    if (BufferSize == 0) {
        return E_INVALIDARG;
    }

    Buffer[0] = '\0';
    return E_NOTIMPL;
}
