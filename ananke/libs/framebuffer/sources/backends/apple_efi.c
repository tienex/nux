/*++
    Module Name:

        apple_efi.c

    Abstract:

        Apple EFI framebuffer backend (consolidated into uefi_gop.c).

        This file now contains only a compatibility shim. The actual
        implementation has been merged into the unified UEFI GOP backend
        (uefi_gop.c) which handles all UEFI graphics protocols including:
        - Modern UEFI GOP (Graphics Output Protocol)
        - Legacy UGA (Universal Graphics Adapter, EFI 1.x)
        - Apple EFI quirks (BGR mode, Retina displays, non-standard resolutions)

        The UEFI GOP backend automatically detects and handles Apple-specific
        framebuffer configurations through the FRAMEBUFFER_DESC structure.

--*/

#include <ananke/framebuffer/backends.h>

/* Forward declaration */
IFramebufferBackend *FbCreateUefiGopBackend(VOID);

/*
 * Create an Apple EFI framebuffer backend.
 *
 * This function is maintained for API compatibility but returns
 * a UEFI GOP backend instance. The UEFI GOP backend (uefi_gop.c)
 * handles all UEFI framebuffers including Apple-specific configurations.
 */
IFramebufferBackend *
FbCreateAppleEfiBackend(
    VOID
    )
{
    return FbCreateUefiGopBackend();
}
