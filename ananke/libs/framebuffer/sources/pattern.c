/*++
    Module Name:

        pattern.c

    Abstract:

        Implementation of IFramebufferPattern for tiled drawing.
        Similar to Core Graphics CGPattern.

    Environment:

        C implementation.
--*/

#include <ananke/framebuffer/pattern.h>
#include <ananke/framebuffer/graphics2d.h>
#include <ananke/framebuffer/image.h>
#include <ananke/memory.h>

/* --------------------------------------------------------------- */
/*  IFramebufferPattern Implementation                             */
/* --------------------------------------------------------------- */

typedef struct _FB_PATTERN_IMPL {
    IFramebufferPattern     Base;
    REFOBJ                  RefCount;

    /* Pattern descriptor */
    FB_PATTERN_DESC         Descriptor;

    /* Cached image (for image-based patterns) */
    IFramebufferImage       *Image;
} FB_PATTERN_IMPL;

/* --------------------------------------------------------------- */
/*  IUnknown Methods                                               */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbPattern_QueryInterface(
    IFramebufferPattern *This,
    REFIID Riid,
    VOID **Object
    )
{
    if (AnxIsEqualGuid(Riid, &IID_IUnknown) ||
        AnxIsEqualGuid(Riid, &IID_IFramebufferPattern)) {
        IUnknown_AddRef((IUnknown *)This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
FbPattern_AddRef(
    IFramebufferPattern *This
    )
{
    FB_PATTERN_IMPL *Pattern = (FB_PATTERN_IMPL *)This;
    return AnxInterlockedIncrement(&Pattern->RefCount);
}

static ULONG STDMETHODCALLTYPE
FbPattern_Release(
    IFramebufferPattern *This
    )
{
    FB_PATTERN_IMPL *Pattern = (FB_PATTERN_IMPL *)This;
    ULONG RefCount = AnxInterlockedDecrement(&Pattern->RefCount);

    if (RefCount == 0) {
        if (Pattern->Image != NULL) {
            IUnknown_Release((IUnknown *)Pattern->Image);
        }
        AnxFree(Pattern);
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferPattern Methods                                    */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbPattern_GetDescriptor(
    IFramebufferPattern *This,
    FB_PATTERN_DESC *Descriptor
    )
{
    FB_PATTERN_IMPL *Pattern = (FB_PATTERN_IMPL *)This;

    if (Descriptor == NULL) {
        return E_POINTER;
    }

    *Descriptor = Pattern->Descriptor;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPattern_GetCellSize(
    IFramebufferPattern *This,
    UINT32 *Width,
    UINT32 *Height
    )
{
    FB_PATTERN_IMPL *Pattern = (FB_PATTERN_IMPL *)This;

    if (Width != NULL) {
        *Width = Pattern->Descriptor.Width;
    }
    if (Height != NULL) {
        *Height = Pattern->Descriptor.Height;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPattern_GetSpacing(
    IFramebufferPattern *This,
    FLOAT *XStep,
    FLOAT *YStep
    )
{
    FB_PATTERN_IMPL *Pattern = (FB_PATTERN_IMPL *)This;

    if (XStep != NULL) {
        *XStep = Pattern->Descriptor.XStep;
    }
    if (YStep != NULL) {
        *YStep = Pattern->Descriptor.YStep;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPattern_IsColored(
    IFramebufferPattern *This,
    BOOLEAN *Colored
    )
{
    FB_PATTERN_IMPL *Pattern = (FB_PATTERN_IMPL *)This;

    if (Colored == NULL) {
        return E_POINTER;
    }

    *Colored = Pattern->Descriptor.IsColored;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPattern_DrawCell(
    IFramebufferPattern *This,
    IFramebuffer2DContext *Context
    )
{
    FB_PATTERN_IMPL *Pattern = (FB_PATTERN_IMPL *)This;

    if (Context == NULL) {
        return E_POINTER;
    }

    /* If image-based pattern, draw the image */
    if (Pattern->Image != NULL) {
        return IFramebuffer2DContext_DrawImage(Context, Pattern->Image, 0.0f, 0.0f);
    }

    /* Otherwise call the draw callback */
    if (Pattern->Descriptor.DrawCallback != NULL) {
        Pattern->Descriptor.DrawCallback(Pattern->Descriptor.UserData, Context);
        return S_OK;
    }

    return E_FAIL;
}

/* --------------------------------------------------------------- */
/*  VTable                                                         */
/* --------------------------------------------------------------- */

static IFramebufferPatternVtbl FbPattern_Vtbl = {
    /* IUnknown */
    FbPattern_QueryInterface,
    FbPattern_AddRef,
    FbPattern_Release,

    /* IFramebufferPattern */
    FbPattern_GetDescriptor,
    FbPattern_GetCellSize,
    FbPattern_GetSpacing,
    FbPattern_IsColored,
    FbPattern_DrawCell,
};

/* --------------------------------------------------------------- */
/*  Factory Functions                                              */
/* --------------------------------------------------------------- */

IFramebufferPattern *
FbCreatePattern(
    IN CONST FB_PATTERN_DESC *Descriptor
    )
{
    if (Descriptor == NULL) {
        return NULL;
    }

    FB_PATTERN_IMPL *Pattern = (FB_PATTERN_IMPL *)AnxAllocate(sizeof(FB_PATTERN_IMPL));
    if (Pattern == NULL) {
        return NULL;
    }

    AnxZeroMemory(Pattern, sizeof(FB_PATTERN_IMPL));

    Pattern->Base.lpVtbl = &FbPattern_Vtbl;
    Pattern->RefCount = 1;
    Pattern->Descriptor = *Descriptor;
    Pattern->Image = NULL;

    return (IFramebufferPattern *)Pattern;
}

IFramebufferPattern *
FbCreatePatternFromImage(
    IN IFramebufferImage *Image,
    IN FLOAT XStep,
    IN FLOAT YStep
    )
{
    if (Image == NULL) {
        return NULL;
    }

    /* Get image dimensions */
    UINT32 Width, Height;
    HRESULT Hr = IFramebufferImage_GetDimensions(Image, &Width, &Height);
    if (FAILED(Hr)) {
        return NULL;
    }

    FB_PATTERN_IMPL *Pattern = (FB_PATTERN_IMPL *)AnxAllocate(sizeof(FB_PATTERN_IMPL));
    if (Pattern == NULL) {
        return NULL;
    }

    AnxZeroMemory(Pattern, sizeof(FB_PATTERN_IMPL));

    Pattern->Base.lpVtbl = &FbPattern_Vtbl;
    Pattern->RefCount = 1;
    Pattern->Descriptor.Width = Width;
    Pattern->Descriptor.Height = Height;
    Pattern->Descriptor.XStep = XStep;
    Pattern->Descriptor.YStep = YStep;
    Pattern->Descriptor.Tiling = FbPatternTilingConstantSpacing;
    Pattern->Descriptor.IsColored = TRUE;
    Pattern->Descriptor.DrawCallback = NULL;
    Pattern->Descriptor.UserData = NULL;

    /* Store image reference */
    IUnknown_AddRef((IUnknown *)Image);
    Pattern->Image = Image;

    return (IFramebufferPattern *)Pattern;
}
