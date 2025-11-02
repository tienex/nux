/*++
    Module Name:

        factory.c

    Abstract:

        Framebuffer backend factory and registration system.

        Backends register themselves at initialization time, allowing
        the framework to remain agnostic about specific implementations.

--*/

#include <ananke/framebuffer/backends.h>

/* Maximum number of backend types */
#define FB_MAX_BACKEND_TYPES    100

/* Backend registry entry */
typedef struct _FB_BACKEND_REGISTRY_ENTRY {
    FB_BACKEND_TYPE         Type;
    FB_BACKEND_CONSTRUCTOR  Constructor;
} FB_BACKEND_REGISTRY_ENTRY;

/* Backend registry */
static FB_BACKEND_REGISTRY_ENTRY gBackendRegistry[FB_MAX_BACKEND_TYPES];
static UINT32 gBackendRegistryCount = 0;

/*
 * Register a backend constructor for a given type.
 * Backends call this during initialization to register themselves.
 */
VOID
FbRegisterBackend(
    IN FB_BACKEND_TYPE Type,
    IN FB_BACKEND_CONSTRUCTOR Constructor
    )
{
    if (gBackendRegistryCount >= FB_MAX_BACKEND_TYPES) {
        /* Registry full - silently ignore */
        return;
    }

    /* Check if already registered (allow override) */
    for (UINT32 i = 0; i < gBackendRegistryCount; i++) {
        if (gBackendRegistry[i].Type == Type) {
            gBackendRegistry[i].Constructor = Constructor;
            return;
        }
    }

    /* Add new entry */
    gBackendRegistry[gBackendRegistryCount].Type = Type;
    gBackendRegistry[gBackendRegistryCount].Constructor = Constructor;
    gBackendRegistryCount++;
}

/*
 * Initialize the backend registry.
 * Called automatically on first FbCreateBackend() call.
 * Each backend registers itself for one or more backend types.
 */
VOID
FbInitializeBackendRegistry(
    VOID
    )
{
    /* Generic backend */
    FbRegisterBackend(FbBackendGeneric, FbCreateGenericBackend);

    /* Hercules backend */
    FbRegisterBackend(FbBackendHercules, FbCreateHerculesBackend);

    /* PC Graphics backend (unified CGA/EGA/VGA/SVGA/VESA/XGA) */
    FbRegisterBackend(FbBackendPcGraphics, FbCreatePcGraphicsBackend);
    FbRegisterBackend(FbBackendVga16, FbCreatePcGraphicsBackend);
    FbRegisterBackend(FbBackendVesaLinear, FbCreatePcGraphicsBackend);
    FbRegisterBackend(FbBackendVesaBanked, FbCreatePcGraphicsBackend);

    /* UEFI backend (unified GOP/UGA/Apple EFI) */
    FbRegisterBackend(FbBackendUefiGop, FbCreateUefiBackend);
    FbRegisterBackend(FbBackendUefiUga, FbCreateUefiBackend);
    FbRegisterBackend(FbBackendAppleEfi, FbCreateUefiBackend);

    /* Unix framebuffer device backend (Linux, BSD, Solaris, etc.) */
#if defined(__linux__) || defined(__unix__)
    extern IFramebufferBackend *FbCreateFbdevBackend(VOID);
    FbRegisterBackend(FbBackendGeneric, FbCreateFbdevBackend);
#endif

    /* Terminal backend (unified ANSI/text mode) */
    extern IFramebufferBackend *FbCreateTerminalBackend(VOID);
    FbRegisterBackend(FbBackendGeneric, FbCreateTerminalBackend);

    /* TODO: Add registration for other platform-specific backends
     * (Amiga, Atari, Mac, Sun, SGI, NeXT, Acorn) when they are
     * compiled in. Each backend should register itself here. */
}

/*
 * Create a framebuffer backend by type.
 * Looks up the registered backend constructor for the given type.
 */
IFramebufferBackend *
FbCreateBackend(
    IN FB_BACKEND_TYPE Type
    )
{
    /* Initialize backends on first call */
    static BOOLEAN Initialized = FALSE;
    if (!Initialized) {
        FbInitializeBackendRegistry();
        Initialized = TRUE;
    }

    /* Look up in registry */
    for (UINT32 i = 0; i < gBackendRegistryCount; i++) {
        if (gBackendRegistry[i].Type == Type) {
            if (gBackendRegistry[i].Constructor != NULL) {
                return gBackendRegistry[i].Constructor();
            }
        }
    }

    /* Backend not found */
    return NULL;
}
