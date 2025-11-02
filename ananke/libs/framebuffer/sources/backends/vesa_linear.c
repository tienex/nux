/*++
    Module Name:

        vesa_linear.c

    Abstract:

        VESA linear framebuffer backend (consolidated into pcga.c).

        This file now contains only a compatibility shim. The actual
        implementation has been merged into the unified PC Graphics backend
        (pcga.c) which handles all VESA modes (linear and banked) through
        the FRAMEBUFFER_DESC structure.

        VESA linear framebuffer modes (VESA 2.0+) are supported by configuring
        the PC Graphics backend with:
        - FbMemoryLinear memory organization
        - Linear framebuffer base address from VBE mode info
        - Appropriate pixel format (RGB555, RGB565, RGB888, etc.)

--*/

#include <ananke/framebuffer/backends.h>

/* Forward declaration */
IFramebufferBackend *FbCreatePcGraphicsBackend(VOID);

/*
 * Create a VESA linear framebuffer backend.
 *
 * This function is maintained for API compatibility but returns
 * a PC Graphics backend instance. The PC Graphics backend (pcga.c)
 * handles all VESA modes through the FRAMEBUFFER_DESC descriptor.
 */
IFramebufferBackend *
FbCreateVesaLinearBackend(
    VOID
    )
{
    return FbCreatePcGraphicsBackend();
}
