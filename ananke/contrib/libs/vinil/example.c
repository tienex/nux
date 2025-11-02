/*++
    Module Name:

        example.c

    Abstract:

        Example program demonstrating VINIL COM-based API usage.

    Copyright (C) 2025 NUX Project

    SPDX-License-Identifier:    CDDL-1.0
--*/

#include <vinil/vinil.h>
#include <stdio.h>

INT32
main (
    VOID
    )
{
    HRESULT          Hr;
    IVinilContext    *Context = NULL;
    IVinilProgram    *Program = NULL;
    IVinilExecutable *Executable = NULL;
    UINT32           Major, Minor, Patch;
    CONST CHAR8      *ErrorMsg;
    UINT64           Cycles;

    printf("VINIL Library COM Example\n");
    printf("=========================\n\n");

    //
    // Create context
    //
    Hr = VinilCreateContext(&Context);
    if (FAILED(Hr)) {
        printf("✗ Failed to create context: 0x%08X\n", Hr);
        return 1;
    }
    printf("✓ Created execution context\n");

    //
    // Get version
    //
    Hr = Context->lpVtbl->GetVersion(Context, &Major, &Minor, &Patch);
    if (SUCCEEDED(Hr)) {
        printf("  Version: %u.%u.%u\n\n", Major, Minor, Patch);
    }

    //
    // Create program
    //
    Hr = Context->lpVtbl->CreateProgram(Context, &Program);
    if (FAILED(Hr)) {
        printf("✗ Failed to create program: 0x%08X\n", Hr);
        goto Cleanup;
    }
    printf("✓ Created IL program\n");
    printf("  (IL construction not yet implemented)\n\n");

    //
    // Compile program
    //
    Hr = Program->lpVtbl->Compile(
        Program,
        VinilCompileFlagUseJit | VinilCompileFlagOptimize,
        &Executable
        );
    if (FAILED(Hr)) {
        printf("✗ Compilation failed: 0x%08X\n", Hr);
        goto Cleanup;
    }
    printf("✓ Compiled program (JIT mode)\n");

    //
    // Execute
    //
    Hr = Executable->lpVtbl->Execute(Executable, VinilExecModeGraphics, NULL);
    if (Hr == E_NOTIMPL) {
        printf("⚠ Execution returned: Not implemented (expected)\n\n");
    } else if (FAILED(Hr)) {
        printf("✗ Execution failed: 0x%08X\n", Hr);
    } else {
        Executable->lpVtbl->GetStats(Executable, &Cycles);
        printf("✓ Execution succeeded (%llu cycles)\n\n", 
               (unsigned long long)Cycles);
    }

    //
    // Get error message (should be empty)
    //
    Hr = Context->lpVtbl->GetLastError(Context, &ErrorMsg);
    if (SUCCEEDED(Hr) && ErrorMsg[0] != '\0') {
        printf("Last error: %s\n", ErrorMsg);
    }

    printf("✓ Cleaned up resources\n\n");
    printf("VINIL COM library is functional!\n");
    printf("Next steps: Implement IL construction, compilation, and execution.\n");

Cleanup:
    if (Executable != NULL) {
        Executable->lpVtbl->Release(Executable);
    }
    if (Program != NULL) {
        Program->lpVtbl->Release(Program);
    }
    if (Context != NULL) {
        Context->lpVtbl->Release(Context);
    }

    return SUCCEEDED(Hr) ? 0 : 1;
}
