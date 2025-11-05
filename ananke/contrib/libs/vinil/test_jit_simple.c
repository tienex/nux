/** @file
  Simple SLJIT Direct Test
**/

#define SLJIT_CONFIG_AUTO 1
#include "../sljit/sljit_src/sljitLir.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  float x[4];
  float y[4];
  float z[4];
} TestState;

int main(void) {
  struct sljit_compiler *C;
  void (*func)(TestState *state);
  TestState state = {{1.0f, 2.0f, 3.0f, 4.0f},
                     {5.0f, 6.0f, 7.0f, 8.0f},
                     {0, 0, 0, 0}};

  printf("Creating SLJIT compiler...\n");
  C = sljit_create_compiler(NULL);
  if (!C) {
    printf("Failed to create compiler\n");
    return 1;
  }

  printf("Emitting prologue...\n");
  /* SLJIT_ARGS1V(P): void func(void *ptr)
     0 scratch int regs + 2 float scratch regs, 1 saved int reg (S0),
     0 local stack */
  sljit_emit_enter(C, 0, SLJIT_ARGS1V(P), SLJIT_ENTER_FLOAT(2), 1, 0);

  /* z[0] = x[0] + y[0] */
  printf("Emitting float load x[0]...\n");
  sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16, SLJIT_FR0, SLJIT_MEM1(SLJIT_S0), 0);

  printf("Emitting float load y[0]...\n");
  sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_ALIGNED_16, SLJIT_FR1, SLJIT_MEM1(SLJIT_S0), 16);

  printf("Emitting float add...\n");
  sljit_emit_fop2(C, SLJIT_ADD_F32, SLJIT_FR0, 0, SLJIT_FR0, 0, SLJIT_FR1, 0);

  printf("Emitting float store z[0]...\n");
  sljit_emit_fmem(C, SLJIT_MOV_F32 | SLJIT_MEM_STORE | SLJIT_MEM_ALIGNED_16, SLJIT_FR0, SLJIT_MEM1(SLJIT_S0), 32);

  printf("Emitting return...\n");
  sljit_emit_return_void(C);

  printf("Generating code...\n");
  func = (void (*)(TestState*))sljit_generate_code(C, 0, NULL);
  if (!func) {
    printf("Failed to generate code\n");
    sljit_free_compiler(C);
    return 1;
  }

  printf("Code generated at %p\n", (void*)func);

  printf("Before: x[0]=%.1f, y[0]=%.1f, z[0]=%.1f\n", state.x[0], state.y[0], state.z[0]);

  printf("Executing JIT code...\n");
  func(&state);

  printf("After: z[0]=%.1f\n", state.z[0]);
  printf("Expected: z[0]=6.0\n");

  sljit_free_code((void*)func, NULL);
  sljit_free_compiler(C);

  if (state.z[0] >= 5.99f && state.z[0] <= 6.01f) {
    printf("\nSUCCESS!\n");
    return 0;
  } else {
    printf("\nFAILED!\n");
    return 1;
  }
}
