/*++
    Module Name:

        test_disasm.c

    Abstract:

        Test program for VINIL disassembler, binary format, and assembler.

    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#include <vinil/disasm.h>
#include <vinil/binary.h>
#include <vinil/asm.h>
#include <stdio.h>
#include <string.h>

static VOID
TestOpcodeInfo (
    VOID
    )
{
    CONST VINIL_OPCODE_INFO *Info;
    UINT32                  i;

    printf("=== Testing Opcode Information ===\n");

    for (i = 0; i < 10; i++) {
        Info = VinilGetOpcodeInfo(i);
        if (Info != NULL) {
            printf("Opcode %2u: %-12s - %s\n",
                Info->Opcode,
                Info->Name,
                Info->Description);
            printf("           Category: %u, Sources: %u, HasDest: %s, Graphics: %s, Compute: %s\n",
                Info->Category,
                Info->NumSources,
                Info->HasDestination ? "Yes" : "No",
                Info->IsGraphics ? "Yes" : "No",
                Info->IsCompute ? "Yes" : "No");
        }
    }

    printf("\n");
}

static VOID
TestOpcodeName (
    VOID
    )
{
    UINT32      i;
    CONST CHAR8 *Name;

    printf("=== Testing Opcode Names ===\n");

    for (i = 0; i < 20; i++) {
        Name = VinilGetOpcodeName(i);
        printf("Opcode %2u: %s\n", i, Name);
    }

    printf("\n");
}

static VOID
TestValidation (
    VOID
    )
{
    printf("=== Testing Opcode Validation ===\n");

    printf("Opcode 0 valid: %s\n", VinilIsValidOpcode(0) ? "Yes" : "No");
    printf("Opcode 50 valid: %s\n", VinilIsValidOpcode(50) ? "Yes" : "No");
    printf("Opcode 79 valid: %s\n", VinilIsValidOpcode(79) ? "Yes" : "No");
    printf("Opcode 80 valid: %s\n", VinilIsValidOpcode(80) ? "Yes" : "No");
    printf("Opcode 255 valid: %s\n", VinilIsValidOpcode(255) ? "Yes" : "No");

    printf("\n");
}

static VOID
TestDisassembly (
    VOID
    )
{
    UINT32  FakeInstruction[8];
    CHAR8   Buffer[256];
    HRESULT Hr;

    printf("=== Testing Instruction Disassembly ===\n");

    /* Create a fake ADD instruction (opcode 1) */
    memset(FakeInstruction, 0, sizeof(FakeInstruction));
    FakeInstruction[0] = 1;  /* ADD opcode */

    Hr = VinilDisasmInstruction(FakeInstruction, VinilDisasmNone, Buffer, sizeof(Buffer));
    if (SUCCEEDED(Hr)) {
        printf("Disassembly: %s\n", Buffer);
    } else {
        printf("Disassembly failed: 0x%08X\n", Hr);
    }

    /* Try with verbose flag */
    Hr = VinilDisasmInstruction(FakeInstruction, VinilDisasmVerbose, Buffer, sizeof(Buffer));
    if (SUCCEEDED(Hr)) {
        printf("Verbose:     %s\n", Buffer);
    }

    /* Try with addresses */
    Hr = VinilDisasmInstruction(FakeInstruction, VinilDisasmShowAddresses, Buffer, sizeof(Buffer));
    if (SUCCEEDED(Hr)) {
        printf("With Addr:   %s\n", Buffer);
    }

    printf("\n");
}

static VOID
TestBinaryFormat (
    VOID
    )
{
    UINT8   Buffer[1024];
    UINTN   BytesWritten;
    HRESULT Hr;
    VOID    *FakeProgram = (VOID *)0x12345678;  /* Dummy pointer */

    printf("=== Testing Binary Format ===\n");

    /* Test serialization */
    Hr = VinilSerializeProgram(FakeProgram, Buffer, sizeof(Buffer), &BytesWritten);
    if (SUCCEEDED(Hr)) {
        printf("Serialization successful: %zu bytes written\n", BytesWritten);
    } else {
        printf("Serialization failed: 0x%08X\n", Hr);
    }

    /* Test validation */
    Hr = VinilValidateBinary(Buffer, BytesWritten);
    if (SUCCEEDED(Hr)) {
        printf("Binary validation: PASSED\n");

        /* Print header info */
        VINIL_BINARY_HEADER *Hdr = (VINIL_BINARY_HEADER *)Buffer;
        printf("  Magic: 0x%08X\n", Hdr->Magic);
        printf("  Version: 0x%08X\n", Hdr->Version);
        printf("  Mode: %u\n", Hdr->Mode);
        printf("  NumSections: %u\n", Hdr->NumSections);
    } else {
        printf("Binary validation: FAILED (0x%08X)\n", Hr);
    }

    printf("\n");
}

static VOID
TestAssembler (
    VOID
    )
{
    CONST CHAR8 Source[] =
        "; Test assembly\n"
        "ADD r0, r1, r2\n"
        "MUL r3, r4, r5\n";
    VOID            *Program;
    VINIL_ASM_ERROR Error;
    HRESULT         Hr;

    printf("=== Testing Assembler ===\n");

    Hr = VinilAssemble(Source, sizeof(Source) - 1, VinilAsmNone, &Program, &Error);
    if (SUCCEEDED(Hr)) {
        printf("Assembly successful!\n");
        if (Program != NULL) {
            printf("Program created at %p\n", Program);
        }
    } else if (Hr == E_NOTIMPL) {
        printf("Assembly parser implemented but program construction not yet complete\n");
    } else {
        printf("Assembly failed: 0x%08X\n", Hr);
        if (Error.Message != NULL) {
            printf("  Error at line %u, column %u: %s\n",
                Error.Line, Error.Column, Error.Message);
        }
    }

    printf("\n");
}

INT32
main (
    VOID
    )
{
    printf("VINIL Disassembler, Binary Format, and Assembler Test\n");
    printf("======================================================\n\n");

    TestOpcodeInfo();
    TestOpcodeName();
    TestValidation();
    TestDisassembly();
    TestBinaryFormat();
    TestAssembler();

    printf("All tests completed!\n");
    return 0;
}
