/*++
    Module Name:

        backends_init.c

    Abstract:

        Backend initialization module.

        This is the ONE file that knows about all available backends.
        It calls each backend's registration function to register them
        with the factory.

        The factory itself (factory.c) remains backend-agnostic and only
        provides the registry mechanism. This separation allows backends
        to be added or removed by modifying only this file.

--*/

#include <ananke/framebuffer/backends.h>

/* --------------------------------------------------------------- */
/*  External Backend Registration Functions                        */
/* --------------------------------------------------------------- */

/* Each backend exposes a registration function */
extern VOID FbRegisterGenericBackend(VOID);
extern VOID FbRegisterHerculesBackend(VOID);
extern VOID FbRegisterPcGraphicsBackend(VOID);
extern VOID FbRegisterUefiBackend(VOID);
extern VOID FbRegisterTerminalBackend(VOID);

#if defined(__linux__) || defined(__unix__) || defined(__FreeBSD__) || \
    defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || \
    defined(__sun) || defined(__HAIKU__)
extern VOID FbRegisterFbdevBackend(VOID);
#endif

/* --------------------------------------------------------------- */
/*  Backend Initialization                                         */
/* --------------------------------------------------------------- */

/*
 * Initialize all available backends.
 * Called automatically on first FbCreateBackend() call.
 *
 * This function calls each backend's registration function,
 * which in turn registers the backend for one or more backend types.
 */
VOID
FbInitializeBackends(
    VOID
    )
{
    /* Register all backends */
    FbRegisterGenericBackend();
    FbRegisterHerculesBackend();
    FbRegisterPcGraphicsBackend();
    FbRegisterUefiBackend();
    FbRegisterTerminalBackend();

    /* Platform-specific backends */
#if defined(__linux__) || defined(__unix__) || defined(__FreeBSD__) || \
    defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || \
    defined(__sun) || defined(__HAIKU__)
    FbRegisterFbdevBackend();
#endif
}
