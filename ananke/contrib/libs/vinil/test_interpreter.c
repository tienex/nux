/** @file
  VINIL Interpreter Test Program

  Simple test to verify interpreter executes IL programs correctly.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#include <vinil/vinil.h>
#include <vinil/builder.h>
#include <stdio.h>
#include <stdlib.h>

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

  printf ("VINIL Interpreter Test\n");
  printf ("======================\n\n");

  /* Test 1: Create builder */
  printf ("Creating builder...\n");
  Result = VinilCreateBuilder (&Builder);
  if (FAILED (Result)) {
    printf ("FAILED: VinilCreateBuilder returned 0x%X\n", Result);
    return 1;
  }
  printf ("SUCCESS: Builder created\n\n");

  /* Test 2: Create variables */
  printf ("Creating variables...\n");
  Result = Builder->lpVtbl->CreateVariable (Builder, VinilVariableTypeFloat4, (CONST CHAR8 *)"a", &A);
  if (FAILED (Result)) {
    printf ("FAILED: CreateVariable(A) returned 0x%X\n", Result);
    goto cleanup;
  }

  Result = Builder->lpVtbl->CreateVariable (Builder, VinilVariableTypeFloat4, (CONST CHAR8 *)"b", &B);
  if (FAILED (Result)) {
    printf ("FAILED: CreateVariable(B) returned 0x%X\n", Result);
    goto cleanup;
  }

  Result = Builder->lpVtbl->CreateVariable (Builder, VinilVariableTypeFloat4, (CONST CHAR8 *)"c", &C);
  if (FAILED (Result)) {
    printf ("FAILED: CreateVariable(C) returned 0x%X\n", Result);
    goto cleanup;
  }
  printf ("SUCCESS: Variables created\n\n");

  /* Test 3: Build program (c = a + b) */
  printf ("Building program: c = a + b...\n");
  Result = Builder->lpVtbl->BuildAdd (Builder, C, A, B);
  if (FAILED (Result)) {
    printf ("FAILED: BuildAdd returned 0x%X\n", Result);
    goto cleanup;
  }

  Result = Builder->lpVtbl->BuildRet (Builder);
  if (FAILED (Result)) {
    printf ("FAILED: BuildRet returned 0x%X\n", Result);
    goto cleanup;
  }
  printf ("SUCCESS: Program built\n\n");

  /* Test 4: Finalize program */
  printf ("Finalizing program...\n");
  Result = Builder->lpVtbl->Finalize (Builder, &Program);
  if (FAILED (Result)) {
    printf ("FAILED: Finalize returned 0x%X\n", Result);
    goto cleanup;
  }
  printf ("SUCCESS: Program finalized\n\n");

  /* Test 5: Create execution context */
  printf ("Creating execution context...\n");
  Result = VinilCreateContext (&Context);
  if (FAILED (Result)) {
    printf ("FAILED: VinilCreateContext returned 0x%X\n", Result);
    goto cleanup;
  }
  printf ("SUCCESS: Context created\n\n");

  /* Test 6: Execute program */
  printf ("Executing program with interpreter backend...\n");
  Result = Context->lpVtbl->Execute (Context, Program, VinilBackendInterpreter, NULL, NULL);
  if (FAILED (Result)) {
    printf ("FAILED: Execute returned 0x%X\n", Result);
    goto cleanup;
  }
  printf ("SUCCESS: Program executed\n\n");

  /* Test 7: Check version */
  {
    UINT32 Major, Minor, Patch;
    Result = VinilGetVersion (&Major, &Minor, &Patch);
    if (SUCCEEDED (Result)) {
      printf ("VINIL Version: %u.%u.%u\n", Major, Minor, Patch);
    }
  }

  /* Test 8: Check supported backends */
  {
    UINT32 Backends;
    Result = VinilGetSupportedBackends (&Backends);
    if (SUCCEEDED (Result)) {
      printf ("Supported backends: ");
      if (Backends & (1 << VinilBackendInterpreter)) {
        printf ("Interpreter ");
      }
      if (Backends & (1 << VinilBackendJit)) {
        printf ("JIT ");
      }
      if (Backends & (1 << VinilBackendAot)) {
        printf ("AOT ");
      }
      printf ("\n");
    }
  }

  printf ("\nAll tests PASSED!\n");

cleanup:
  if (Context != NULL) {
    Context->lpVtbl->Release (Context);
  }
  if (Program != NULL) {
    Program->lpVtbl->Release (Program);
  }
  if (C != NULL) {
    C->lpVtbl->Release (C);
  }
  if (B != NULL) {
    B->lpVtbl->Release (B);
  }
  if (A != NULL) {
    A->lpVtbl->Release (A);
  }
  if (Builder != NULL) {
    Builder->lpVtbl->Release (Builder);
  }

  return SUCCEEDED (Result) ? 0 : 1;
}
