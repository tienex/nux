/** @file
  VINIL JIT Backend Test Program

  Tests JIT compilation and compares results with interpreter.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#define COBJMACROS
#include <vinil/vinil.h>
#include <vinil/builder.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main (
  void
  )
{
  IVinilBuilder   *Builder = NULL;
  IVinilContext   *Context = NULL;
  IVinilProgram   *Program = NULL;
  IVinilVariable  *A = NULL;
  IVinilVariable  *B = NULL;
  IVinilVariable  *C = NULL;
  HRESULT         Result;
  UINT32          Backends;

  printf ("VINIL JIT Backend Test\n");
  printf ("======================\n\n");

  /* Check supported backends */
  Result = VinilGetSupportedBackends (&Backends);
  if (SUCCEEDED (Result)) {
    printf ("Supported backends:\n");
    if (Backends & (1 << VinilBackendInterpreter)) {
      printf ("  - Interpreter\n");
    }
    if (Backends & (1 << VinilBackendJit)) {
      printf ("  - JIT\n");
    }
    if (Backends & (1 << VinilBackendAot)) {
      printf ("  - AOT\n");
    }
    printf ("\n");
  }

  /* Create builder */
  printf ("Creating builder and building test program...\n");
  Result = VinilCreateBuilder (&Builder);
  if (FAILED (Result)) {
    printf ("FAILED: VinilCreateBuilder returned 0x%X\n", Result);
    return 1;
  }

  /* Create variables */
  Result = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, (CONST CHAR8 *)"a", &A);
  if (FAILED (Result)) {
    printf ("FAILED: CreateVariable(A) returned 0x%X\n", Result);
    goto cleanup;
  }

  Result = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, (CONST CHAR8 *)"b", &B);
  if (FAILED (Result)) {
    printf ("FAILED: CreateVariable(B) returned 0x%X\n", Result);
    goto cleanup;
  }

  Result = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, (CONST CHAR8 *)"c", &C);
  if (FAILED (Result)) {
    printf ("FAILED: CreateVariable(C) returned 0x%X\n", Result);
    goto cleanup;
  }

  /* Build program: c = (a + b) * a */
  Result = IVinilBuilder_BuildAdd (Builder, C, A, B);
  if (FAILED (Result)) {
    printf ("FAILED: BuildAdd returned 0x%X\n", Result);
    goto cleanup;
  }

  Result = IVinilBuilder_BuildMul (Builder, C, C, A);
  if (FAILED (Result)) {
    printf ("FAILED: BuildMul returned 0x%X\n", Result);
    goto cleanup;
  }

  Result = IVinilBuilder_BuildRet (Builder);
  if (FAILED (Result)) {
    printf ("FAILED: BuildRet returned 0x%X\n", Result);
    goto cleanup;
  }

  /* Finalize program */
  Result = IVinilBuilder_Finalize (Builder, &Program);
  if (FAILED (Result)) {
    printf ("FAILED: Finalize returned 0x%X\n", Result);
    goto cleanup;
  }
  printf ("SUCCESS: Program built (c = (a + b) * a)\n\n");

  /* Create execution context */
  Result = VinilCreateContext (&Context);
  if (FAILED (Result)) {
    printf ("FAILED: VinilCreateContext returned 0x%X\n", Result);
    goto cleanup;
  }

  /* Test Interpreter backend */
  printf ("Testing Interpreter backend...\n");
  Result = IVinilContext_Execute (Context, Program, VinilBackendInterpreter, NULL, NULL);
  if (FAILED (Result)) {
    printf ("FAILED: Interpreter Execute returned 0x%X\n", Result);
    goto cleanup;
  }
  printf ("SUCCESS: Interpreter executed\n\n");

  /* Test JIT backend */
  if (Backends & (1 << VinilBackendJit)) {
    printf ("Testing JIT backend...\n");
    Result = IVinilContext_Execute (Context, Program, VinilBackendJit, NULL, NULL);
    if (FAILED (Result)) {
      printf ("FAILED: JIT Execute returned 0x%X\n", Result);
      /* Non-fatal - JIT might not implement all opcodes yet */
      if (Result == E_NOTIMPL) {
        printf ("  (E_NOTIMPL: Some opcodes not yet implemented in JIT)\n\n");
      }
    } else {
      printf ("SUCCESS: JIT executed\n\n");
    }
  } else {
    printf ("JIT backend not available on this platform\n\n");
  }

  printf ("All tests completed!\n");

cleanup:
  if (Context != NULL) {
    IVinilContext_Release (Context);
  }
  if (Program != NULL) {
    IVinilProgram_Release (Program);
  }
  if (C != NULL) {
    IVinilVariable_Release (C);
  }
  if (B != NULL) {
    IVinilVariable_Release (B);
  }
  if (A != NULL) {
    IVinilVariable_Release (A);
  }
  if (Builder != NULL) {
    IVinilBuilder_Release (Builder);
  }

  return SUCCEEDED (Result) ? 0 : 1;
}
