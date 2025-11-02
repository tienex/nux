/*++
    Module Name:

        vesa_banked.c

    Abstract:

        VESA banked/segmented framebuffer backend (consolidated into pcga.c).

        This file now contains only a compatibility shim. The actual
        implementation has been merged into the unified PC Graphics backend
        (pcga.c) which handles all VESA modes including banked modes with
        64KB window switching.

        VESA banked modes (VESA 1.x/2.0) are supported by configuring
        the PC Graphics backend with:
        - FbMemoryBanked memory organization
        - Window base address (typically 0xA0000)
        - Bank switching function via FbPcGraphicsSetBankFunction()

--*/

#include <ananke/framebuffer/backends.h>

/* Forward declarations */
IFramebufferBackend *FbCreatePcGraphicsBackend(VOID);
VOID FbPcGraphicsSetBankFunction(IFramebufferBackend *Backend, VOID (*BankSwitchFunc)(UINT32));

/*
 * Create a VESA banked/segmented framebuffer backend.
 *
 * This function is maintained for API compatibility but returns
 * a PC Graphics backend instance. The PC Graphics backend (pcga.c)
 * handles all VESA banked modes through the FRAMEBUFFER_DESC descriptor.
 */
IFramebufferBackend *
FbCreateVesaBankedBackend(
    VOID
    )
{
    return FbCreatePcGraphicsBackend();
}

/*
 * Set the bank switching function for VESA banked modes.
 *
 * This delegates to the PC Graphics backend's bank switching function.
 * The function should perform VESA INT 10h AX=4F05h bank switching.
 */
VOID
FbVesaBankedSetBankFunction(
    IFramebufferBackend *Backend,
    VOID (*SetBankFunc)(UINT32)
    )
{
    FbPcGraphicsSetBankFunction(Backend, SetBankFunc);
}
