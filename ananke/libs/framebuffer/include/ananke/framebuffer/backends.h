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
/*  Backend Type Enumeration                                        */
/* --------------------------------------------------------------- */

typedef enum _FB_BACKEND_TYPE {
    FbBackendGeneric        = 0,  /* Generic software renderer */
    FbBackendHercules       = 1,  /* Hercules Graphics Card (720x348, 1BPP) */
    FbBackendVga16          = 2,  /* VGA 16-color planar mode */
    FbBackendVesaLinear     = 3,  /* VESA linear framebuffer */
    FbBackendVesaBanked     = 4,  /* VESA banked/segmented mode */
    FbBackendUefiGop        = 5,  /* UEFI Graphics Output Protocol */
    FbBackendAppleEfi       = 6,  /* Apple EFI framebuffer */
} FB_BACKEND_TYPE;

/* --------------------------------------------------------------- */
/*  Backend Constructors                                            */
/* --------------------------------------------------------------- */

/*
 * Create a generic framebuffer backend.
 * Supports all RGB formats with software rendering.
 */
IFramebufferBackend *
FbCreateGenericBackend(
    VOID
    );

/*
 * Create a Hercules Graphics Card backend.
 * 720x348 monochrome (1BPP) with bank-interleaved memory.
 */
IFramebufferBackend *
FbCreateHerculesBackend(
    VOID
    );

/*
 * Create a VGA 16-color planar mode backend.
 * Supports standard VGA graphics modes (640x480x16, etc.).
 */
IFramebufferBackend *
FbCreateVga16Backend(
    VOID
    );

/*
 * Create a VESA linear framebuffer backend.
 * For VESA 2.0+ linear framebuffer modes (LFB).
 */
IFramebufferBackend *
FbCreateVesaLinearBackend(
    VOID
    );

/*
 * Create a VESA banked/segmented framebuffer backend.
 * For VESA 1.x/2.0 banked modes with 64KB window.
 */
IFramebufferBackend *
FbCreateVesaBankedBackend(
    VOID
    );

/*
 * Set the bank switching function for VESA banked backend.
 * Must be called after backend creation.
 */
VOID
FbVesaBankedSetBankFunction(
    IN IFramebufferBackend *Backend,
    IN VOID (*SetBankFunc)(UINT32)
    );

/*
 * Create a UEFI Graphics Output Protocol (GOP) backend.
 * For modern UEFI systems.
 */
IFramebufferBackend *
FbCreateUefiGopBackend(
    VOID
    );

/*
 * Set the GOP protocol instance for UEFI GOP backend.
 * This is optional but recommended for accelerated operations.
 */
VOID
FbUefiGopSetProtocol(
    IN IFramebufferBackend *Backend,
    IN VOID *GopProtocol
    );

/*
 * Create an Apple EFI framebuffer backend.
 * Handles Apple-specific quirks (BGR mode, Retina displays).
 */
IFramebufferBackend *
FbCreateAppleEfiBackend(
    VOID
    );

/* --------------------------------------------------------------- */
/*  Text and Palette Components                                     */
/* --------------------------------------------------------------- */

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

/* --------------------------------------------------------------- */
/*  Backend Factory                                                 */
/* --------------------------------------------------------------- */

/*
 * Create a framebuffer backend by type.
 * This is a convenience function that calls the appropriate constructor.
 */
IFramebufferBackend *
FbCreateBackend(
    IN FB_BACKEND_TYPE Type
    );
