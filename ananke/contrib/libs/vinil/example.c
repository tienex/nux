/*
** ==========================================================================
**
** VINIL Usage Example
**
** Simple example demonstrating the VINIL API
**
** ==========================================================================
*/

#include <vinil/vinil.h>
#include <stdio.h>

int main(void) {
    printf("VINIL Library Example\n");
    printf("====================\n");
    printf("Version: %s\n\n", vinil_version_string());

    /* Create execution context */
    vinil_context* ctx = vinil_context_create();
    if (!ctx) {
        fprintf(stderr, "Failed to create VINIL context\n");
        return 1;
    }
    printf("✓ Created execution context\n");

    /* Create a program */
    vinil_program* prog = vinil_program_create(ctx);
    if (!prog) {
        fprintf(stderr, "Failed to create program\n");
        vinil_context_destroy(ctx);
        return 1;
    }
    printf("✓ Created program\n");

    /* TODO: Build IL program here */
    printf("  (IL program construction not yet implemented)\n");

    /* Compile program (using JIT) */
    vinil_executable* exe = vinil_program_compile(ctx, prog, VINIL_TRUE);
    if (!exe) {
        fprintf(stderr, "Failed to compile program\n");
        vinil_program_destroy(prog);
        vinil_context_destroy(ctx);
        return 1;
    }
    printf("✓ Compiled program (JIT mode)\n");

    /* Execute */
    vinil_error err = vinil_execute(ctx, exe, NULL);
    if (err != VINIL_SUCCESS) {
        printf("⚠ Execution returned: %s (expected - not yet implemented)\n",
               vinil_error_string(err));
    }

    /* Cleanup */
    vinil_executable_destroy(exe);
    vinil_program_destroy(prog);
    vinil_context_destroy(ctx);
    printf("✓ Cleaned up resources\n");

    printf("\nVINIL library is functional!\n");
    printf("Next steps: Implement IL construction, compilation, and execution.\n");

    return 0;
}
