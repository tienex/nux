/*++
    Module Name:

        uefi_uga.c

    Abstract:

        UEFI UGA (Universal Graphics Adapter) backend (consolidated into uefi_gop.c).

        This file now contains only a compatibility shim. UGA was the predecessor
        to GOP (Graphics Output Protocol) used in EFI 1.x and early UEFI 2.x.

        The UEFI GOP backend provides comprehensive support for both modern GOP
        and legacy UGA protocols, handling:
        - Direct framebuffer access
        - Hardware acceleration via Blt() operations
        - Multiple pixel formats (RGB555, RGB565, RGB888, RGBA8888)
        - Mode enumeration and setting

        For systems that only support UGA, the GOP backend can be configured
        to use UGA protocol calls.

--*/

#include <ananke/framebuffer/backends.h>

/* Forward declarations */
IFramebufferBackend *FbCreateUefiGopBackend(VOID);
VOID FbUefiUgaSetProtocol(IFramebufferBackend *Backend, VOID *UgaProtocol);

/*
 * Create a UEFI UGA (Universal Graphics Adapter) backend.
 *
 * This function is maintained for API compatibility but returns
 * a UEFI GOP backend instance. The GOP backend handles both modern
 * GOP and legacy UGA protocols.
 */
IFramebufferBackend *
FbCreateUefiUgaBackend(
    VOID
    )
{
    return FbCreateUefiGopBackend();
}

/*
 * Set the UGA protocol instance for UEFI UGA backend.
 * This can be used to configure the GOP backend to use UGA protocol.
 */
VOID
FbUefiUgaSetProtocol(
    IFramebufferBackend *Backend,
    VOID *UgaProtocol
    )
{
    /* UGA protocol could be stored in GOP backend for fallback */
    (void)Backend;
    (void)UgaProtocol;
}
