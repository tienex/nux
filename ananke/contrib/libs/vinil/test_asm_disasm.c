/** @file
  Test assembler and disassembler for bytecode extension opcodes.

  Verifies that all 20 new opcodes can be assembled and disassembled correctly.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#define COBJMACROS
#include <vinil/vinil.h>
#include <vinil/asm.h>
#include <vinil/disasm.h>
#include <vinil/il.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *TestProgram =
  "; Test all 20 bytecode extension opcodes\n"
  "\n"
  "; Variable declarations\n"
  "temp int r0, r1, r2;\n"
  "\n"
  "; Stack operations\n"
  "PUSH r0\n"
  "POP r1\n"
  "DUP\n"
  "SWAP\n"
  "\n"
  "; Zero extensions\n"
  "ZEXT8 r1, r0\n"
  "ZEXT16 r1, r0\n"
  "ZEXT32 r1, r0\n"
  "\n"
  "; Sign extensions\n"
  "SEXT8 r1, r0\n"
  "SEXT16 r1, r0\n"
  "SEXT32 r1, r0\n"
  "\n"
  "; Truncations\n"
  "TRUNC8 r1, r0\n"
  "TRUNC16 r1, r0\n"
  "TRUNC32 r1, r0\n"
  "\n"
  "; Unsigned arithmetic\n"
  "MULU r2, r0, r1\n"
  "DIVU r2, r0, r1\n"
  "MODU r2, r0, r1\n"
  "\n"
  "; Indexed memory\n"
  "LOAD.IDX r2, r0, r1\n"
  "STORE.IDX r0, r1, r2\n"
  "LEA r2, r0, r1\n"
  "\n"
  "; System\n"
  "TRAP\n"
  "RET\n";

int main(void) {
  IVinilContext *Context = NULL;
  IVinilProgram *Program = NULL;
  HRESULT Result;
  int ExitCode = 0;

  printf("\n=== VINIL Assembler/Disassembler Test ===\n\n");

  /* Create context */
  printf("Creating context...\n");
  Result = VinilCreateContext(&Context);
  if (FAILED(Result)) {
    printf("ERROR: Failed to create context (0x%08X)\n", Result);
    return 1;
  }
  printf("  Context created\n\n");

  /* Assemble test program */
  printf("Assembling test program with 20 bytecode opcodes...\n");
  VINIL_ASM_ERROR AsmError = {0};
  Result = VinilAssemble(
    (CONST CHAR8 *)TestProgram,
    strlen(TestProgram),
    VinilAsmNone,
    (VOID **)&Program,
    &AsmError
  );

  if (FAILED(Result)) {
    printf("ERROR: Assembly failed (0x%08X)\n", Result);
    if (AsmError.Message) {
      printf("  Error at line %u, column %u: %s\n",
        AsmError.Line, AsmError.Column, AsmError.Message);
    }
    ExitCode = 1;
    goto cleanup;
  }
  printf("  Assembly successful\n\n");

  /* Disassemble back */
  printf("Disassembling program...\n");
  CHAR8 DisasmBuffer[8192];
  Result = VinilDisasmProgram(Program, VinilDisasmNone, DisasmBuffer, sizeof(DisasmBuffer));

  if (FAILED(Result)) {
    printf("ERROR: Disassembly failed (0x%08X)\n", Result);
    ExitCode = 1;
    goto cleanup;
  }
  printf("  Disassembly successful\n\n");

  /* Print disassembled output */
  printf("=== Disassembled Output ===\n");
  printf("%s\n", DisasmBuffer);
  printf("=== End Disassembly ===\n\n");

  /* Verify key opcodes appear */
  const char *RequiredOpcodes[] = {
    "PUSH", "POP", "DUP", "SWAP",
    "ZEXT8", "ZEXT16", "ZEXT32",
    "SEXT8", "SEXT16", "SEXT32",
    "TRUNC8", "TRUNC16", "TRUNC32",
    "MULU", "DIVU", "MODU",
    "LOAD.IDX", "STORE.IDX", "LEA",
    "TRAP"
  };

  printf("Verifying all opcodes present in disassembly...\n");
  for (size_t i = 0; i < sizeof(RequiredOpcodes)/sizeof(RequiredOpcodes[0]); i++) {
    if (strstr(DisasmBuffer, RequiredOpcodes[i]) == NULL) {
      printf("  ERROR: Missing opcode: %s\n", RequiredOpcodes[i]);
      ExitCode = 1;
    } else {
      printf("  ✓ %s\n", RequiredOpcodes[i]);
    }
  }

  if (ExitCode == 0) {
    printf("\n=== All Tests PASSED ===\n\n");
  } else {
    printf("\n=== Some Tests FAILED ===\n\n");
  }

cleanup:
  if (Program != NULL) {
    IVinilProgram_Release(Program);
  }
  if (Context != NULL) {
    IVinilContext_Release(Context);
  }

  return ExitCode;
}
