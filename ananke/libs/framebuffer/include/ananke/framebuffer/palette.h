/*++
    Module Name:

        palette.h

    Abstract:

        Palette interface for indexed color modes.
        Simple palette entry manipulation for color cycling and effects.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/framebuffer.h>

/* --------------------------------------------------------------- */
/*  IFramebufferPalette - Palette interface                        */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferPalette "FB000015-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferPalette,
    0xFB000015, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferPalette, IUnknown,
    IID_IFramebufferPalette, ANX_IID_IFramebufferPalette)

    /* Get palette size (number of colors) */
    ANX_IFACE_METHOD(HRESULT, GetSize, (
        OUT UINT32 *Size))

    /* Get a single palette entry */
    ANX_IFACE_METHOD(HRESULT, GetEntry, (
        IN UINT32 Index,
        OUT FB_PALETTE_ENTRY *Entry))

    /* Set a single palette entry */
    ANX_IFACE_METHOD(HRESULT, SetEntry, (
        IN UINT32 Index,
        IN CONST FB_PALETTE_ENTRY *Entry))

    /* Get multiple palette entries */
    ANX_IFACE_METHOD(HRESULT, GetEntries, (
        IN UINT32 StartIndex,
        IN UINT32 Count,
        OUT FB_PALETTE_ENTRY *Entries))

    /* Set multiple palette entries */
    ANX_IFACE_METHOD(HRESULT, SetEntries, (
        IN UINT32 StartIndex,
        IN UINT32 Count,
        IN CONST FB_PALETTE_ENTRY *Entries))

    /* Get entire palette */
    ANX_IFACE_METHOD(HRESULT, GetPalette, (
        OUT FB_PALETTE_ENTRY *Palette,
        IN UINT32 MaxEntries))

    /* Set entire palette */
    ANX_IFACE_METHOD(HRESULT, SetPalette, (
        IN CONST FB_PALETTE_ENTRY *Palette,
        IN UINT32 Count))

ANX_END_INTERFACE(IFramebufferPalette)

/* --------------------------------------------------------------- */
/*  Helper Macros for C (COBJMACROS style)                         */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IFramebufferPalette_GetSize(This, Size) \
    ((This)->lpVtbl->GetSize(This, Size))
#define IFramebufferPalette_GetEntry(This, Index, Entry) \
    ((This)->lpVtbl->GetEntry(This, Index, Entry))
#define IFramebufferPalette_SetEntry(This, Index, Entry) \
    ((This)->lpVtbl->SetEntry(This, Index, Entry))
#define IFramebufferPalette_GetEntries(This, Start, Count, Entries) \
    ((This)->lpVtbl->GetEntries(This, Start, Count, Entries))
#define IFramebufferPalette_SetEntries(This, Start, Count, Entries) \
    ((This)->lpVtbl->SetEntries(This, Start, Count, Entries))
#define IFramebufferPalette_GetPalette(This, Palette, Max) \
    ((This)->lpVtbl->GetPalette(This, Palette, Max))
#define IFramebufferPalette_SetPalette(This, Palette, Count) \
    ((This)->lpVtbl->SetPalette(This, Palette, Count))

#endif /* !__cplusplus */

/* --------------------------------------------------------------- */
/*  Standard Palettes                                              */
/* --------------------------------------------------------------- */

/* VGA 16-color palette */
extern CONST FB_PALETTE_ENTRY gVga16Palette[16];

/* VGA 256-color palette (6-bit RGB cube + grayscale) */
extern CONST FB_PALETTE_ENTRY gVga256Palette[256];

/* Grayscale palette (256 shades) */
extern CONST FB_PALETTE_ENTRY gGrayscalePalette[256];

/* Web-safe 216-color palette */
extern CONST FB_PALETTE_ENTRY gWebSafePalette[216];
