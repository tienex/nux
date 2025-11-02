/*++
    Module Name:

        screen.c

    Abstract:

        IFramebufferScreen implementation.

        Represents a hardware framebuffer and provides the main interface
        for applications to access the display. Wraps the backend and engine
        layers to provide a complete high-level API.

--*/

#include <ananke/framebuffer/screen.h>
#include <ananke/framebuffer/engine.h>
#include <ananke/framebuffer/backends.h>
#include <ananke/framebuffer/cursor.h>
#include <ananke/framebuffer/palette.h>
#include <ananke/framebuffer/image.h>
#include <ananke/framebuffer/com_helpers.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Screen Implementation Structure                                */
/* --------------------------------------------------------------- */

typedef struct _FB_SCREEN_IMPL {
    IFramebufferScreen      Base;
    REFOBJ                  RefCount;
    IFramebufferBackend     *Backend;
    FB_ENGINE_CONTEXT       *Engine;
    FRAMEBUFFER_DESC        Descriptor;
    IFramebufferCursor      *Cursor;
    IFramebufferPalette     *Palette;
    IFramebufferText        *Text;
    BOOLEAN                 IsLocked;
    VOID                    *LockAddress;
    UINT32                  LockPitch;
} FB_SCREEN_IMPL;

/* --------------------------------------------------------------- */
/*  Forward Declarations                                            */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE FbScreen_QueryInterface(
    IFramebufferScreen *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE FbScreen_AddRef(IFramebufferScreen *This);
static UINT32 STDMETHODCALLTYPE FbScreen_Release(IFramebufferScreen *This);
static HRESULT STDMETHODCALLTYPE FbScreen_GetDescriptor(
    IFramebufferScreen *This, FRAMEBUFFER_DESC *Descriptor);
static HRESULT STDMETHODCALLTYPE FbScreen_GetMode(
    IFramebufferScreen *This, FB_MODE_DESC *Mode);
static HRESULT STDMETHODCALLTYPE FbScreen_Clear(
    IFramebufferScreen *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbScreen_SetPixel(
    IFramebufferScreen *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbScreen_GetPixel(
    IFramebufferScreen *This, INT32 X, INT32 Y, FB_COLOR *Color);
static HRESULT STDMETHODCALLTYPE FbScreen_FillRect(
    IFramebufferScreen *This, CONST FB_RECT *Rect, FB_COLOR Color, FB_ROP Rop);
static HRESULT STDMETHODCALLTYPE FbScreen_BlitImage(
    IFramebufferScreen *This, INT32 DestX, INT32 DestY,
    IFramebufferImage *Image, CONST FB_RECT *SourceRect, FB_ROP Rop);
static HRESULT STDMETHODCALLTYPE FbScreen_WaitForVBlank(
    IFramebufferScreen *This);
static HRESULT STDMETHODCALLTYPE FbScreen_GetActivePage(
    IFramebufferScreen *This, UINT32 *Page);
static HRESULT STDMETHODCALLTYPE FbScreen_SetActivePage(
    IFramebufferScreen *This, UINT32 Page);
static HRESULT STDMETHODCALLTYPE FbScreen_GetVisiblePage(
    IFramebufferScreen *This, UINT32 *Page);
static HRESULT STDMETHODCALLTYPE FbScreen_SetVisiblePage(
    IFramebufferScreen *This, UINT32 Page);
static HRESULT STDMETHODCALLTYPE FbScreen_FlipPages(
    IFramebufferScreen *This, BOOLEAN WaitForVBlank);
static HRESULT STDMETHODCALLTYPE FbScreen_GetCursor(
    IFramebufferScreen *This, IFramebufferCursor **Cursor);
static HRESULT STDMETHODCALLTYPE FbScreen_GetPalette(
    IFramebufferScreen *This, IFramebufferPalette **Palette);
static HRESULT STDMETHODCALLTYPE FbScreen_GetText(
    IFramebufferScreen *This, IFramebufferText **Text);
static HRESULT STDMETHODCALLTYPE FbScreen_Lock(
    IFramebufferScreen *This, VOID **FramebufferAddress, UINT32 *Pitch);
static HRESULT STDMETHODCALLTYPE FbScreen_Unlock(
    IFramebufferScreen *This);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferScreenVtbl gScreenVtbl = {
    .QueryInterface = FbScreen_QueryInterface,
    .AddRef         = FbScreen_AddRef,
    .Release        = FbScreen_Release,
    .GetDescriptor  = FbScreen_GetDescriptor,
    .GetMode        = FbScreen_GetMode,
    .Clear          = FbScreen_Clear,
    .SetPixel       = FbScreen_SetPixel,
    .GetPixel       = FbScreen_GetPixel,
    .FillRect       = FbScreen_FillRect,
    .BlitImage      = FbScreen_BlitImage,
    .WaitForVBlank  = FbScreen_WaitForVBlank,
    .GetActivePage  = FbScreen_GetActivePage,
    .SetActivePage  = FbScreen_SetActivePage,
    .GetVisiblePage = FbScreen_GetVisiblePage,
    .SetVisiblePage = FbScreen_SetVisiblePage,
    .FlipPages      = FbScreen_FlipPages,
    .GetCursor      = FbScreen_GetCursor,
    .GetPalette     = FbScreen_GetPalette,
    .GetText        = FbScreen_GetText,
    .Lock           = FbScreen_Lock,
    .Unlock         = FbScreen_Unlock,
};

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbScreen_QueryInterface(
    IFramebufferScreen *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (ANX_GUID_EQUALS(riid, &IID_IUnknown) ||
        ANX_GUID_EQUALS(riid, &IID_IFramebufferScreen)) {
        *ppvObject = &Screen->Base;
        IUnknown_AddRef((IUnknown *)&Screen->Base);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
FbScreen_AddRef(
    IFramebufferScreen *This
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;
    return ANX_REF_INC(&Screen->RefCount);
}

static UINT32 STDMETHODCALLTYPE
FbScreen_Release(
    IFramebufferScreen *This
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;
    UINT32 RefCount = ANX_REF_DEC(&Screen->RefCount);

    if (RefCount == 0) {
        /* Release child interfaces */
        if (Screen->Cursor != NULL) {
            IUnknown_Release((IUnknown *)Screen->Cursor);
        }
        if (Screen->Palette != NULL) {
            IUnknown_Release((IUnknown *)Screen->Palette);
        }
        if (Screen->Text != NULL) {
            IUnknown_Release((IUnknown *)Screen->Text);
        }

        /* Destroy engine */
        if (Screen->Engine != NULL) {
            FbDestroyEngine(Screen->Engine);
        }

        /* Release backend */
        if (Screen->Backend != NULL) {
            IUnknown_Release((IUnknown *)Screen->Backend);
        }

        /* Free screen object */
        ANX_FREE(Screen);
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferScreen Implementation                               */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbScreen_GetDescriptor(
    IFramebufferScreen *This,
    FRAMEBUFFER_DESC *Descriptor
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    ANX_MEMCPY(Descriptor, &Screen->Descriptor, sizeof(FRAMEBUFFER_DESC));
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_GetMode(
    IFramebufferScreen *This,
    FB_MODE_DESC *Mode
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;

    if (Mode == NULL) {
        return E_POINTER;
    }

    /* Convert descriptor to mode description */
    ANX_MEMSET(Mode, 0, sizeof(FB_MODE_DESC));
    Mode->Width = Screen->Descriptor.Width;
    Mode->Height = Screen->Descriptor.Height;
    Mode->BitsPerPixel = Screen->Descriptor.BitsPerPixel;
    Mode->RefreshRate = 60;  /* Default refresh rate */

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_Clear(
    IFramebufferScreen *This,
    FB_COLOR Color
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;
    return IFramebufferBackend_Clear(Screen->Backend, Color);
}

static HRESULT STDMETHODCALLTYPE
FbScreen_SetPixel(
    IFramebufferScreen *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;
    return IFramebufferBackend_SetPixel(Screen->Backend, X, Y, Color);
}

static HRESULT STDMETHODCALLTYPE
FbScreen_GetPixel(
    IFramebufferScreen *This,
    INT32 X,
    INT32 Y,
    FB_COLOR *Color
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;
    return IFramebufferBackend_GetPixel(Screen->Backend, X, Y, Color);
}

static HRESULT STDMETHODCALLTYPE
FbScreen_FillRect(
    IFramebufferScreen *This,
    CONST FB_RECT *Rect,
    FB_COLOR Color,
    FB_ROP Rop
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;
    return FbEngineFillRect(Screen->Engine, Rect, Color, Rop);
}

static HRESULT STDMETHODCALLTYPE
FbScreen_BlitImage(
    IFramebufferScreen *This,
    INT32 DestX,
    INT32 DestY,
    IFramebufferImage *Image,
    CONST FB_RECT *SourceRect,
    FB_ROP Rop
    )
{
    if (Image == NULL) {
        return E_POINTER;
    }

    /* Use image's BlitToScreen method */
    return IFramebufferImage_BlitToScreen(Image, This, DestX, DestY, SourceRect, Rop);
}

static HRESULT STDMETHODCALLTYPE
FbScreen_WaitForVBlank(
    IFramebufferScreen *This
    )
{
    /* Most backends don't support VBlank - return success */
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_GetActivePage(
    IFramebufferScreen *This,
    UINT32 *Page
    )
{
    if (Page == NULL) {
        return E_POINTER;
    }

    /* Single page by default */
    *Page = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_SetActivePage(
    IFramebufferScreen *This,
    UINT32 Page
    )
{
    /* Page flipping not supported by default */
    return (Page == 0) ? S_OK : E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_GetVisiblePage(
    IFramebufferScreen *This,
    UINT32 *Page
    )
{
    if (Page == NULL) {
        return E_POINTER;
    }

    /* Single page by default */
    *Page = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_SetVisiblePage(
    IFramebufferScreen *This,
    UINT32 Page
    )
{
    /* Page flipping not supported by default */
    return (Page == 0) ? S_OK : E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_FlipPages(
    IFramebufferScreen *This,
    BOOLEAN WaitForVBlank
    )
{
    /* Page flipping not supported by default */
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_GetCursor(
    IFramebufferScreen *This,
    IFramebufferCursor **Cursor
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;

    if (Cursor == NULL) {
        return E_POINTER;
    }

    /* Create cursor on demand */
    if (Screen->Cursor == NULL) {
        Screen->Cursor = FbCreateCursor(Screen->Backend);
        if (Screen->Cursor == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    *Cursor = Screen->Cursor;
    IUnknown_AddRef((IUnknown *)Screen->Cursor);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_GetPalette(
    IFramebufferScreen *This,
    IFramebufferPalette **Palette
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;

    if (Palette == NULL) {
        return E_POINTER;
    }

    /* Create palette manager on demand */
    if (Screen->Palette == NULL) {
        Screen->Palette = FbCreatePaletteManager();
        if (Screen->Palette == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    *Palette = Screen->Palette;
    IUnknown_AddRef((IUnknown *)Screen->Palette);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_GetText(
    IFramebufferScreen *This,
    IFramebufferText **Text
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;

    if (Text == NULL) {
        return E_POINTER;
    }

    /* Create text renderer on demand */
    if (Screen->Text == NULL) {
        Screen->Text = FbCreateTextRenderer(Screen->Backend);
        if (Screen->Text == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    *Text = Screen->Text;
    IUnknown_AddRef((IUnknown *)Screen->Text);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_Lock(
    IFramebufferScreen *This,
    VOID **FramebufferAddress,
    UINT32 *Pitch
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;
    HRESULT Hr;

    if (FramebufferAddress == NULL || Pitch == NULL) {
        return E_POINTER;
    }

    if (Screen->IsLocked) {
        return E_FAIL;
    }

    /* Try to lock the backend */
    Hr = IFramebufferBackend_GetDescriptor(Screen->Backend, &Screen->Descriptor);
    if (FAILED(Hr)) {
        return Hr;
    }

    /* Check if backend supports direct access */
    if (!Screen->Descriptor.IsAddressable) {
        return E_FAIL;
    }

    Screen->LockAddress = (VOID *)(UINTN)Screen->Descriptor.PhysicalBase;
    Screen->LockPitch = Screen->Descriptor.Pitch;
    Screen->IsLocked = TRUE;

    *FramebufferAddress = Screen->LockAddress;
    *Pitch = Screen->LockPitch;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbScreen_Unlock(
    IFramebufferScreen *This
    )
{
    FB_SCREEN_IMPL *Screen = (FB_SCREEN_IMPL *)This;

    if (!Screen->IsLocked) {
        return E_FAIL;
    }

    Screen->IsLocked = FALSE;
    Screen->LockAddress = NULL;
    Screen->LockPitch = 0;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

/*
 * Create a screen from a backend.
 */
IFramebufferScreen *
FbCreateScreen(
    IN IFramebufferBackend *Backend
    )
{
    FB_SCREEN_IMPL *Screen;
    HRESULT Hr;

    if (Backend == NULL) {
        return NULL;
    }

    /* Allocate screen object */
    Screen = (FB_SCREEN_IMPL *)ANX_MALLOC(sizeof(FB_SCREEN_IMPL));
    if (Screen == NULL) {
        return NULL;
    }

    /* Initialize */
    ANX_MEMSET(Screen, 0, sizeof(FB_SCREEN_IMPL));
    Screen->Base.lpVtbl = &gScreenVtbl;
    Screen->RefCount.RefCount = 1;
    Screen->Backend = Backend;
    Screen->IsLocked = FALSE;

    /* Add reference to backend */
    IUnknown_AddRef((IUnknown *)Backend);

    /* Get backend descriptor */
    Hr = IFramebufferBackend_GetDescriptor(Backend, &Screen->Descriptor);
    if (FAILED(Hr)) {
        IUnknown_Release((IUnknown *)&Screen->Base);
        return NULL;
    }

    /* Create engine context */
    Screen->Engine = FbCreateEngine(Backend);
    if (Screen->Engine == NULL) {
        IUnknown_Release((IUnknown *)&Screen->Base);
        return NULL;
    }

    return &Screen->Base;
}

/*
 * Create a screen from a backend type.
 */
IFramebufferScreen *
FbCreateScreenByType(
    IN FB_BACKEND_TYPE Type,
    IN CONST FRAMEBUFFER_DESC *Descriptor
    )
{
    IFramebufferBackend *Backend;
    IFramebufferScreen *Screen;
    HRESULT Hr;

    /* Create backend */
    Backend = FbCreateBackend(Type);
    if (Backend == NULL) {
        return NULL;
    }

    /* Initialize backend if descriptor provided */
    if (Descriptor != NULL) {
        Hr = IFramebufferBackend_Initialize(Backend, Descriptor);
        if (FAILED(Hr)) {
            IUnknown_Release((IUnknown *)Backend);
            return NULL;
        }
    }

    /* Create screen */
    Screen = FbCreateScreen(Backend);

    /* Release backend reference (screen now owns it) */
    IUnknown_Release((IUnknown *)Backend);

    return Screen;
}
