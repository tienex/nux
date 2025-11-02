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
    FbBackendUefiUga        = 2,  /* UEFI Universal Graphics Adapter (EFI 1.x) */
    FbBackendVesaLinear     = 3,  /* VESA linear framebuffer */
    FbBackendVesaBanked     = 4,  /* VESA banked/segmented mode */

    /* IBM PC graphics adapters */
    FbBackendPcGraphics     = 10, /* Unified PC graphics (CGA/EGA/VGA/SVGA/XGA) */
    FbBackendHercules       = 11, /* Hercules Graphics Card (720x348, 1BPP) */
    FbBackendVga16          = 12, /* VGA 16-color (alias for PcGraphics) */

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
 * Create a UEFI framebuffer backend.
 * Unified backend for all UEFI graphics protocols:
 * - GOP (Graphics Output Protocol) - UEFI 2.x standard
 * - UGA (Universal Graphics Adapter) - EFI 1.x legacy
 * - Apple EFI quirks (BGR mode, Retina displays, non-standard resolutions)
 */
IFramebufferBackend *
FbCreateUefiBackend(
    VOID
    );

/*
 * Set the GOP protocol instance for UEFI backend.
 * This is optional but recommended for hardware-accelerated operations.
 */
VOID
FbUefiSetProtocol(
    IN IFramebufferBackend *Backend,
    IN VOID *GopProtocol
    );

/* --------------------------------------------------------------- */
/*  IBM PC Graphics Adapters                                        */
/* --------------------------------------------------------------- */

/*
 * Create a unified PC graphics adapter backend.
 *
 * Supports all IBM PC-compatible graphics modes through the
 * FRAMEBUFFER_DESC structure which describes memory organization:
 *
 * - CGA modes (320x200x4, 640x200x2)
 *   - FbPixelFormatIndexed4 with FbMemoryInterleaved
 *   - BankInterleave=2, BankOffset=0x2000 for even/odd scanlines
 *
 * - EGA modes (640x350x16 planar)
 *   - FbPixelFormatPlanar4 with FbMemoryPlanar
 *   - NumPlanes=4
 *
 * - VGA modes
 *   - Mode 13h: 320x200x256 (FbPixelFormatIndexed256, FbMemoryLinear)
 *   - Mode 12h: 640x480x16 (FbPixelFormatPlanar4, FbMemoryPlanar)
 *
 * - SVGA modes (800x600+)
 *   - FbMemoryLinear or FbMemoryBanked depending on mode
 *   - Various pixel formats (Indexed256, RGB555, RGB565, etc.)
 *
 * - XGA modes (1024x768+)
 *   - Usually FbMemoryLinear with RGB formats
 *
 * The FRAMEBUFFER_DESC structure describes how memory is organized,
 * not which hardware it is. This allows the same backend to handle
 * any PC-compatible graphics mode.
 */
IFramebufferBackend *
FbCreatePcGraphicsBackend(
    VOID
    );

/*
 * Set bank switching function for VESA banked modes.
 * Must be called after backend creation for banked modes.
 */
VOID
FbPcGraphicsSetBankFunction(
    IN IFramebufferBackend *Backend,
    IN VOID (*BankSwitchFunc)(UINT32)
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
/*  Backend Factory and Registration                                */
/* --------------------------------------------------------------- */

/*
 * Backend constructor function type.
 * Backends implement this signature to create instances.
 */
typedef IFramebufferBackend *(*FB_BACKEND_CONSTRUCTOR)(VOID);

/*
 * Register a backend constructor for a given type.
 * Backends call this during initialization to register themselves.
 * Multiple types can map to the same constructor (e.g., VGA16 and VESA → PC Graphics).
 */
VOID
FbRegisterBackend(
    IN FB_BACKEND_TYPE Type,
    IN FB_BACKEND_CONSTRUCTOR Constructor
    );

/*
 * Initialize the backend registry.
 * This is called automatically on first FbCreateBackend() call.
 * Backends register themselves during this initialization.
 */
VOID
FbInitializeBackendRegistry(
    VOID
    );

/*
 * Create a framebuffer backend by type.
 * Looks up the registered backend constructor for the given type.
 */
IFramebufferBackend *
FbCreateBackend(
    IN FB_BACKEND_TYPE Type
    );
