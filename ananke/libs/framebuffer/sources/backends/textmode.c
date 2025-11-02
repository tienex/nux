/*++
    Module Name:

        textmode.c

    Abstract:

        Text mode framebuffer backend (consolidated into ansi_terminal.c).

        This file now contains only a compatibility shim. The actual
        implementation uses the ANSI Terminal backend which provides
        comprehensive text-based rendering with support for:
        - ANSI escape sequences
        - 256-color palette
        - Unicode block characters for graphics
        - VT100/xterm compatibility

        Text mode (BIOS text mode, EFI console text protocol) can be
        handled by the ANSI Terminal backend configured for basic output.

--*/

#include <ananke/framebuffer/backends.h>

/* Forward declaration */
IFramebufferBackend *FbCreateAnsiTerminalBackend(VOID);

/*
 * Create a text mode framebuffer backend.
 *
 * This function is maintained for API compatibility but returns
 * an ANSI Terminal backend instance. The ANSI Terminal backend
 * handles all text-based rendering including BIOS/EFI text modes.
 */
IFramebufferBackend *
FbCreateTextmodeBackend(
    VOID
    )
{
    return FbCreateAnsiTerminalBackend();
}
