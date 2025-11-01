/*++
    Module Name:

        palette.h

    Abstract:

        Palette interface for indexed color modes.
        Supports palette animation for effects and smooth color transitions.

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
/*  Palette Animation Definitions                                  */
/* --------------------------------------------------------------- */

typedef struct _FB_PALETTE_ANIMATION {
    UINT32              StartIndex;     /* Starting palette index */
    UINT32              Count;          /* Number of colors to animate */
    UINT32              FrameCount;     /* Number of animation frames */
    UINT32              CurrentFrame;   /* Current frame index */
    UINT32              DelayMs;        /* Delay between frames in milliseconds */
    FB_PALETTE_ENTRY    *Frames;        /* Array of palette entries (Count * FrameCount) */
    BOOLEAN             Loop;           /* Loop animation when complete */
    BOOLEAN             Active;         /* Animation is active */
} FB_PALETTE_ANIMATION;

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

    /* Palette animation support */

    /* Create a palette animation */
    ANX_IFACE_METHOD(HRESULT, CreateAnimation, (
        IN UINT32 StartIndex,
        IN UINT32 Count,
        IN UINT32 FrameCount,
        IN UINT32 DelayMs,
        IN CONST FB_PALETTE_ENTRY *Frames,
        IN BOOLEAN Loop,
        OUT UINT32 *AnimationId))

    /* Start a palette animation */
    ANX_IFACE_METHOD(HRESULT, StartAnimation, (
        IN UINT32 AnimationId))

    /* Stop a palette animation */
    ANX_IFACE_METHOD(HRESULT, StopAnimation, (
        IN UINT32 AnimationId))

    /* Update all active animations (should be called regularly) */
    ANX_IFACE_METHOD(HRESULT, UpdateAnimations, (
        VOID))

    /* Remove a palette animation */
    ANX_IFACE_METHOD(HRESULT, RemoveAnimation, (
        IN UINT32 AnimationId))

    /* Fade palette to black or white */
    ANX_IFACE_METHOD(HRESULT, FadeTo, (
        IN UINT32 StartIndex,
        IN UINT32 Count,
        IN CONST FB_PALETTE_ENTRY *TargetPalette,
        IN UINT32 Steps,
        IN UINT32 DelayMs))

    /* Rotate palette entries (for smooth scrolling effects) */
    ANX_IFACE_METHOD(HRESULT, RotateEntries, (
        IN UINT32 StartIndex,
        IN UINT32 Count,
        IN INT32 Offset))

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
#define IFramebufferPalette_CreateAnimation(This, Start, Count, Frames, Delay, Data, Loop, Id) \
    ((This)->lpVtbl->CreateAnimation(This, Start, Count, Frames, Delay, Data, Loop, Id))
#define IFramebufferPalette_StartAnimation(This, Id) \
    ((This)->lpVtbl->StartAnimation(This, Id))
#define IFramebufferPalette_StopAnimation(This, Id) \
    ((This)->lpVtbl->StopAnimation(This, Id))
#define IFramebufferPalette_UpdateAnimations(This) \
    ((This)->lpVtbl->UpdateAnimations(This))
#define IFramebufferPalette_RemoveAnimation(This, Id) \
    ((This)->lpVtbl->RemoveAnimation(This, Id))
#define IFramebufferPalette_FadeTo(This, Start, Count, Target, Steps, Delay) \
    ((This)->lpVtbl->FadeTo(This, Start, Count, Target, Steps, Delay))
#define IFramebufferPalette_RotateEntries(This, Start, Count, Offset) \
    ((This)->lpVtbl->RotateEntries(This, Start, Count, Offset))

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
