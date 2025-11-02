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

        case FbBackendUefiGop:
            return FbCreateUefiGopBackend();

        case FbBackendUefiUga:
            return FbCreateUefiUgaBackend();

        case FbBackendAppleEfi:
            return FbCreateAppleEfiBackend();

        default:
            return NULL;
    }
}
