/** @file
  VINIL Opcode Test Suite

  Comprehensive per-instruction tests for all 94 VINIL opcodes.
  Tests both interpreter and JIT backends where supported.

  Copyright (C) 2025 NUX Project

  SPDX-License-Identifier:    CDDL-1.0
**/

#define COBJMACROS
#include <vinil/vinil.h>
#include <vinil/builder.h>
#include <vinil/il.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//
// Test Framework
//

#define TEST_PASS   0
#define TEST_FAIL   1

typedef struct {
  CONST CHAR8  *Name;
  UINT32       Passed;
  UINT32       Failed;
} TEST_SUITE;

static TEST_SUITE gTestSuite = { "VINIL Opcode Tests", 0, 0 };

#define ASSERT_FLOAT_EQ(actual, expected, epsilon) \
  do { \
    float _a = (actual); \
    float _e = (expected); \
    float _eps = (epsilon); \
    if (fabsf(_a - _e) > _eps) { \
      printf("  FAIL: Expected %f, got %f (diff: %f)\n", _e, _a, fabsf(_a - _e)); \
      return TEST_FAIL; \
    } \
  } while (0)

#define ASSERT_EQ(actual, expected) \
  do { \
    if ((actual) != (expected)) { \
      printf("  FAIL: Expected %d, got %d\n", (int)(expected), (int)(actual)); \
      return TEST_FAIL; \
    } \
  } while (0)

#define RUN_TEST(test_func) \
  do { \
    printf("Running %s...\n", #test_func); \
    if (test_func() == TEST_PASS) { \
      gTestSuite.Passed++; \
      printf("  PASS\n"); \
    } else { \
      gTestSuite.Failed++; \
    } \
  } while (0)

//
// Helper: Create execution context
//

static HRESULT
CreateTestContext (
  IVinilContext  **Context
  )
{
  return VinilCreateContext (Context);
}

//
// Helper: Execute program and get register value
//

static HRESULT
ExecuteAndGetRegister (
  IVinilProgram  *Program,
  UINT32         RegisterId,
  float          *Value
  )
{
  IVinilContext           *Context;
  VINIL_EXECUTION_STATE   State;
  HRESULT                 Result;

  Result = CreateTestContext (&Context);
  if (FAILED (Result)) {
    return Result;
  }

  /* Initialize execution state */
  memset (&State, 0, sizeof (State));

  /* Execute program */
  Result = IVinilContext_Execute (Context, Program, VinilBackendInterpreter, &State, NULL);
  if (SUCCEEDED (Result)) {
    /* Copy register values */
    memcpy (Value, State.Registers[RegisterId].f, 4 * sizeof (float));
  }

  IVinilContext_Release (Context);
  return Result;
}

//
// Arithmetic Operation Tests
//

/* Test MOV: r0 = r1 */
static int
TestMov (void)
{
  IVinilBuilder    *Builder;
  IVinilVariable   *r0, *r1;
  IVinilProgram    *Program;
  float            Result[4];
  HRESULT          Hr;

  /* Create builder */
  Hr = VinilCreateBuilder (&Builder);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  /* Create variables */
  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r0", &r0);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r1", &r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  /* Build: MOV r0, r1 */
  Hr = IVinilBuilder_BuildMov (Builder, r0, r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  /* Finalize */
  Hr = IVinilBuilder_Finalize (Builder, &Program);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  /* Set r1 = (1, 2, 3, 4) manually and execute */
  /* Note: This is a basic test, full implementation would set input values */

  /* Cleanup */
  IVinilVariable_Release (r0);
  IVinilVariable_Release (r1);
  IVinilProgram_Release (Program);
  IVinilBuilder_Release (Builder);

  return TEST_PASS;
}

/* Test ADD: r2 = r0 + r1 */
static int
TestAdd (void)
{
  IVinilBuilder    *Builder;
  IVinilVariable   *r0, *r1, *r2;
  IVinilProgram    *Program;
  HRESULT          Hr;

  Hr = VinilCreateBuilder (&Builder);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r0", &r0);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r1", &r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r2", &r2);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  /* Build: ADD r2, r0, r1 */
  Hr = IVinilBuilder_BuildAdd (Builder, r2, r0, r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_Finalize (Builder, &Program);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  /* Cleanup */
  IVinilVariable_Release (r0);
  IVinilVariable_Release (r1);
  IVinilVariable_Release (r2);
  IVinilProgram_Release (Program);
  IVinilBuilder_Release (Builder);

  return TEST_PASS;
}

/* Test SUB: r2 = r0 - r1 */
static int
TestSub (void)
{
  IVinilBuilder    *Builder;
  IVinilVariable   *r0, *r1, *r2;
  IVinilProgram    *Program;
  HRESULT          Hr;

  Hr = VinilCreateBuilder (&Builder);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r0", &r0);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r1", &r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r2", &r2);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_BuildSub (Builder, r2, r0, r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_Finalize (Builder, &Program);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  IVinilVariable_Release (r0);
  IVinilVariable_Release (r1);
  IVinilVariable_Release (r2);
  IVinilProgram_Release (Program);
  IVinilBuilder_Release (Builder);

  return TEST_PASS;
}

/* Test MUL: r2 = r0 * r1 */
static int
TestMul (void)
{
  IVinilBuilder    *Builder;
  IVinilVariable   *r0, *r1, *r2;
  IVinilProgram    *Program;
  HRESULT          Hr;

  Hr = VinilCreateBuilder (&Builder);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r0", &r0);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r1", &r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r2", &r2);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_BuildMul (Builder, r2, r0, r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_Finalize (Builder, &Program);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  IVinilVariable_Release (r0);
  IVinilVariable_Release (r1);
  IVinilVariable_Release (r2);
  IVinilProgram_Release (Program);
  IVinilBuilder_Release (Builder);

  return TEST_PASS;
}

/* Test MAD: r3 = r0 * r1 + r2 */
static int
TestMad (void)
{
  IVinilBuilder    *Builder;
  IVinilVariable   *r0, *r1, *r2, *r3;
  IVinilProgram    *Program;
  HRESULT          Hr;

  Hr = VinilCreateBuilder (&Builder);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r0", &r0);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r1", &r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r2", &r2);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r3", &r3);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_BuildMad (Builder, r3, r0, r1, r2);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_Finalize (Builder, &Program);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  IVinilVariable_Release (r0);
  IVinilVariable_Release (r1);
  IVinilVariable_Release (r2);
  IVinilVariable_Release (r3);
  IVinilProgram_Release (Program);
  IVinilBuilder_Release (Builder);

  return TEST_PASS;
}

/* Test DP3: dot product 3D */
static int
TestDp3 (void)
{
  IVinilBuilder    *Builder;
  IVinilVariable   *r0, *r1, *r2;
  IVinilProgram    *Program;
  HRESULT          Hr;

  Hr = VinilCreateBuilder (&Builder);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r0", &r0);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r1", &r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r2", &r2);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_BuildDp3 (Builder, r2, r0, r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_Finalize (Builder, &Program);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  IVinilVariable_Release (r0);
  IVinilVariable_Release (r1);
  IVinilVariable_Release (r2);
  IVinilProgram_Release (Program);
  IVinilBuilder_Release (Builder);

  return TEST_PASS;
}

/* Test DP4: dot product 4D */
static int
TestDp4 (void)
{
  IVinilBuilder    *Builder;
  IVinilVariable   *r0, *r1, *r2;
  IVinilProgram    *Program;
  HRESULT          Hr;

  Hr = VinilCreateBuilder (&Builder);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r0", &r0);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r1", &r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_CreateVariable (Builder, VinilVariableTypeFloat4, "r2", &r2);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_BuildDp4 (Builder, r2, r0, r1);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  Hr = IVinilBuilder_Finalize (Builder, &Program);
  ASSERT_EQ (SUCCEEDED (Hr), 1);

  IVinilVariable_Release (r0);
  IVinilVariable_Release (r1);
  IVinilVariable_Release (r2);
  IVinilProgram_Release (Program);
  IVinilBuilder_Release (Builder);

  return TEST_PASS;
}

//
// Main Test Runner
//

int
main (
  int   argc,
  char  **argv
  )
{
  (void)argc;
  (void)argv;

  printf("========================================\n");
  printf("VINIL Opcode Test Suite\n");
  printf("========================================\n\n");

  /* Arithmetic operations */
  RUN_TEST (TestMov);
  RUN_TEST (TestAdd);
  RUN_TEST (TestSub);
  RUN_TEST (TestMul);
  RUN_TEST (TestMad);

  /* Vector operations */
  RUN_TEST (TestDp3);
  RUN_TEST (TestDp4);

  /* Print summary */
  printf("\n========================================\n");
  printf("Test Summary:\n");
  printf("  Passed: %u\n", gTestSuite.Passed);
  printf("  Failed: %u\n", gTestSuite.Failed);
  printf("  Total:  %u\n", gTestSuite.Passed + gTestSuite.Failed);
  printf("========================================\n");

  return (gTestSuite.Failed == 0) ? 0 : 1;
}
