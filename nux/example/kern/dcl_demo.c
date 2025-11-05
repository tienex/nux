/**
 * @file dcl_demo.c
 * @brief DCL Shell demonstration for NUX kernel
 *
 * This file demonstrates the integration of the DCL (Digital Command Language)
 * shell into the NUX kernel, showcasing syntax coloring and the client/server
 * architecture for extensible command parsing.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <nux/nux.h>
#include <hal/hal.h>
#include <dcl/dcl.h>

/**
 * DCL shell demonstration
 *
 * This function initializes and runs the DCL shell, demonstrating:
 * - ANSI escape code rendering for syntax coloring
 * - COM-based architecture with IDcl, IDclRenderer, IDclParser interfaces
 * - Message bus for client/server communication
 * - Extensible syntax provider system
 */
VOID
dcl_shell_demo(VOID)
{
    HRESULT hr;
    IDcl *pDcl = NULL;
    IDclRenderer *pRenderer = NULL;
    IDclSyntaxProvider *pSyntaxProvider = NULL;
    IDclParser *pParser = NULL;
    IDclMessageBus *pMessageBus = NULL;

    printf("=== NUX DCL Shell Demo ===\n");
    printf("Initializing DCL shell with syntax coloring...\n\n");

    /* Create the DCL shell instance */
    hr = DclCreateShell(&pDcl);
    if (FAILED(hr)) {
        printf("ERROR: Failed to create DCL shell (HRESULT: 0x%08lx)\n", (unsigned long)hr);
        return;
    }

    /* Initialize the shell */
    hr = pDcl->lpVtbl->Initialize(pDcl);
    if (FAILED(hr)) {
        printf("ERROR: Failed to initialize DCL shell (HRESULT: 0x%08lx)\n", (unsigned long)hr);
        pDcl->lpVtbl->Release((IUnknown *)pDcl);
        return;
    }

    /* Get the renderer interface */
    hr = pDcl->lpVtbl->GetRendererInterface(pDcl, &pRenderer);
    if (SUCCEEDED(hr)) {
        printf("Renderer interface: OK\n");
        pRenderer->lpVtbl->Release((IUnknown *)pRenderer);
    }

    /* Get the syntax provider interface */
    hr = pDcl->lpVtbl->GetSyntaxProviderInterface(pDcl, &pSyntaxProvider);
    if (SUCCEEDED(hr)) {
        printf("Syntax provider interface: OK\n");
        pSyntaxProvider->lpVtbl->Release((IUnknown *)pSyntaxProvider);
    }

    /* Get the parser interface */
    hr = pDcl->lpVtbl->GetParserInterface(pDcl, &pParser);
    if (SUCCEEDED(hr)) {
        printf("Parser interface: OK\n");
        pParser->lpVtbl->Release((IUnknown *)pParser);
    }

    /* Get the message bus interface */
    hr = pDcl->lpVtbl->GetMessageBusInterface(pDcl, &pMessageBus);
    if (SUCCEEDED(hr)) {
        printf("Message bus interface: OK\n");
        pMessageBus->lpVtbl->Release((IUnknown *)pMessageBus);
    }

    printf("\nAll COM interfaces initialized successfully!\n");
    printf("Starting DCL shell...\n\n");

    /* Set the global DCL instance */
    gpDcl = pDcl;

    /* Run the shell (this will display the demo) */
    hr = pDcl->lpVtbl->Run(pDcl);
    if (FAILED(hr)) {
        printf("\nERROR: Shell execution failed (HRESULT: 0x%08lx)\n", (unsigned long)hr);
    }

    /* Shutdown */
    printf("\n\nShutting down DCL shell...\n");
    pDcl->lpVtbl->Shutdown(pDcl);
    pDcl->lpVtbl->Release((IUnknown *)pDcl);

    printf("DCL shell demo completed.\n");
}

/**
 * Demonstrate syntax coloring with various DCL commands
 */
VOID
dcl_syntax_demo(VOID)
{
    HRESULT hr;
    const CHAR8 *testCommands[] = {
        "SHOW DIRECTORY test.txt",
        "SET DEFAULT sys$sysdevice:[dir]",
        "IF condition THEN command",
        "ASSIGN value $variable",
        "! This is a comment",
        "COPY file1.txt file2.txt",
        NULL
    };
    UINTN i;

    printf("\n=== DCL Syntax Coloring Demo ===\n\n");

    /* Initialize DCL shell using legacy API */
    hr = dcl_initialize();
    if (FAILED(hr)) {
        printf("ERROR: Failed to initialize DCL shell\n");
        return;
    }

    /* Execute test commands to demonstrate syntax coloring */
    for (i = 0; testCommands[i] != NULL; i++) {
        printf("Command %lu: %s\n", (unsigned long)i + 1, testCommands[i]);

        hr = dcl_execute_command(testCommands[i]);
        if (FAILED(hr)) {
            printf("  -> Execution failed\n");
        }
        printf("\n");
    }

    /* Cleanup */
    dcl_shutdown();
    printf("Syntax coloring demo completed.\n");
}
