/*++
    Module Name:

        palette.c

    Abstract:

        Palette management implementation for indexed color modes.

--*/

#include <ananke/framebuffer.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Standard VGA Palettes                                           */
/* --------------------------------------------------------------- */

/*
 * Standard VGA 256-color palette (6-bit RGB cube + grayscale).
 * This is the default VGA palette used by DOS, Windows, and most PC software.
 *
 * Layout:
 *   Entries 0-15:  Standard 16-color VGA palette (EGA compatible)
 *   Entries 16-231: 216-color 6×6×6 RGB cube (web-safe colors)
 *   Entries 232-255: 24-step grayscale ramp
 */
CONST FB_PALETTE_ENTRY gVga256Palette[256] = {
    /* Standard 16-color VGA palette */
    {   0,   0,   0, 0 },  /*  0: Black */
    {   0,   0, 170, 0 },  /*  1: Blue */
    {   0, 170,   0, 0 },  /*  2: Green */
    {   0, 170, 170, 0 },  /*  3: Cyan */
    { 170,   0,   0, 0 },  /*  4: Red */
    { 170,   0, 170, 0 },  /*  5: Magenta */
    { 170,  85,   0, 0 },  /*  6: Brown */
    { 170, 170, 170, 0 },  /*  7: Light Gray */
    {  85,  85,  85, 0 },  /*  8: Dark Gray */
    {  85,  85, 255, 0 },  /*  9: Light Blue */
    {  85, 255,  85, 0 },  /* 10: Light Green */
    {  85, 255, 255, 0 },  /* 11: Light Cyan */
    { 255,  85,  85, 0 },  /* 12: Light Red */
    { 255,  85, 255, 0 },  /* 13: Light Magenta */
    { 255, 255,  85, 0 },  /* 14: Yellow */
    { 255, 255, 255, 0 },  /* 15: White */

    /* Extended 216-color cube (6x6x6) */
    /* Entries 16-231: RGB cube */
    {   0,   0,   0, 0 }, {   0,   0,  51, 0 }, {   0,   0, 102, 0 }, {   0,   0, 153, 0 },
    {   0,   0, 204, 0 }, {   0,   0, 255, 0 }, {   0,  51,   0, 0 }, {   0,  51,  51, 0 },
    {   0,  51, 102, 0 }, {   0,  51, 153, 0 }, {   0,  51, 204, 0 }, {   0,  51, 255, 0 },
    {   0, 102,   0, 0 }, {   0, 102,  51, 0 }, {   0, 102, 102, 0 }, {   0, 102, 153, 0 },
    {   0, 102, 204, 0 }, {   0, 102, 255, 0 }, {   0, 153,   0, 0 }, {   0, 153,  51, 0 },
    {   0, 153, 102, 0 }, {   0, 153, 153, 0 }, {   0, 153, 204, 0 }, {   0, 153, 255, 0 },
    {   0, 204,   0, 0 }, {   0, 204,  51, 0 }, {   0, 204, 102, 0 }, {   0, 204, 153, 0 },
    {   0, 204, 204, 0 }, {   0, 204, 255, 0 }, {   0, 255,   0, 0 }, {   0, 255,  51, 0 },
    {   0, 255, 102, 0 }, {   0, 255, 153, 0 }, {   0, 255, 204, 0 }, {   0, 255, 255, 0 },
    {  51,   0,   0, 0 }, {  51,   0,  51, 0 }, {  51,   0, 102, 0 }, {  51,   0, 153, 0 },
    {  51,   0, 204, 0 }, {  51,   0, 255, 0 }, {  51,  51,   0, 0 }, {  51,  51,  51, 0 },
    {  51,  51, 102, 0 }, {  51,  51, 153, 0 }, {  51,  51, 204, 0 }, {  51,  51, 255, 0 },
    {  51, 102,   0, 0 }, {  51, 102,  51, 0 }, {  51, 102, 102, 0 }, {  51, 102, 153, 0 },
    {  51, 102, 204, 0 }, {  51, 102, 255, 0 }, {  51, 153,   0, 0 }, {  51, 153,  51, 0 },
    {  51, 153, 102, 0 }, {  51, 153, 153, 0 }, {  51, 153, 204, 0 }, {  51, 153, 255, 0 },
    {  51, 204,   0, 0 }, {  51, 204,  51, 0 }, {  51, 204, 102, 0 }, {  51, 204, 153, 0 },
    {  51, 204, 204, 0 }, {  51, 204, 255, 0 }, {  51, 255,   0, 0 }, {  51, 255,  51, 0 },
    {  51, 255, 102, 0 }, {  51, 255, 153, 0 }, {  51, 255, 204, 0 }, {  51, 255, 255, 0 },
    { 102,   0,   0, 0 }, { 102,   0,  51, 0 }, { 102,   0, 102, 0 }, { 102,   0, 153, 0 },
    { 102,   0, 204, 0 }, { 102,   0, 255, 0 }, { 102,  51,   0, 0 }, { 102,  51,  51, 0 },
    { 102,  51, 102, 0 }, { 102,  51, 153, 0 }, { 102,  51, 204, 0 }, { 102,  51, 255, 0 },
    { 102, 102,   0, 0 }, { 102, 102,  51, 0 }, { 102, 102, 102, 0 }, { 102, 102, 153, 0 },
    { 102, 102, 204, 0 }, { 102, 102, 255, 0 }, { 102, 153,   0, 0 }, { 102, 153,  51, 0 },
    { 102, 153, 102, 0 }, { 102, 153, 153, 0 }, { 102, 153, 204, 0 }, { 102, 153, 255, 0 },
    { 102, 204,   0, 0 }, { 102, 204,  51, 0 }, { 102, 204, 102, 0 }, { 102, 204, 153, 0 },
    { 102, 204, 204, 0 }, { 102, 204, 255, 0 }, { 102, 255,   0, 0 }, { 102, 255,  51, 0 },
    { 102, 255, 102, 0 }, { 102, 255, 153, 0 }, { 102, 255, 204, 0 }, { 102, 255, 255, 0 },
    { 153,   0,   0, 0 }, { 153,   0,  51, 0 }, { 153,   0, 102, 0 }, { 153,   0, 153, 0 },
    { 153,   0, 204, 0 }, { 153,   0, 255, 0 }, { 153,  51,   0, 0 }, { 153,  51,  51, 0 },
    { 153,  51, 102, 0 }, { 153,  51, 153, 0 }, { 153,  51, 204, 0 }, { 153,  51, 255, 0 },
    { 153, 102,   0, 0 }, { 153, 102,  51, 0 }, { 153, 102, 102, 0 }, { 153, 102, 153, 0 },
    { 153, 102, 204, 0 }, { 153, 102, 255, 0 }, { 153, 153,   0, 0 }, { 153, 153,  51, 0 },
    { 153, 153, 102, 0 }, { 153, 153, 153, 0 }, { 153, 153, 204, 0 }, { 153, 153, 255, 0 },
    { 153, 204,   0, 0 }, { 153, 204,  51, 0 }, { 153, 204, 102, 0 }, { 153, 204, 153, 0 },
    { 153, 204, 204, 0 }, { 153, 204, 255, 0 }, { 153, 255,   0, 0 }, { 153, 255,  51, 0 },
    { 153, 255, 102, 0 }, { 153, 255, 153, 0 }, { 153, 255, 204, 0 }, { 153, 255, 255, 0 },
    { 204,   0,   0, 0 }, { 204,   0,  51, 0 }, { 204,   0, 102, 0 }, { 204,   0, 153, 0 },
    { 204,   0, 204, 0 }, { 204,   0, 255, 0 }, { 204,  51,   0, 0 }, { 204,  51,  51, 0 },
    { 204,  51, 102, 0 }, { 204,  51, 153, 0 }, { 204,  51, 204, 0 }, { 204,  51, 255, 0 },
    { 204, 102,   0, 0 }, { 204, 102,  51, 0 }, { 204, 102, 102, 0 }, { 204, 102, 153, 0 },
    { 204, 102, 204, 0 }, { 204, 102, 255, 0 }, { 204, 153,   0, 0 }, { 204, 153,  51, 0 },
    { 204, 153, 102, 0 }, { 204, 153, 153, 0 }, { 204, 153, 204, 0 }, { 204, 153, 255, 0 },
    { 204, 204,   0, 0 }, { 204, 204,  51, 0 }, { 204, 204, 102, 0 }, { 204, 204, 153, 0 },
    { 204, 204, 204, 0 }, { 204, 204, 255, 0 }, { 204, 255,   0, 0 }, { 204, 255,  51, 0 },
    { 204, 255, 102, 0 }, { 204, 255, 153, 0 }, { 204, 255, 204, 0 }, { 204, 255, 255, 0 },
    { 255,   0,   0, 0 }, { 255,   0,  51, 0 }, { 255,   0, 102, 0 }, { 255,   0, 153, 0 },
    { 255,   0, 204, 0 }, { 255,   0, 255, 0 }, { 255,  51,   0, 0 }, { 255,  51,  51, 0 },
    { 255,  51, 102, 0 }, { 255,  51, 153, 0 }, { 255,  51, 204, 0 }, { 255,  51, 255, 0 },
    { 255, 102,   0, 0 }, { 255, 102,  51, 0 }, { 255, 102, 102, 0 }, { 255, 102, 153, 0 },
    { 255, 102, 204, 0 }, { 255, 102, 255, 0 }, { 255, 153,   0, 0 }, { 255, 153,  51, 0 },
    { 255, 153, 102, 0 }, { 255, 153, 153, 0 }, { 255, 153, 204, 0 }, { 255, 153, 255, 0 },
    { 255, 204,   0, 0 }, { 255, 204,  51, 0 }, { 255, 204, 102, 0 }, { 255, 204, 153, 0 },
    { 255, 204, 204, 0 }, { 255, 204, 255, 0 }, { 255, 255,   0, 0 }, { 255, 255,  51, 0 },
    { 255, 255, 102, 0 }, { 255, 255, 153, 0 }, { 255, 255, 204, 0 }, { 255, 255, 255, 0 },

    /* Grayscale ramp (entries 232-255) */
    {   8,   8,   8, 0 }, {  18,  18,  18, 0 }, {  28,  28,  28, 0 }, {  38,  38,  38, 0 },
    {  48,  48,  48, 0 }, {  58,  58,  58, 0 }, {  68,  68,  68, 0 }, {  78,  78,  78, 0 },
    {  88,  88,  88, 0 }, {  98,  98,  98, 0 }, { 108, 108, 108, 0 }, { 118, 118, 118, 0 },
    { 128, 128, 128, 0 }, { 138, 138, 138, 0 }, { 148, 148, 148, 0 }, { 158, 158, 158, 0 },
    { 168, 168, 168, 0 }, { 178, 178, 178, 0 }, { 188, 188, 188, 0 }, { 198, 198, 198, 0 },
    { 208, 208, 208, 0 }, { 218, 218, 218, 0 }, { 228, 228, 228, 0 }, { 238, 238, 238, 0 },
};

/* --------------------------------------------------------------- */
/*  Palette Manager Structure                                       */
/* --------------------------------------------------------------- */

typedef struct _FB_PALETTE_MGR {
    IFramebufferPalette         Base;
    REFOBJ                      RefCount;
    FB_PALETTE_ENTRY            Palette[256];
} FB_PALETTE_MGR;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE FbPalette_QueryInterface(
    IFramebufferPalette *This,
    REFIID riid,
    VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE FbPalette_AddRef(
    IFramebufferPalette *This);
static UINT32 STDMETHODCALLTYPE FbPalette_Release(
    IFramebufferPalette *This);
static HRESULT STDMETHODCALLTYPE FbPalette_SetPaletteEntry(
    IFramebufferPalette *This,
    UINT8 Index,
    FB_PALETTE_ENTRY Entry);
static HRESULT STDMETHODCALLTYPE FbPalette_GetPaletteEntry(
    IFramebufferPalette *This,
    UINT8 Index,
    FB_PALETTE_ENTRY *Entry);
static HRESULT STDMETHODCALLTYPE FbPalette_SetPalette(
    IFramebufferPalette *This,
    CONST FB_PALETTE_ENTRY *Palette,
    UINT32 Count);
static HRESULT STDMETHODCALLTYPE FbPalette_GetPalette(
    IFramebufferPalette *This,
    FB_PALETTE_ENTRY *Palette,
    UINT32 Count);
static HRESULT STDMETHODCALLTYPE FbPalette_LoadStandardVgaPalette(
    IFramebufferPalette *This);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferPaletteVtbl gFbPaletteVtbl = {
    .QueryInterface             = FbPalette_QueryInterface,
    .AddRef                     = FbPalette_AddRef,
    .Release                    = FbPalette_Release,
    .SetPaletteEntry            = FbPalette_SetPaletteEntry,
    .GetPaletteEntry            = FbPalette_GetPaletteEntry,
    .SetPalette                 = FbPalette_SetPalette,
    .GetPalette                 = FbPalette_GetPalette,
    .LoadStandardVgaPalette     = FbPalette_LoadStandardVgaPalette,
};

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbPalette_QueryInterface(
    IFramebufferPalette *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    FB_PALETTE_MGR *Manager = (FB_PALETTE_MGR *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferPalette)) {
        *ppvObject = &Manager->Base;
        FbPalette_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
FbPalette_AddRef(
    IFramebufferPalette *This
    )
{
    FB_PALETTE_MGR *Manager = (FB_PALETTE_MGR *)This;
    return ANX_REF_INC(&Manager->RefCount);
}

static UINT32 STDMETHODCALLTYPE
FbPalette_Release(
    IFramebufferPalette *This
    )
{
    FB_PALETTE_MGR *Manager = (FB_PALETTE_MGR *)This;
    return ANX_REF_DEC(&Manager->RefCount);
}

/* --------------------------------------------------------------- */
/*  IFramebufferPalette Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbPalette_SetPaletteEntry(
    IFramebufferPalette *This,
    UINT8 Index,
    FB_PALETTE_ENTRY Entry
    )
{
    FB_PALETTE_MGR *Manager = (FB_PALETTE_MGR *)This;
    Manager->Palette[Index] = Entry;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPalette_GetPaletteEntry(
    IFramebufferPalette *This,
    UINT8 Index,
    FB_PALETTE_ENTRY *Entry
    )
{
    FB_PALETTE_MGR *Manager = (FB_PALETTE_MGR *)This;

    if (Entry == NULL) {
        return E_POINTER;
    }

    *Entry = Manager->Palette[Index];
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPalette_SetPalette(
    IFramebufferPalette *This,
    CONST FB_PALETTE_ENTRY *Palette,
    UINT32 Count
    )
{
    FB_PALETTE_MGR *Manager = (FB_PALETTE_MGR *)This;
    UINT32 i;

    if (Palette == NULL) {
        return E_POINTER;
    }

    if (Count > 256) {
        Count = 256;
    }

    for (i = 0; i < Count; i++) {
        Manager->Palette[i] = Palette[i];
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPalette_GetPalette(
    IFramebufferPalette *This,
    FB_PALETTE_ENTRY *Palette,
    UINT32 Count
    )
{
    FB_PALETTE_MGR *Manager = (FB_PALETTE_MGR *)This;
    UINT32 i;

    if (Palette == NULL) {
        return E_POINTER;
    }

    if (Count > 256) {
        Count = 256;
    }

    for (i = 0; i < Count; i++) {
        Palette[i] = Manager->Palette[i];
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPalette_LoadStandardVgaPalette(
    IFramebufferPalette *This
    )
{
    return FbPalette_SetPalette(This, gVga256Palette, 256);
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static FB_PALETTE_MGR gPaletteMgrInstance = {
    .Base.lpVtbl        = &gFbPaletteVtbl,
    .RefCount.RefCount  = 1,
};

IFramebufferPalette *
FbCreatePaletteManager(
    VOID
    )
{
    /* Initialize with standard VGA palette */
    FbPalette_LoadStandardVgaPalette((IFramebufferPalette *)&gPaletteMgrInstance);
    return (IFramebufferPalette *)&gPaletteMgrInstance;
}

/*
 * Standard VGA 16-color palette (EGA compatible).
 * This is just the first 16 entries of gVga256Palette.
 */
CONST FB_PALETTE_ENTRY gVga16Palette[16] = {
    {   0,   0,   0, 0 },  /*  0: Black */
    {   0,   0, 170, 0 },  /*  1: Blue */
    {   0, 170,   0, 0 },  /*  2: Green */
    {   0, 170, 170, 0 },  /*  3: Cyan */
    { 170,   0,   0, 0 },  /*  4: Red */
    { 170,   0, 170, 0 },  /*  5: Magenta */
    { 170,  85,   0, 0 },  /*  6: Brown */
    { 170, 170, 170, 0 },  /*  7: Light Gray */
    {  85,  85,  85, 0 },  /*  8: Dark Gray */
    {  85,  85, 255, 0 },  /*  9: Light Blue */
    {  85, 255,  85, 0 },  /* 10: Light Green */
    {  85, 255, 255, 0 },  /* 11: Light Cyan */
    { 255,  85,  85, 0 },  /* 12: Light Red */
    { 255,  85, 255, 0 },  /* 13: Light Magenta */
    { 255, 255,  85, 0 },  /* 14: Yellow */
    { 255, 255, 255, 0 },  /* 15: White */
};
