/*++
    Module Name:

        factory.c

    Abstract:

        Framebuffer backend factory implementation.

--*/

#include <ananke/framebuffer/backends.h>

IFramebufferBackend *
FbCreateBackend(
    IN FB_BACKEND_TYPE Type
    )
{
    switch (Type) {
        case FbBackendGeneric:
            return FbCreateGenericBackend();

        case FbBackendHercules:
            return FbCreateHerculesBackend();

        /* VGA16, VESA Linear, and VESA Banked are all handled by the
         * unified PC Graphics backend which supports all PC-compatible
         * graphics modes (CGA, EGA, VGA, SVGA, VESA) through the
         * FRAMEBUFFER_DESC memory organization descriptor. */
        case FbBackendVga16:
        case FbBackendVesaLinear:
        case FbBackendVesaBanked:
        case FbBackendPcGraphics:
            return FbCreatePcGraphicsBackend();

        /* UEFI GOP, UGA, and Apple EFI are all handled by the unified
         * UEFI GOP backend which supports all UEFI graphics protocols:
         * - Modern GOP (Graphics Output Protocol)
         * - Legacy UGA (Universal Graphics Adapter, EFI 1.x)
         * - Apple EFI quirks (BGR, Retina, non-standard resolutions) */
        case FbBackendUefiGop:
        case FbBackendUefiUga:
        case FbBackendAppleEfi:
            return FbCreateUefiGopBackend();

        default:
            return NULL;
    }
}
