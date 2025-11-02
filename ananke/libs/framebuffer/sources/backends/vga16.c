/*++
    Module Name:

        vga16.c

    Abstract:

        VGA 16-color planar mode backend (consolidated into pcga.c).

        This file now contains only a compatibility shim. The actual
        implementation has been merged into the unified PC Graphics backend
        (pcga.c) which handles all PC-compatible graphics modes including:
        - CGA (320x200x4, 640x200x2)
        - EGA (640x350x16 planar)
        - VGA (640x480x16 planar, 320x200x256 linear)
        - SVGA/VESA (all modes, linear and banked)

        The FRAMEBUFFER_DESC structure describes memory organization,
        allowing the PC Graphics backend to handle any PC mode.

--*/

#include <ananke/framebuffer/backends.h>

/* Forward declaration */
IFramebufferBackend *FbCreatePcGraphicsBackend(VOID);

/*
 * Create a VGA 16-color planar mode backend.
 *
 * This function is maintained for API compatibility but returns
 * a PC Graphics backend instance. The PC Graphics backend (pcga.c)
 * handles all VGA modes through the FRAMEBUFFER_DESC descriptor.
 */
IFramebufferBackend *
FbCreateVga16Backend(
    VOID
    )
{
    return FbCreatePcGraphicsBackend();
}
