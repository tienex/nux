/*++
    Module Name:

        surface.c

    Abstract:

        IFramebufferSurface implementation.

        Represents a software surface (off-screen buffer) for rendering
        operations. Used for double-buffering, off-screen composition,
        and image manipulation before blitting to the screen.

--*/

#include <ananke/framebuffer/screen.h>
#include <ananke/framebuffer/engine.h>
#include <ananke/framebuffer/backends.h>
#include <ananke/framebuffer/palette.h>
#include <ananke/framebuffer/com_helpers.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Surface Implementation Structure                               */
/* --------------------------------------------------------------- */

typedef struct _FB_SURFACE_IMPL {
    IFramebufferSurface     Base;
    REFOBJ                  RefCount;
    IFramebufferBackend     *Backend;
    FB_ENGINE_CONTEXT       *Engine;
    FRAMEBUFFER_DESC        Descriptor;
    IFramebufferPalette     *Palette;
    IFramebufferText        *Text;
    UINT8                   *Memory;
    UINT32                  MemorySize;
    BOOLEAN                 IsLocked;
    UINT32                  LockCount;
} FB_SURFACE_IMPL;

/* --------------------------------------------------------------- */
/*  Forward Declarations                                            */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE FbSurface_QueryInterface(
    IFramebufferSurface *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE FbSurface_AddRef(IFramebufferSurface *This);
static UINT32 STDMETHODCALLTYPE FbSurface_Release(IFramebufferSurface *This);
static HRESULT STDMETHODCALLTYPE FbSurface_GetDescriptor(
    IFramebufferSurface *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE FbSurface_Clear(
    IFramebufferSurface *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbSurface_SetPixel(
    IFramebufferSurface *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbSurface_GetPixel(
    IFramebufferSurface *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE FbSurface_FillRect(
    IFramebufferSurface *This, CONST FB_RECT *Rect, FB_COLOR Color, FB_ROP Rop);
static HRESULT STDMETHODCALLTYPE FbSurface_BlitImage(
    IFramebufferSurface *This, INT32 DestX, INT32 DestY,
    IFramebufferImage *Image, CONST FB_RECT *SourceRect, FB_ROP Rop);
static HRESULT STDMETHODCALLTYPE FbSurface_BlitToScreen(
    IFramebufferSurface *This, IFramebufferScreen *Screen,
    INT32 DestX, INT32 DestY, CONST FB_RECT *SourceRect, FB_ROP Rop);
static HRESULT STDMETHODCALLTYPE FbSurface_GetPalette(
    IFramebufferSurface *This, IFramebufferPalette **Palette);
static HRESULT STDMETHODCALLTYPE FbSurface_GetText(
    IFramebufferSurface *This, IFramebufferText **Text);
static HRESULT STDMETHODCALLTYPE FbSurface_Lock(
    IFramebufferSurface *This, VOID **MemoryAddress, UINT32 *Pitch);
static HRESULT STDMETHODCALLTYPE FbSurface_Unlock(
    IFramebufferSurface *This);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferSurfaceVtbl gSurfaceVtbl = {
    .QueryInterface = FbSurface_QueryInterface,
    .AddRef         = FbSurface_AddRef,
    .Release        = FbSurface_Release,
    .GetDescriptor  = FbSurface_GetDescriptor,
    .Clear          = FbSurface_Clear,
    .SetPixel       = FbSurface_SetPixel,
    .GetPixel       = FbSurface_GetPixel,
    .FillRect       = FbSurface_FillRect,
    .BlitImage      = FbSurface_BlitImage,
    .BlitToScreen   = FbSurface_BlitToScreen,
    .GetPalette     = FbSurface_GetPalette,
    .GetText        = FbSurface_GetText,
    .Lock           = FbSurface_Lock,
    .Unlock         = FbSurface_Unlock,
};

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbSurface_QueryInterface(
    IFramebufferSurface *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (ANX_GUID_EQUALS(riid, &IID_IUnknown) ||
        ANX_GUID_EQUALS(riid, &IID_IFramebufferSurface)) {
        *ppvObject = &Surface->Base;
        IUnknown_AddRef((IUnknown *)&Surface->Base);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
FbSurface_AddRef(
    IFramebufferSurface *This
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;
    return ANX_REF_INC(&Surface->RefCount);
}

static UINT32 STDMETHODCALLTYPE
FbSurface_Release(
    IFramebufferSurface *This
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;
    UINT32 RefCount = ANX_REF_DEC(&Surface->RefCount);

    if (RefCount == 0) {
        /* Release child interfaces */
        if (Surface->Palette != NULL) {
            IUnknown_Release((IUnknown *)Surface->Palette);
        }
        if (Surface->Text != NULL) {
            IUnknown_Release((IUnknown *)Surface->Text);
        }

        /* Destroy engine */
        if (Surface->Engine != NULL) {
            FbDestroyEngine(Surface->Engine);
        }

        /* Release backend */
        if (Surface->Backend != NULL) {
            IUnknown_Release((IUnknown *)Surface->Backend);
        }

        /* Free memory */
        if (Surface->Memory != NULL) {
            ANX_FREE(Surface->Memory);
        }

        /* Free surface object */
        ANX_FREE(Surface);
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferSurface Implementation                              */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbSurface_GetDescriptor(
    IFramebufferSurface *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    ANX_MEMCPY(Descriptor, &Surface->Descriptor, sizeof(FRAMEBUFFER_DESC));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbSurface_Clear(
    IFramebufferSurface *This,
    FB_COLOR Color
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;
    return IFramebufferBackend_Clear(Surface->Backend, Color);
}

static HRESULT STDMETHODCALLTYPE
FbSurface_SetPixel(
    IFramebufferSurface *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;
    return IFramebufferBackend_SetPixel(Surface->Backend, X, Y, Color);
}

static HRESULT STDMETHODCALLTYPE
FbSurface_GetPixel(
    IFramebufferSurface *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;
    return IFramebufferBackend_GetPixel(Surface->Backend, X, Y, Color);
}

static HRESULT STDMETHODCALLTYPE
FbSurface_FillRect(
    IFramebufferSurface *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color,
    FB_ROP Rop
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;
    return FbEngineFillRect(Surface->Engine, Rect, Color, Rop);
}

static HRESULT STDMETHODCALLTYPE
FbSurface_BlitImage(
    IFramebufferSurface *This,
    INT32 DestX,
    INT32 DestY,
    IFramebufferImage *Image,
    CONST FB_RECT *SourceRect,
    FB_ROP Rop
    )
{
    /* TODO: Implement when IFramebufferImage is implemented */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
FbSurface_BlitToScreen(
    IFramebufferSurface *This,
    IFramebufferScreen *Screen,
    INT32 DestX,
    INT32 DestY,
    CONST FB_RECT *SourceRect,
    FB_ROP Rop
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;
    HRESULT Hr;
    VOID *SurfaceData;
    UINT32 SurfacePitch;
    UINT32 Width, Height;
    UINT32 SrcX, SrcY;

    if (Screen == NULL) {
        return E_POINTER;
    }

    /* Determine blit region */
    if (SourceRect != NULL) {
        SrcX = SourceRect->X;
        SrcY = SourceRect->Y;
        Width = SourceRect->Width;
        Height = SourceRect->Height;
    } else {
        SrcX = 0;
        SrcY = 0;
        Width = Surface->Descriptor.Width;
        Height = Surface->Descriptor.Height;
    }

    /* Lock surface to get data pointer */
    Hr = FbSurface_Lock(This, &SurfaceData, &SurfacePitch);
    if (FAILED(Hr)) {
        return Hr;
    }

    /* Blit to screen using engine */
    /* TODO: Get screen's backend and use engine to blit */
    /* For now, just use pixel-by-pixel copy */
    for (UINT32 Y = 0; Y < Height; Y++) {
        for (UINT32 X = 0; X < Width; X++) {
            FB_COLOR Color;
            Hr = FbSurface_GetPixel(This, SrcX + X, SrcY + Y, &Color);
            if (SUCCEEDED(Hr)) {
                IFramebufferScreen_SetPixel(Screen, DestX + X, DestY + Y, Color);
            }
        }
    }

    FbSurface_Unlock(This);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbSurface_GetPalette(
    IFramebufferSurface *This,
    IFramebufferPalette **Palette
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;

    if (Palette == NULL) {
        return E_POINTER;
    }

    /* Create palette manager on demand */
    if (Surface->Palette == NULL) {
        Surface->Palette = FbCreatePaletteManager();
        if (Surface->Palette == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    *Palette = Surface->Palette;
    IUnknown_AddRef((IUnknown *)Surface->Palette);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbSurface_GetText(
    IFramebufferSurface *This,
    IFramebufferText **Text
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;

    if (Text == NULL) {
        return E_POINTER;
    }

    /* Create text renderer on demand */
    if (Surface->Text == NULL) {
        Surface->Text = FbCreateTextRenderer(Surface->Backend);
        if (Surface->Text == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    *Text = Surface->Text;
    IUnknown_AddRef((IUnknown *)Surface->Text);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbSurface_Lock(
    IFramebufferSurface *This,
    VOID **MemoryAddress,
    UINT32 *Pitch
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;

    if (MemoryAddress == NULL || Pitch == NULL) {
        return E_POINTER;
    }

    /* Surfaces are always lockable */
    Surface->LockCount++;
    *MemoryAddress = Surface->Memory;
    *Pitch = Surface->Descriptor.Pitch;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbSurface_Unlock(
    IFramebufferSurface *This
    )
{
    FB_SURFACE_IMPL *Surface = (FB_SURFACE_IMPL *)This;

    if (Surface->LockCount == 0) {
        return E_FAIL;
    }

    Surface->LockCount--;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

/*
 * Create a software surface.
 */
IFramebufferSurface *
FbCreateSurface(
    IN UINT32 Width,
    IN UINT32 Height,
    IN FB_PIXEL_FORMAT Format
    )
{
    FB_SURFACE_IMPL *Surface;
    IFramebufferBackend *Backend;
    FRAMEBUFFER_DESC Descriptor;
    HRESULT Hr;

    if (Width == 0 || Height == 0) {
        return NULL;
    }

    /* Allocate surface object */
    Surface = (FB_SURFACE_IMPL *)ANX_MALLOC(sizeof(FB_SURFACE_IMPL));
    if (Surface == NULL) {
        return NULL;
    }

    /* Initialize */
    ANX_MEMSET(Surface, 0, sizeof(FB_SURFACE_IMPL));
    Surface->Base.lpVtbl = &gSurfaceVtbl;
    Surface->RefCount.RefCount = 1;
    Surface->IsLocked = FALSE;
    Surface->LockCount = 0;

    /* Calculate memory requirements */
    UINT32 BytesPerPixel = FbGetBytesPerPixel(Format);
    if (BytesPerPixel == 0) {
        BytesPerPixel = 1;  /* Packed formats */
    }

    UINT32 Pitch = Width * BytesPerPixel;
    /* Align to 4-byte boundary */
    if (Pitch % 4 != 0) {
        Pitch += 4 - (Pitch % 4);
    }

    Surface->MemorySize = Pitch * Height;

    /* Allocate memory */
    Surface->Memory = (UINT8 *)ANX_MALLOC(Surface->MemorySize);
    if (Surface->Memory == NULL) {
        ANX_FREE(Surface);
        return NULL;
    }

    ANX_MEMSET(Surface->Memory, 0, Surface->MemorySize);

    /* Set up descriptor */
    ANX_MEMSET(&Descriptor, 0, sizeof(FRAMEBUFFER_DESC));
    Descriptor.Width = Width;
    Descriptor.Height = Height;
    Descriptor.Pitch = Pitch;
    Descriptor.PixelFormat = Format;
    Descriptor.MemoryOrganization = FbMemoryLinear;
    Descriptor.PhysicalBase = (UINT64)(UINTN)Surface->Memory;
    Descriptor.Size = Surface->MemorySize;
    Descriptor.BitsPerPixel = FbGetBitsPerPixel(Format);
    Descriptor.IsAddressable = TRUE;

    ANX_MEMCPY(&Surface->Descriptor, &Descriptor, sizeof(FRAMEBUFFER_DESC));

    /* Create generic backend for software rendering */
    Backend = FbCreateBackend(FbBackendGeneric);
    if (Backend == NULL) {
        ANX_FREE(Surface->Memory);
        ANX_FREE(Surface);
        return NULL;
    }

    /* Initialize backend */
    Hr = IFramebufferBackend_Initialize(Backend, &Descriptor);
    if (FAILED(Hr)) {
        IUnknown_Release((IUnknown *)Backend);
        ANX_FREE(Surface->Memory);
        ANX_FREE(Surface);
        return NULL;
    }

    Surface->Backend = Backend;

    /* Create engine context */
    Surface->Engine = FbCreateEngine(Backend);
    if (Surface->Engine == NULL) {
        IUnknown_Release((IUnknown *)Backend);
        ANX_FREE(Surface->Memory);
        ANX_FREE(Surface);
        return NULL;
    }

    return &Surface->Base;
}
