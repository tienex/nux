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

        case FbBackendVga16:
            return FbCreateVga16Backend();

        case FbBackendVesaLinear:
            return FbCreateVesaLinearBackend();

        case FbBackendVesaBanked:
            return FbCreateVesaBankedBackend();

        case FbBackendUefiGop:
            return FbCreateUefiGopBackend();

        case FbBackendAppleEfi:
            return FbCreateAppleEfiBackend();

        default:
            return NULL;
    }
}
