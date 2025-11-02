/*++
    Module Name:

        disasm.c

    Abstract:

        VINIL IL disassembler implementation using shared opcode table.

    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#define COBJMACROS
#include <vinil/disasm.h>
#include <vinil/il.h>
#include "vinil_internal.h"
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
    CONST VINIL_INSTRUCTION_NODE  *Inst;
    CONST VINIL_OPCODE_INFO       *Info;
    CHAR8                         *Ptr;
    UINTN                         Remaining;
    CONST CHAR8                   *Name;
    UINTN                         NameLen;
    UINT32                        Id;
    UINT32                        i;

    if (Buffer == NULL || Instruction == NULL) {
        return E_POINTER;
    }

    if (BufferSize == 0) {
        return E_INVALIDARG;
    }

    /* Cast to internal instruction structure */
    Inst = (CONST VINIL_INSTRUCTION_NODE *)Instruction;

    /* Get opcode info */
    Info = VinilGetOpcodeInfo (Inst->Opcode);
    if (Info == NULL) {
        snprintf ((char *)Buffer, BufferSize, "INVALID_OPCODE(%u)", Inst->Opcode);
        return S_OK;
    }

    Ptr = Buffer;
    Remaining = BufferSize;

    /* Format: OPCODE dst, src0, src1, src2 */
    snprintf ((char *)Ptr, Remaining, "%-10s", Info->Name);
    Ptr += strlen ((const char *)Ptr);
    Remaining = BufferSize - (Ptr - Buffer);

    /* Destination operand */
    if (Info->HasDestination && Inst->Dst != NULL) {
        if (IVinilVariable_GetName (Inst->Dst, &Name, &NameLen) == S_OK && Name != NULL && NameLen > 0) {
            snprintf ((char *)Ptr, Remaining, " %.*s", (int)NameLen, Name);
        } else if (IVinilVariable_GetId (Inst->Dst, &Id) == S_OK) {
            snprintf ((char *)Ptr, Remaining, " r%u", Id);
        } else {
            snprintf ((char *)Ptr, Remaining, " <unknown>");
        }
        Ptr += strlen ((const char *)Ptr);
        Remaining = BufferSize - (Ptr - Buffer);
    }

    /* Source operands */
    for (i = 0; i < Info->NumSources && i < 3; i++) {
        if (Inst->Src[i] == NULL) {
            continue;
        }

        /* Add comma separator after destination or previous source */
        if (i > 0 || Info->HasDestination) {
            snprintf ((char *)Ptr, Remaining, ",");
            Ptr += strlen ((const char *)Ptr);
            Remaining = BufferSize - (Ptr - Buffer);
        }

        /* Format source operand */
        if (IVinilVariable_GetName (Inst->Src[i], &Name, &NameLen) == S_OK && Name != NULL && NameLen > 0) {
            snprintf ((char *)Ptr, Remaining, " %.*s", (int)NameLen, Name);
        } else if (IVinilVariable_GetId (Inst->Src[i], &Id) == S_OK) {
            snprintf ((char *)Ptr, Remaining, " r%u", Id);
        } else {
            snprintf ((char *)Ptr, Remaining, " <unknown>");
        }
        Ptr += strlen ((const char *)Ptr);
        Remaining = BufferSize - (Ptr - Buffer);
    }

    return S_OK;
}

HRESULT
VinilDisasmProgram (
    CONST VOID          *Program,
    VINIL_DISASM_FLAGS  Flags,
    CHAR8               *Buffer,
    UINTN               BufferSize
    )
{
    CONST VINIL_PROGRAM_IMPL      *Prog;
    CONST VINIL_INSTRUCTION_NODE  *Inst;
    CHAR8                         LineBuffer[256];
    CHAR8                         *Ptr;
    UINTN                         Remaining;
    UINT32                        InstructionIndex;
    HRESULT                       Hr;

    if (Buffer == NULL || Program == NULL) {
        return E_POINTER;
    }

    if (BufferSize == 0) {
        return E_INVALIDARG;
    }

    /* Cast to internal program structure */
    Prog = (CONST VINIL_PROGRAM_IMPL *)Program;

    Ptr = Buffer;
    Remaining = BufferSize;

    /* Write program header */
    if (Prog->Mode == VinilExecutionModeGraphics) {
        snprintf ((char *)Ptr, Remaining, "; VINIL IL Program - Graphics Mode\n");
    } else {
        snprintf ((char *)Ptr, Remaining, "; VINIL IL Program - Compute Mode\n");
    }
    Ptr += strlen ((const char *)Ptr);
    Remaining = BufferSize - (Ptr - Buffer);

    snprintf ((char *)Ptr, Remaining, "; Instructions: %u\n\n", Prog->InstructionCount);
    Ptr += strlen ((const char *)Ptr);
    Remaining = BufferSize - (Ptr - Buffer);

    /* Iterate through instruction list and disassemble each one */
    InstructionIndex = 0;
    for (Inst = Prog->FirstInstruction; Inst != NULL; Inst = Inst->Next) {
        /* Show instruction index if requested */
        if (Flags & VinilDisasmShowAddresses) {
            snprintf ((char *)Ptr, Remaining, "%4u: ", InstructionIndex);
            Ptr += strlen ((const char *)Ptr);
            Remaining = BufferSize - (Ptr - Buffer);
        }

        /* Disassemble instruction to line buffer */
        Hr = VinilDisasmInstruction (Inst, Flags, LineBuffer, sizeof (LineBuffer));
        if (FAILED (Hr)) {
            snprintf ((char *)Ptr, Remaining, "<disassembly failed>\n");
        } else {
            snprintf ((char *)Ptr, Remaining, "%s\n", LineBuffer);
        }

        Ptr += strlen ((const char *)Ptr);
        Remaining = BufferSize - (Ptr - Buffer);

        /* Check if buffer is getting full */
        if (Remaining < 100) {
            snprintf ((char *)Ptr, Remaining, "\n; ... output truncated ...\n");
            break;
        }

        InstructionIndex++;
    }

    return S_OK;
}
