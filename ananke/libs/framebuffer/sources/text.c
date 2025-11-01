/*++
    Module Name:

        text.c

    Abstract:

        Text rendering implementation with Unicode and BIDI support.

--*/

#include <ananke/framebuffer.h>
#include <ananke/framebuffer/font.h>
#include <ananke/framebuffer/bidi.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Text Renderer Structure                                         */
/* --------------------------------------------------------------- */

typedef struct _FB_TEXT_RENDERER {
    IFramebufferText            Base;
    REFOBJ                      RefCount;
    IFramebufferBackend         *Backend;
    CONST BITMAP_FONT           *Font;
} FB_TEXT_RENDERER;

/* Forward declarations */
static HRESULT STDMETHODCALLTYPE FbText_QueryInterface(
    IFramebufferText *This,
    REFIID riid,
    VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE FbText_AddRef(
    IFramebufferText *This);
static UINT32 STDMETHODCALLTYPE FbText_Release(
    IFramebufferText *This);
static HRESULT STDMETHODCALLTYPE FbText_DrawChar(
    IFramebufferText *This,
    INT32 X,
    INT32 Y,
    CHAR16 Character,
    FB_COLOR Foreground,
    FB_COLOR Background);
static HRESULT STDMETHODCALLTYPE FbText_DrawString(
    IFramebufferText *This,
    INT32 X,
    INT32 Y,
    CONST CHAR16 *String,
    UINTN Length,
    FB_COLOR Foreground,
    FB_COLOR Background,
    FB_TEXT_DIRECTION Direction);
static HRESULT STDMETHODCALLTYPE FbText_MeasureString(
    IFramebufferText *This,
    CONST CHAR16 *String,
    UINTN Length,
    UINT32 *Width,
    UINT32 *Height);
static HRESULT STDMETHODCALLTYPE FbText_GetFontMetrics(
    IFramebufferText *This,
    UINT32 *CharWidth,
    UINT32 *CharHeight);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferTextVtbl gFbTextVtbl = {
    .QueryInterface     = FbText_QueryInterface,
    .AddRef             = FbText_AddRef,
    .Release            = FbText_Release,
    .DrawChar           = FbText_DrawChar,
    .DrawString         = FbText_DrawString,
    .MeasureString      = FbText_MeasureString,
    .GetFontMetrics     = FbText_GetFontMetrics,
};

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbText_QueryInterface(
    IFramebufferText *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    FB_TEXT_RENDERER *Renderer = (FB_TEXT_RENDERER *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IFramebufferText)) {
        *ppvObject = &Renderer->Base;
        FbText_AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
FbText_AddRef(
    IFramebufferText *This
    )
{
    FB_TEXT_RENDERER *Renderer = (FB_TEXT_RENDERER *)This;
    return ANX_REF_INC(&Renderer->RefCount);
}

static UINT32 STDMETHODCALLTYPE
FbText_Release(
    IFramebufferText *This
    )
{
    FB_TEXT_RENDERER *Renderer = (FB_TEXT_RENDERER *)This;
    UINT32 RefCount = ANX_REF_DEC(&Renderer->RefCount);

    if (RefCount == 0) {
        /* Release backend reference */
        if (Renderer->Backend != NULL) {
            IUnknown_Release((IUnknown *)Renderer->Backend);
        }
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferText Implementation                                 */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbText_DrawChar(
    IFramebufferText *This,
    INT32 X,
    INT32 Y,
    CHAR16 Character,
    FB_COLOR Foreground,
    FB_COLOR Background
    )
{
    FB_TEXT_RENDERER *Renderer = (FB_TEXT_RENDERER *)This;
    CONST UINT8 *Glyph;
    UINT32 GlyphStride;

    if (Renderer->Backend == NULL || Renderer->Font == NULL) {
        return E_FAIL;
    }

    /* Get glyph bitmap */
    Glyph = FbGetGlyph(Renderer->Font, Character);
    if (Glyph == NULL) {
        return E_FAIL;
    }

    GlyphStride = FbGetGlyphStride(Renderer->Font);

    /* Use backend to blit the glyph */
    return IFramebufferBackend_BlitMonoBitmap(
        Renderer->Backend,
        X, Y,
        Renderer->Font->Width,
        Renderer->Font->Height,
        Glyph,
        Foreground,
        Background
    );
}

static HRESULT STDMETHODCALLTYPE
FbText_DrawString(
    IFramebufferText *This,
    INT32 X,
    INT32 Y,
    CONST CHAR16 *String,
    UINTN Length,
    FB_COLOR Foreground,
    FB_COLOR Background,
    FB_TEXT_DIRECTION Direction
    )
{
    FB_TEXT_RENDERER *Renderer = (FB_TEXT_RENDERER *)This;
    CHAR16 ReorderedBuffer[256];
    CONST CHAR16 *DisplayString;
    UINTN DisplayLength;
    UINTN i;
    INT32 CurrentX;
    HRESULT Hr;

    if (String == NULL || Renderer->Backend == NULL || Renderer->Font == NULL) {
        return E_POINTER;
    }

    /* Apply BIDI reordering if needed */
    if (Direction == FbTextDirectionRTL && Length < 256) {
        DisplayLength = FbReorderBidiString(
            String,
            Length,
            ReorderedBuffer,
            BidiDirectionRTL
        );
        DisplayString = ReorderedBuffer;
    } else {
        DisplayString = String;
        DisplayLength = Length;
    }

    /* Draw each character */
    CurrentX = X;
    for (i = 0; i < DisplayLength; i++) {
        Hr = FbText_DrawChar(
            This,
            CurrentX,
            Y,
            DisplayString[i],
            Foreground,
            Background
        );

        if (FAILED(Hr)) {
            return Hr;
        }

        CurrentX += Renderer->Font->Width;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbText_MeasureString(
    IFramebufferText *This,
    CONST CHAR16 *String,
    UINTN Length,
    UINT32 *Width,
    UINT32 *Height
    )
{
    FB_TEXT_RENDERER *Renderer = (FB_TEXT_RENDERER *)This;

    if (String == NULL || Width == NULL || Height == NULL) {
        return E_POINTER;
    }

    if (Renderer->Font == NULL) {
        return E_FAIL;
    }

    *Width = (UINT32)(Length * Renderer->Font->Width);
    *Height = Renderer->Font->Height;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbText_GetFontMetrics(
    IFramebufferText *This,
    UINT32 *CharWidth,
    UINT32 *CharHeight
    )
{
    FB_TEXT_RENDERER *Renderer = (FB_TEXT_RENDERER *)This;

    if (CharWidth == NULL || CharHeight == NULL) {
        return E_POINTER;
    }

    if (Renderer->Font == NULL) {
        return E_FAIL;
    }

    *CharWidth = Renderer->Font->Width;
    *CharHeight = Renderer->Font->Height;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Public Constructor                                              */
/* --------------------------------------------------------------- */

static FB_TEXT_RENDERER gTextRendererInstance = {
    .Base.lpVtbl        = &gFbTextVtbl,
    .RefCount.RefCount  = 1,
    .Backend            = NULL,
    .Font               = &FB_FONT_DEFAULT,
};

IFramebufferText *
FbCreateTextRenderer(
    IN IFramebufferBackend *Backend
    )
{
    if (Backend == NULL) {
        return NULL;
    }

    gTextRendererInstance.Backend = Backend;
    IUnknown_AddRef((IUnknown *)Backend);

    return (IFramebufferText *)&gTextRendererInstance;
}
