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
    /* Generic and modern backends */
    FbBackendGeneric        = 0,  /* Generic software renderer */
    FbBackendUefiGop        = 1,  /* UEFI Graphics Output Protocol */
    FbBackendVesaLinear     = 2,  /* VESA linear framebuffer */
    FbBackendVesaBanked     = 3,  /* VESA banked/segmented mode */

    /* IBM PC graphics adapters */
    FbBackendCga            = 10, /* CGA (320x200x4, 640x200x2) */
    FbBackendEga            = 11, /* EGA (640x350x16 planar) */
    FbBackendVga            = 12, /* VGA (Mode 13h, planar modes) */
    FbBackendVga16          = 13, /* VGA 16-color planar mode */
    FbBackendSvga           = 14, /* SVGA extended modes */
    FbBackendXga            = 15, /* XGA modes */
    FbBackendHercules       = 16, /* Hercules Graphics Card (720x348, 1BPP) */

    /* Apple platforms */
    FbBackendAppleEfi       = 20, /* Apple EFI framebuffer */
    FbBackendMacClassic     = 21, /* Classic Macintosh framebuffer */
    FbBackendMacValkyrie    = 22, /* Mac Valkyrie video */
    FbBackendAppleLisa      = 23, /* Apple Lisa framebuffer */

    /* Commodore Amiga */
    FbBackendAmigaOcs       = 30, /* Amiga OCS chipset */
    FbBackendAmigaEcs       = 31, /* Amiga ECS chipset */
    FbBackendAmigaAga       = 32, /* Amiga AGA chipset */

    /* Atari */
    FbBackendAtariSt        = 40, /* Atari ST */
    FbBackendAtariTt        = 41, /* Atari TT */
    FbBackendAtariFalcon    = 42, /* Atari Falcon */

    /* Sun Microsystems */
    FbBackendSunCgThree     = 50, /* Sun cgthree */
    FbBackendSunCgSix       = 51, /* Sun cgsix */
    FbBackendSunBwTwo       = 52, /* Sun bwtwo (monochrome) */
    FbBackendSunCgFour      = 53, /* Sun cgfour */

    /* SGI */
    FbBackendSgiNewport     = 60, /* SGI Newport graphics */
    FbBackendSgiImpact      = 61, /* SGI Impact graphics */

    /* NeXT */
    FbBackendNext           = 70, /* NeXT Display */

    /* Acorn */
    FbBackendAcornVidc      = 80, /* Acorn VIDC (ARM) */
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
/*  IBM PC Graphics Adapters                                        */
/* --------------------------------------------------------------- */

/*
 * Create a CGA backend.
 * Supports 320x200x4 and 640x200x2 modes.
 */
IFramebufferBackend *
FbCreateCgaBackend(
    VOID
    );

/*
 * Create an EGA backend.
 * Supports 640x350x16 planar mode.
 */
IFramebufferBackend *
FbCreateEgaBackend(
    VOID
    );

/*
 * Create a VGA backend with Mode 13h support.
 * Supports 320x200x256 linear mode.
 */
IFramebufferBackend *
FbCreateVgaBackend(
    VOID
    );

/*
 * Create an SVGA backend.
 * Supports extended SVGA modes.
 */
IFramebufferBackend *
FbCreateSvgaBackend(
    VOID
    );

/*
 * Create an XGA backend.
 * Supports XGA extended modes.
 */
IFramebufferBackend *
FbCreateXgaBackend(
    VOID
    );

/* --------------------------------------------------------------- */
/*  Apple Platforms                                                 */
/* --------------------------------------------------------------- */

/*
 * Create a classic Macintosh framebuffer backend.
 * Supports 1-bit monochrome display.
 */
IFramebufferBackend *
FbCreateMacClassicBackend(
    VOID
    );

/*
 * Create a Macintosh Valkyrie video backend.
 * Supports color modes on later compact Macs.
 */
IFramebufferBackend *
FbCreateMacValkyrieBackend(
    VOID
    );

/*
 * Create an Apple Lisa framebuffer backend.
 * Supports Lisa's 720x364 monochrome display.
 */
IFramebufferBackend *
FbCreateAppleLisaBackend(
    VOID
    );

/* --------------------------------------------------------------- */
/*  Commodore Amiga                                                 */
/* --------------------------------------------------------------- */

/*
 * Create an Amiga OCS chipset backend.
 * Supports planar modes up to 32 colors.
 */
IFramebufferBackend *
FbCreateAmigaOcsBackend(
    VOID
    );

/*
 * Create an Amiga ECS chipset backend.
 * Supports planar modes up to 64 colors.
 */
IFramebufferBackend *
FbCreateAmigaEcsBackend(
    VOID
    );

/*
 * Create an Amiga AGA chipset backend.
 * Supports chunky modes and HAM8.
 */
IFramebufferBackend *
FbCreateAmigaAgaBackend(
    VOID
    );

/* --------------------------------------------------------------- */
/*  Atari                                                           */
/* --------------------------------------------------------------- */

/*
 * Create an Atari ST backend.
 * Supports ST low/medium/high resolution modes.
 */
IFramebufferBackend *
FbCreateAtariStBackend(
    VOID
    );

/*
 * Create an Atari TT backend.
 * Supports TT high color modes.
 */
IFramebufferBackend *
FbCreateAtariTtBackend(
    VOID
    );

/*
 * Create an Atari Falcon backend.
 * Supports Falcon true color modes.
 */
IFramebufferBackend *
FbCreateAtariFalconBackend(
    VOID
    );

/* --------------------------------------------------------------- */
/*  Sun Microsystems                                                */
/* --------------------------------------------------------------- */

/*
 * Create a Sun cgthree backend.
 * 8-bit color framebuffer.
 */
IFramebufferBackend *
FbCreateSunCgThreeBackend(
    VOID
    );

/*
 * Create a Sun cgsix backend.
 * Accelerated 8-bit color framebuffer.
 */
IFramebufferBackend *
FbCreateSunCgSixBackend(
    VOID
    );

/*
 * Create a Sun bwtwo backend.
 * 1-bit monochrome framebuffer.
 */
IFramebufferBackend *
FbCreateSunBwTwoBackend(
    VOID
    );

/*
 * Create a Sun cgfour backend.
 * 8-bit color framebuffer.
 */
IFramebufferBackend *
FbCreateSunCgFourBackend(
    VOID
    );

/* --------------------------------------------------------------- */
/*  SGI                                                             */
/* --------------------------------------------------------------- */

/*
 * Create an SGI Newport graphics backend.
 * Supports SGI Indy/Indigo2 graphics.
 */
IFramebufferBackend *
FbCreateSgiNewportBackend(
    VOID
    );

/*
 * Create an SGI Impact graphics backend.
 * Supports high-end SGI workstation graphics.
 */
IFramebufferBackend *
FbCreateSgiImpactBackend(
    VOID
    );

/* --------------------------------------------------------------- */
/*  NeXT                                                            */
/* --------------------------------------------------------------- */

/*
 * Create a NeXT Display backend.
 * Supports 2-bit grayscale display.
 */
IFramebufferBackend *
FbCreateNextBackend(
    VOID
    );

/* --------------------------------------------------------------- */
/*  Acorn                                                           */
/* --------------------------------------------------------------- */

/*
 * Create an Acorn VIDC backend.
 * Supports ARM-based Acorn machines.
 */
IFramebufferBackend *
FbCreateAcornVidcBackend(
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
