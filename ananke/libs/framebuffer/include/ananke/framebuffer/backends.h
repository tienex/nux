/*++
    Module Name:

        backends.h

    Abstract:

        Framebuffer backend constructors and factory functions.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/framebuffer.h>

/* --------------------------------------------------------------- */
/*  Backend Constructors                                            */
/* --------------------------------------------------------------- */

/*
 * Create a generic framebuffer backend.
 * Supports all RGB formats.
 */
IFramebufferBackend *
FbCreateGenericBackend(
    VOID
    );

/*
 * Create a text renderer for a given backend.
 */
IFramebufferText *
FbCreateTextRenderer(
    IN IFramebufferBackend *Backend
    );

/*
 * Create a palette manager.
 */
IFramebufferPalette *
FbCreatePaletteManager(
    VOID
    );
