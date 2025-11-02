/*++
    Module Name:

        gradient.c

    Abstract:

        Implementation of IFramebufferGradient and IFramebufferShading.
        Similar to Core Graphics CGGradient and CGShading.

    Environment:

        C implementation.
--*/

#include <ananke/framebuffer/gradient.h>
#include <ananke/memory.h>
#include <ananke/math.h>

/* --------------------------------------------------------------- */
/*  IFramebufferGradient Implementation                            */
/* --------------------------------------------------------------- */

typedef struct _FB_GRADIENT_IMPL {
    IFramebufferGradient    Base;
    REFOBJ                  RefCount;

    /* Gradient stops */
    FB_GRADIENT_STOP        *Stops;
    UINT32                  StopCount;
} FB_GRADIENT_IMPL;

/* --------------------------------------------------------------- */
/*  IUnknown Methods                                               */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbGradient_QueryInterface(
    IFramebufferGradient *This,
    REFIID Riid,
    VOID **Object
    )
{
    if (AnxIsEqualGuid(Riid, &IID_IUnknown) ||
        AnxIsEqualGuid(Riid, &IID_IFramebufferGradient)) {
        IUnknown_AddRef((IUnknown *)This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
FbGradient_AddRef(
    IFramebufferGradient *This
    )
{
    FB_GRADIENT_IMPL *Gradient = (FB_GRADIENT_IMPL *)This;
    return AnxInterlockedIncrement(&Gradient->RefCount);
}

static ULONG STDMETHODCALLTYPE
FbGradient_Release(
    IFramebufferGradient *This
    )
{
    FB_GRADIENT_IMPL *Gradient = (FB_GRADIENT_IMPL *)This;
    ULONG RefCount = AnxInterlockedDecrement(&Gradient->RefCount);

    if (RefCount == 0) {
        if (Gradient->Stops != NULL) {
            AnxFree(Gradient->Stops);
        }
        AnxFree(Gradient);
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferGradient Methods                                   */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbGradient_GetStopCount(
    IFramebufferGradient *This,
    UINT32 *Count
    )
{
    FB_GRADIENT_IMPL *Gradient = (FB_GRADIENT_IMPL *)This;

    if (Count == NULL) {
        return E_POINTER;
    }

    *Count = Gradient->StopCount;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGradient_GetColorAtLocation(
    IFramebufferGradient *This,
    FLOAT Location,
    FB_COLOR *Color
    )
{
    FB_GRADIENT_IMPL *Gradient = (FB_GRADIENT_IMPL *)This;

    if (Color == NULL) {
        return E_POINTER;
    }

    if (Gradient->StopCount == 0) {
        *Color = 0;
        return S_FALSE;
    }

    /* Clamp location to [0, 1] */
    if (Location <= 0.0f) {
        *Color = Gradient->Stops[0].Color;
        return S_OK;
    }
    if (Location >= 1.0f) {
        *Color = Gradient->Stops[Gradient->StopCount - 1].Color;
        return S_OK;
    }

    /* Find the two stops to interpolate between */
    for (UINT32 I = 0; I < Gradient->StopCount - 1; I++) {
        FB_GRADIENT_STOP *Stop1 = &Gradient->Stops[I];
        FB_GRADIENT_STOP *Stop2 = &Gradient->Stops[I + 1];

        if (Location >= Stop1->Offset && Location <= Stop2->Offset) {
            /* Interpolate between Stop1 and Stop2 */
            FLOAT T = (Location - Stop1->Offset) / (Stop2->Offset - Stop1->Offset);

            UINT8 R1 = (Stop1->Color >> 16) & 0xFF;
            UINT8 G1 = (Stop1->Color >> 8) & 0xFF;
            UINT8 B1 = Stop1->Color & 0xFF;
            UINT8 A1 = (Stop1->Color >> 24) & 0xFF;

            UINT8 R2 = (Stop2->Color >> 16) & 0xFF;
            UINT8 G2 = (Stop2->Color >> 8) & 0xFF;
            UINT8 B2 = Stop2->Color & 0xFF;
            UINT8 A2 = (Stop2->Color >> 24) & 0xFF;

            UINT8 R = (UINT8)(R1 + T * (R2 - R1));
            UINT8 G = (UINT8)(G1 + T * (G2 - G1));
            UINT8 B = (UINT8)(B1 + T * (B2 - B1));
            UINT8 A = (UINT8)(A1 + T * (A2 - A1));

            *Color = (A << 24) | (R << 16) | (G << 8) | B;
            return S_OK;
        }
    }

    /* Shouldn't reach here, but return last color as fallback */
    *Color = Gradient->Stops[Gradient->StopCount - 1].Color;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGradient_GetStops(
    IFramebufferGradient *This,
    FB_GRADIENT_STOP *Stops,
    UINT32 MaxStops,
    UINT32 *StopCount
    )
{
    FB_GRADIENT_IMPL *Gradient = (FB_GRADIENT_IMPL *)This;

    if (StopCount != NULL) {
        *StopCount = Gradient->StopCount;
    }

    if (Stops == NULL) {
        return S_OK;
    }

    if (MaxStops < Gradient->StopCount) {
        return E_OUTOFMEMORY;
    }

    for (UINT32 I = 0; I < Gradient->StopCount; I++) {
        Stops[I] = Gradient->Stops[I];
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  VTable                                                         */
/* --------------------------------------------------------------- */

static IFramebufferGradientVtbl FbGradient_Vtbl = {
    /* IUnknown */
    FbGradient_QueryInterface,
    FbGradient_AddRef,
    FbGradient_Release,

    /* IFramebufferGradient */
    FbGradient_GetStopCount,
    FbGradient_GetColorAtLocation,
    FbGradient_GetStops,
};

/* --------------------------------------------------------------- */
/*  Factory Function                                               */
/* --------------------------------------------------------------- */

IFramebufferGradient *
FbCreateGradient(
    IN CONST FB_COLOR *Colors,
    IN CONST FLOAT *Locations,
    IN UINT32 Count
    )
{
    if (Colors == NULL || Count == 0) {
        return NULL;
    }

    FB_GRADIENT_IMPL *Gradient = (FB_GRADIENT_IMPL *)AnxAllocate(sizeof(FB_GRADIENT_IMPL));
    if (Gradient == NULL) {
        return NULL;
    }

    AnxZeroMemory(Gradient, sizeof(FB_GRADIENT_IMPL));

    Gradient->Base.lpVtbl = &FbGradient_Vtbl;
    Gradient->RefCount = 1;
    Gradient->StopCount = Count;

    /* Allocate stops */
    Gradient->Stops = (FB_GRADIENT_STOP *)AnxAllocate(Count * sizeof(FB_GRADIENT_STOP));
    if (Gradient->Stops == NULL) {
        AnxFree(Gradient);
        return NULL;
    }

    /* Fill in stops */
    for (UINT32 I = 0; I < Count; I++) {
        Gradient->Stops[I].Color = Colors[I];

        if (Locations != NULL) {
            Gradient->Stops[I].Offset = Locations[I];
        } else {
            /* Evenly spaced if no locations provided */
            Gradient->Stops[I].Offset = (FLOAT)I / (FLOAT)(Count - 1);
        }
    }

    /* Sort stops by offset (bubble sort - fine for small arrays) */
    for (UINT32 I = 0; I < Count - 1; I++) {
        for (UINT32 J = I + 1; J < Count; J++) {
            if (Gradient->Stops[J].Offset < Gradient->Stops[I].Offset) {
                FB_GRADIENT_STOP Temp = Gradient->Stops[I];
                Gradient->Stops[I] = Gradient->Stops[J];
                Gradient->Stops[J] = Temp;
            }
        }
    }

    return (IFramebufferGradient *)Gradient;
}

/* --------------------------------------------------------------- */
/*  IFramebufferShading Implementation                             */
/* --------------------------------------------------------------- */

typedef enum _FB_SHADING_TYPE {
    FbShadingAxial,
    FbShadingRadial,
    FbShadingFunction,
} FB_SHADING_TYPE;

typedef struct _FB_SHADING_IMPL {
    IFramebufferShading     Base;
    REFOBJ                  RefCount;

    /* Shading type and callback */
    FB_SHADING_TYPE         Type;
    FB_SHADING_CALLBACK     Callback;
    VOID                    *UserData;

    /* Axial shading parameters */
    FLOAT                   AxialStartX, AxialStartY;
    FLOAT                   AxialEndX, AxialEndY;
    BOOLEAN                 AxialExtendStart;
    BOOLEAN                 AxialExtendEnd;

    /* Radial shading parameters */
    FLOAT                   RadialStartX, RadialStartY, RadialStartRadius;
    FLOAT                   RadialEndX, RadialEndY, RadialEndRadius;
    BOOLEAN                 RadialExtendStart;
    BOOLEAN                 RadialExtendEnd;
} FB_SHADING_IMPL;

/* --------------------------------------------------------------- */
/*  IUnknown Methods                                               */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbShading_QueryInterface(
    IFramebufferShading *This,
    REFIID Riid,
    VOID **Object
    )
{
    if (AnxIsEqualGuid(Riid, &IID_IUnknown) ||
        AnxIsEqualGuid(Riid, &IID_IFramebufferShading)) {
        IUnknown_AddRef((IUnknown *)This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
FbShading_AddRef(
    IFramebufferShading *This
    )
{
    FB_SHADING_IMPL *Shading = (FB_SHADING_IMPL *)This;
    return AnxInterlockedIncrement(&Shading->RefCount);
}

static ULONG STDMETHODCALLTYPE
FbShading_Release(
    IFramebufferShading *This
    )
{
    FB_SHADING_IMPL *Shading = (FB_SHADING_IMPL *)This;
    ULONG RefCount = AnxInterlockedDecrement(&Shading->RefCount);

    if (RefCount == 0) {
        AnxFree(Shading);
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  IFramebufferShading Methods                                    */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbShading_Evaluate(
    IFramebufferShading *This,
    CONST FLOAT *Position,
    FB_COLOR *Color
    )
{
    FB_SHADING_IMPL *Shading = (FB_SHADING_IMPL *)This;

    if (Position == NULL || Color == NULL) {
        return E_POINTER;
    }

    if (Shading->Callback == NULL) {
        return E_FAIL;
    }

    /* Call user callback */
    Shading->Callback(Shading->UserData, Position, Color);

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  VTable                                                         */
/* --------------------------------------------------------------- */

static IFramebufferShadingVtbl FbShading_Vtbl = {
    /* IUnknown */
    FbShading_QueryInterface,
    FbShading_AddRef,
    FbShading_Release,

    /* IFramebufferShading */
    FbShading_Evaluate,
};

/* --------------------------------------------------------------- */
/*  Factory Functions                                              */
/* --------------------------------------------------------------- */

IFramebufferShading *
FbCreateAxialShading(
    IN FB_SHADING_CALLBACK Callback,
    IN VOID *UserData,
    IN FLOAT StartX,
    IN FLOAT StartY,
    IN FLOAT EndX,
    IN FLOAT EndY,
    IN BOOLEAN ExtendStart,
    IN BOOLEAN ExtendEnd
    )
{
    if (Callback == NULL) {
        return NULL;
    }

    FB_SHADING_IMPL *Shading = (FB_SHADING_IMPL *)AnxAllocate(sizeof(FB_SHADING_IMPL));
    if (Shading == NULL) {
        return NULL;
    }

    AnxZeroMemory(Shading, sizeof(FB_SHADING_IMPL));

    Shading->Base.lpVtbl = &FbShading_Vtbl;
    Shading->RefCount = 1;
    Shading->Type = FbShadingAxial;
    Shading->Callback = Callback;
    Shading->UserData = UserData;
    Shading->AxialStartX = StartX;
    Shading->AxialStartY = StartY;
    Shading->AxialEndX = EndX;
    Shading->AxialEndY = EndY;
    Shading->AxialExtendStart = ExtendStart;
    Shading->AxialExtendEnd = ExtendEnd;

    return (IFramebufferShading *)Shading;
}

IFramebufferShading *
FbCreateRadialShading(
    IN FB_SHADING_CALLBACK Callback,
    IN VOID *UserData,
    IN FLOAT StartX,
    IN FLOAT StartY,
    IN FLOAT StartRadius,
    IN FLOAT EndX,
    IN FLOAT EndY,
    IN FLOAT EndRadius,
    IN BOOLEAN ExtendStart,
    IN BOOLEAN ExtendEnd
    )
{
    if (Callback == NULL) {
        return NULL;
    }

    FB_SHADING_IMPL *Shading = (FB_SHADING_IMPL *)AnxAllocate(sizeof(FB_SHADING_IMPL));
    if (Shading == NULL) {
        return NULL;
    }

    AnxZeroMemory(Shading, sizeof(FB_SHADING_IMPL));

    Shading->Base.lpVtbl = &FbShading_Vtbl;
    Shading->RefCount = 1;
    Shading->Type = FbShadingRadial;
    Shading->Callback = Callback;
    Shading->UserData = UserData;
    Shading->RadialStartX = StartX;
    Shading->RadialStartY = StartY;
    Shading->RadialStartRadius = StartRadius;
    Shading->RadialEndX = EndX;
    Shading->RadialEndY = EndY;
    Shading->RadialEndRadius = EndRadius;
    Shading->RadialExtendStart = ExtendStart;
    Shading->RadialExtendEnd = ExtendEnd;

    return (IFramebufferShading *)Shading;
}

IFramebufferShading *
FbCreateFunctionShading(
    IN FB_SHADING_CALLBACK Callback,
    IN VOID *UserData
    )
{
    if (Callback == NULL) {
        return NULL;
    }

    FB_SHADING_IMPL *Shading = (FB_SHADING_IMPL *)AnxAllocate(sizeof(FB_SHADING_IMPL));
    if (Shading == NULL) {
        return NULL;
    }

    AnxZeroMemory(Shading, sizeof(FB_SHADING_IMPL));

    Shading->Base.lpVtbl = &FbShading_Vtbl;
    Shading->RefCount = 1;
    Shading->Type = FbShadingFunction;
    Shading->Callback = Callback;
    Shading->UserData = UserData;

    return (IFramebufferShading *)Shading;
}
