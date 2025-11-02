/*++
    Module Name:

        graphics2d.c

    Abstract:

        IFramebuffer2DContext implementation.

        Provides high-level 2D drawing API with state management,
        transforms, primitives, paths, and text rendering.

--*/

#include <ananke/framebuffer/graphics2d.h>
#include <ananke/framebuffer/screen.h>
#include <ananke/framebuffer/image.h>
#include <ananke/framebuffer/font.h>
#include <ananke/framebuffer/com_helpers.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>
#include <ananke/math.h>

/* --------------------------------------------------------------- */
/*  Constants                                                       */
/* --------------------------------------------------------------- */

#define FB_GFX_MAX_STATE_STACK  32
#define FB_GFX_MAX_PATH_POINTS  8192

/* --------------------------------------------------------------- */
/*  Graphics State                                                  */
/* --------------------------------------------------------------- */

typedef struct _FB_GFX_STATE {
    /* Colors */
    FB_COLOR        StrokeColor;
    FB_COLOR        FillColor;

    /* Line style */
    FLOAT           LineWidth;
    FB_LINE_CAP     LineCap;
    FB_LINE_JOIN    LineJoin;
    FLOAT           MiterLimit;

    /* Blending */
    FLOAT           GlobalAlpha;
    FB_COMPOSITE_OP CompositeOp;
    BOOLEAN         AntiAlias;

    /* Transform */
    FB_TRANSFORM    Transform;

    /* Clipping */
    FB_RECT         ClipRect;
    BOOLEAN         HasClip;

    /* Font */
    IFramebufferFont *Font;
} FB_GFX_STATE;

/* Path point */
typedef struct _FB_PATH_POINT {
    FLOAT               X, Y;
    FB_PATH_COMMAND     Command;
} FB_PATH_POINT;

/* --------------------------------------------------------------- */
/*  Graphics Context Implementation                                 */
/* --------------------------------------------------------------- */

typedef struct _FB_GFX_CONTEXT_IMPL {
    IFramebuffer2DContext   Base;
    REFOBJ                  RefCount;

    /* Target surface */
    IFramebufferSurface     *Surface;
    BOOLEAN                 OwnsSurface;  /* If created from screen */

    /* Current state */
    FB_GFX_STATE            State;

    /* State stack */
    FB_GFX_STATE            StateStack[FB_GFX_MAX_STATE_STACK];
    UINT32                  StateStackDepth;

    /* Current path */
    FB_PATH_POINT           *PathPoints;
    UINT32                  PathPointCount;
    UINT32                  PathPointCapacity;
    FLOAT                   CurrentX, CurrentY;  /* Current path position */
} FB_GFX_CONTEXT_IMPL;

/* --------------------------------------------------------------- */
/*  Forward Declarations                                            */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE FbGfx_QueryInterface(
    IFramebuffer2DContext *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE FbGfx_AddRef(IFramebuffer2DContext *This);
static UINT32 STDMETHODCALLTYPE FbGfx_Release(IFramebuffer2DContext *This);

/* State management */
static HRESULT STDMETHODCALLTYPE FbGfx_Save(IFramebuffer2DContext *This);
static HRESULT STDMETHODCALLTYPE FbGfx_Restore(IFramebuffer2DContext *This);
static HRESULT STDMETHODCALLTYPE FbGfx_Reset(IFramebuffer2DContext *This);

/* Stroke and fill state */
static HRESULT STDMETHODCALLTYPE FbGfx_SetStrokeColor(IFramebuffer2DContext *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbGfx_SetFillColor(IFramebuffer2DContext *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbGfx_SetLineWidth(IFramebuffer2DContext *This, FLOAT Width);
static HRESULT STDMETHODCALLTYPE FbGfx_SetLineCap(IFramebuffer2DContext *This, FB_LINE_CAP Cap);
static HRESULT STDMETHODCALLTYPE FbGfx_SetLineJoin(IFramebuffer2DContext *This, FB_LINE_JOIN Join);
static HRESULT STDMETHODCALLTYPE FbGfx_SetMiterLimit(IFramebuffer2DContext *This, FLOAT Limit);
static HRESULT STDMETHODCALLTYPE FbGfx_SetGlobalAlpha(IFramebuffer2DContext *This, FLOAT Alpha);
static HRESULT STDMETHODCALLTYPE FbGfx_SetCompositeOp(IFramebuffer2DContext *This, FB_COMPOSITE_OP Op);
static HRESULT STDMETHODCALLTYPE FbGfx_SetAntiAlias(IFramebuffer2DContext *This, BOOLEAN Enabled);

/* Transform */
static HRESULT STDMETHODCALLTYPE FbGfx_SetTransform(IFramebuffer2DContext *This, CONST FB_TRANSFORM *Transform);
static HRESULT STDMETHODCALLTYPE FbGfx_GetTransform(IFramebuffer2DContext *This, FB_TRANSFORM *Transform);
static HRESULT STDMETHODCALLTYPE FbGfx_ResetTransform(IFramebuffer2DContext *This);
static HRESULT STDMETHODCALLTYPE FbGfx_Translate(IFramebuffer2DContext *This, FLOAT X, FLOAT Y);
static HRESULT STDMETHODCALLTYPE FbGfx_Scale(IFramebuffer2DContext *This, FLOAT ScaleX, FLOAT ScaleY);
static HRESULT STDMETHODCALLTYPE FbGfx_Rotate(IFramebuffer2DContext *This, FLOAT Angle);

/* Clipping */
static HRESULT STDMETHODCALLTYPE FbGfx_SetClipRect(IFramebuffer2DContext *This, CONST FB_RECT *Rect);
static HRESULT STDMETHODCALLTYPE FbGfx_IntersectClipRect(IFramebuffer2DContext *This, CONST FB_RECT *Rect);
static HRESULT STDMETHODCALLTYPE FbGfx_ResetClip(IFramebuffer2DContext *This);

/* Primitives */
static HRESULT STDMETHODCALLTYPE FbGfx_Clear(IFramebuffer2DContext *This, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbGfx_DrawPixel(IFramebuffer2DContext *This, INT32 X, INT32 Y, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbGfx_DrawLine(IFramebuffer2DContext *This, FLOAT X0, FLOAT Y0, FLOAT X1, FLOAT Y1);
static HRESULT STDMETHODCALLTYPE FbGfx_StrokeRect(IFramebuffer2DContext *This, FLOAT X, FLOAT Y, FLOAT Width, FLOAT Height);
static HRESULT STDMETHODCALLTYPE FbGfx_FillRect(IFramebuffer2DContext *This, FLOAT X, FLOAT Y, FLOAT Width, FLOAT Height);
static HRESULT STDMETHODCALLTYPE FbGfx_StrokeCircle(IFramebuffer2DContext *This, FLOAT CenterX, FLOAT CenterY, FLOAT Radius);
static HRESULT STDMETHODCALLTYPE FbGfx_FillCircle(IFramebuffer2DContext *This, FLOAT CenterX, FLOAT CenterY, FLOAT Radius);
static HRESULT STDMETHODCALLTYPE FbGfx_StrokeEllipse(IFramebuffer2DContext *This, FLOAT CenterX, FLOAT CenterY, FLOAT RadiusX, FLOAT RadiusY);
static HRESULT STDMETHODCALLTYPE FbGfx_FillEllipse(IFramebuffer2DContext *This, FLOAT CenterX, FLOAT CenterY, FLOAT RadiusX, FLOAT RadiusY);
static HRESULT STDMETHODCALLTYPE FbGfx_StrokeArc(IFramebuffer2DContext *This, FLOAT CenterX, FLOAT CenterY, FLOAT Radius, FLOAT StartAngle, FLOAT EndAngle);

/* Path */
static HRESULT STDMETHODCALLTYPE FbGfx_BeginPath(IFramebuffer2DContext *This);
static HRESULT STDMETHODCALLTYPE FbGfx_ClosePath(IFramebuffer2DContext *This);
static HRESULT STDMETHODCALLTYPE FbGfx_MoveTo(IFramebuffer2DContext *This, FLOAT X, FLOAT Y);
static HRESULT STDMETHODCALLTYPE FbGfx_LineTo(IFramebuffer2DContext *This, FLOAT X, FLOAT Y);
static HRESULT STDMETHODCALLTYPE FbGfx_QuadraticCurveTo(IFramebuffer2DContext *This, FLOAT ControlX, FLOAT ControlY, FLOAT X, FLOAT Y);
static HRESULT STDMETHODCALLTYPE FbGfx_BezierCurveTo(IFramebuffer2DContext *This, FLOAT Control1X, FLOAT Control1Y, FLOAT Control2X, FLOAT Control2Y, FLOAT X, FLOAT Y);
static HRESULT STDMETHODCALLTYPE FbGfx_Arc(IFramebuffer2DContext *This, FLOAT CenterX, FLOAT CenterY, FLOAT Radius, FLOAT StartAngle, FLOAT EndAngle, BOOLEAN CounterClockwise);
static HRESULT STDMETHODCALLTYPE FbGfx_Stroke(IFramebuffer2DContext *This);
static HRESULT STDMETHODCALLTYPE FbGfx_Fill(IFramebuffer2DContext *This, FB_FILL_RULE FillRule);

/* Polygon */
static HRESULT STDMETHODCALLTYPE FbGfx_StrokePolygon(IFramebuffer2DContext *This, CONST FB_POINT *Points, UINT32 PointCount);
static HRESULT STDMETHODCALLTYPE FbGfx_FillPolygon(IFramebuffer2DContext *This, CONST FB_POINT *Points, UINT32 PointCount, FB_FILL_RULE FillRule);

/* Image */
static HRESULT STDMETHODCALLTYPE FbGfx_DrawImage(IFramebuffer2DContext *This, IFramebufferImage *Image, FLOAT X, FLOAT Y);
static HRESULT STDMETHODCALLTYPE FbGfx_DrawImageScaled(IFramebuffer2DContext *This, IFramebufferImage *Image, FLOAT X, FLOAT Y, FLOAT Width, FLOAT Height);
static HRESULT STDMETHODCALLTYPE FbGfx_DrawImageEx(IFramebuffer2DContext *This, IFramebufferImage *Image, CONST FB_RECT *SourceRect, CONST FB_RECT *DestRect);

/* Text */
static HRESULT STDMETHODCALLTYPE FbGfx_SetFont(IFramebuffer2DContext *This, IFramebufferFont *Font);
static HRESULT STDMETHODCALLTYPE FbGfx_GetFont(IFramebuffer2DContext *This, IFramebufferFont **Font);
static HRESULT STDMETHODCALLTYPE FbGfx_DrawText(IFramebuffer2DContext *This, CONST CHAR *Text, FLOAT X, FLOAT Y);
static HRESULT STDMETHODCALLTYPE FbGfx_MeasureText(IFramebuffer2DContext *This, CONST CHAR *Text, FLOAT *Width, FLOAT *Height);

/* Gradients */
static HRESULT STDMETHODCALLTYPE FbGfx_SetFillGradient(IFramebuffer2DContext *This, CONST FB_GRADIENT_DESC *Gradient);
static HRESULT STDMETHODCALLTYPE FbGfx_SetStrokeGradient(IFramebuffer2DContext *This, CONST FB_GRADIENT_DESC *Gradient);

/* Utility */
static HRESULT STDMETHODCALLTYPE FbGfx_GetSurface(IFramebuffer2DContext *This, IFramebufferSurface **Surface);
static HRESULT STDMETHODCALLTYPE FbGfx_Flush(IFramebuffer2DContext *This);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebuffer2DContextVtbl gGfxVtbl = {
    .QueryInterface         = FbGfx_QueryInterface,
    .AddRef                 = FbGfx_AddRef,
    .Release                = FbGfx_Release,
    .Save                   = FbGfx_Save,
    .Restore                = FbGfx_Restore,
    .Reset                  = FbGfx_Reset,
    .SetStrokeColor         = FbGfx_SetStrokeColor,
    .SetFillColor           = FbGfx_SetFillColor,
    .SetLineWidth           = FbGfx_SetLineWidth,
    .SetLineCap             = FbGfx_SetLineCap,
    .SetLineJoin            = FbGfx_SetLineJoin,
    .SetMiterLimit          = FbGfx_SetMiterLimit,
    .SetGlobalAlpha         = FbGfx_SetGlobalAlpha,
    .SetCompositeOp         = FbGfx_SetCompositeOp,
    .SetAntiAlias           = FbGfx_SetAntiAlias,
    .SetTransform           = FbGfx_SetTransform,
    .GetTransform           = FbGfx_GetTransform,
    .ResetTransform         = FbGfx_ResetTransform,
    .Translate              = FbGfx_Translate,
    .Scale                  = FbGfx_Scale,
    .Rotate                 = FbGfx_Rotate,
    .SetClipRect            = FbGfx_SetClipRect,
    .IntersectClipRect      = FbGfx_IntersectClipRect,
    .ResetClip              = FbGfx_ResetClip,
    .Clear                  = FbGfx_Clear,
    .DrawPixel              = FbGfx_DrawPixel,
    .DrawLine               = FbGfx_DrawLine,
    .StrokeRect             = FbGfx_StrokeRect,
    .FillRect               = FbGfx_FillRect,
    .StrokeCircle           = FbGfx_StrokeCircle,
    .FillCircle             = FbGfx_FillCircle,
    .StrokeEllipse          = FbGfx_StrokeEllipse,
    .FillEllipse            = FbGfx_FillEllipse,
    .StrokeArc              = FbGfx_StrokeArc,
    .BeginPath              = FbGfx_BeginPath,
    .ClosePath              = FbGfx_ClosePath,
    .MoveTo                 = FbGfx_MoveTo,
    .LineTo                 = FbGfx_LineTo,
    .QuadraticCurveTo       = FbGfx_QuadraticCurveTo,
    .BezierCurveTo          = FbGfx_BezierCurveTo,
    .Arc                    = FbGfx_Arc,
    .Stroke                 = FbGfx_Stroke,
    .Fill                   = FbGfx_Fill,
    .StrokePolygon          = FbGfx_StrokePolygon,
    .FillPolygon            = FbGfx_FillPolygon,
    .DrawImage              = FbGfx_DrawImage,
    .DrawImageScaled        = FbGfx_DrawImageScaled,
    .DrawImageEx            = FbGfx_DrawImageEx,
    .SetFont                = FbGfx_SetFont,
    .GetFont                = FbGfx_GetFont,
    .DrawText               = FbGfx_DrawText,
    .MeasureText            = FbGfx_MeasureText,
    .SetFillGradient        = FbGfx_SetFillGradient,
    .SetStrokeGradient      = FbGfx_SetStrokeGradient,
    .GetSurface             = FbGfx_GetSurface,
    .Flush                  = FbGfx_Flush,
};

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbGfx_QueryInterface(
    IFramebuffer2DContext *This,
    REFIID riid,
    VOID **ppvObject
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (ANX_ISEQUAL_GUID(riid, &IID_IUnknown) ||
        ANX_ISEQUAL_GUID(riid, &IID_IFramebuffer2DContext)) {
        IFramebuffer2DContext_AddRef(This);
        *ppvObject = This;
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINT32 STDMETHODCALLTYPE
FbGfx_AddRef(
    IFramebuffer2DContext *This
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    return ANX_InterlockedIncrement(&Ctx->RefCount.RefCount);
}

static UINT32 STDMETHODCALLTYPE
FbGfx_Release(
    IFramebuffer2DContext *This
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    UINT32 RefCount = ANX_InterlockedDecrement(&Ctx->RefCount.RefCount);

    if (RefCount == 0) {
        /* Release surface if we own it */
        if (Ctx->Surface != NULL) {
            IUnknown_Release((IUnknown *)Ctx->Surface);
        }

        /* Release font if set */
        if (Ctx->State.Font != NULL) {
            IUnknown_Release((IUnknown *)Ctx->State.Font);
        }

        /* Free path buffer */
        if (Ctx->PathPoints != NULL) {
            ANX_FREE(Ctx->PathPoints);
        }

        ANX_FREE(Ctx);
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

static VOID
FbGfx_InitDefaultState(
    FB_GFX_STATE *State
    )
{
    ANX_MEMSET(State, 0, sizeof(FB_GFX_STATE));

    /* Default colors */
    State->StrokeColor.Red = 0;
    State->StrokeColor.Green = 0;
    State->StrokeColor.Blue = 0;
    State->StrokeColor.Alpha = 255;

    State->FillColor.Red = 255;
    State->FillColor.Green = 255;
    State->FillColor.Blue = 255;
    State->FillColor.Alpha = 255;

    /* Default line style */
    State->LineWidth = 1.0f;
    State->LineCap = FbLineCapButt;
    State->LineJoin = FbLineJoinMiter;
    State->MiterLimit = 10.0f;

    /* Default blending */
    State->GlobalAlpha = 1.0f;
    State->CompositeOp = FbCompositeOver;
    State->AntiAlias = FALSE;

    /* Identity transform */
    State->Transform.A = 1.0f;
    State->Transform.B = 0.0f;
    State->Transform.C = 0.0f;
    State->Transform.D = 1.0f;
    State->Transform.E = 0.0f;
    State->Transform.F = 0.0f;

    /* No clipping */
    State->HasClip = FALSE;

    State->Font = NULL;
}

/* Apply transform to point */
static VOID
FbGfx_TransformPoint(
    CONST FB_TRANSFORM *Transform,
    FLOAT *X,
    FLOAT *Y
    )
{
    FLOAT OrigX = *X;
    FLOAT OrigY = *Y;

    *X = Transform->A * OrigX + Transform->C * OrigY + Transform->E;
    *Y = Transform->B * OrigX + Transform->D * OrigY + Transform->F;
}

/* Check if point is within clip rectangle */
static BOOLEAN
FbGfx_IsClipped(
    FB_GFX_CONTEXT_IMPL *Ctx,
    INT32 X,
    INT32 Y
    )
{
    if (!Ctx->State.HasClip) {
        return FALSE;
    }

    if (X < Ctx->State.ClipRect.X || X >= (Ctx->State.ClipRect.X + (INT32)Ctx->State.ClipRect.Width)) {
        return TRUE;
    }

    if (Y < Ctx->State.ClipRect.Y || Y >= (Ctx->State.ClipRect.Y + (INT32)Ctx->State.ClipRect.Height)) {
        return TRUE;
    }

    return FALSE;
}

/* Bresenham line drawing algorithm */
static VOID
FbGfx_DrawLineBresenham(
    FB_GFX_CONTEXT_IMPL *Ctx,
    INT32 X0,
    INT32 Y0,
    INT32 X1,
    INT32 Y1,
    FB_COLOR Color
    )
{
    INT32 DX = ANX_ABS(X1 - X0);
    INT32 DY = ANX_ABS(Y1 - Y0);
    INT32 SX = X0 < X1 ? 1 : -1;
    INT32 SY = Y0 < Y1 ? 1 : -1;
    INT32 Err = DX - DY;
    INT32 X = X0, Y = Y0;

    while (TRUE) {
        /* Draw pixel if not clipped */
        if (!FbGfx_IsClipped(Ctx, X, Y)) {
            IFramebufferSurface_SetPixel(Ctx->Surface, X, Y, Color);
        }

        if (X == X1 && Y == Y1) {
            break;
        }

        INT32 E2 = 2 * Err;
        if (E2 > -DY) {
            Err -= DY;
            X += SX;
        }
        if (E2 < DX) {
            Err += DX;
            Y += SY;
        }
    }
}

/* Midpoint circle algorithm */
static VOID
FbGfx_DrawCirclePoints(
    FB_GFX_CONTEXT_IMPL *Ctx,
    INT32 CenterX,
    INT32 CenterY,
    INT32 X,
    INT32 Y,
    FB_COLOR Color,
    BOOLEAN Fill
    )
{
    if (Fill) {
        /* Draw horizontal lines for filled circle */
        INT32 StartX, EndX;

        /* Draw line from (-X, Y) to (X, Y) */
        StartX = CenterX - X;
        EndX = CenterX + X;
        for (INT32 DrawX = StartX; DrawX <= EndX; DrawX++) {
            if (!FbGfx_IsClipped(Ctx, DrawX, CenterY + Y)) {
                IFramebufferSurface_SetPixel(Ctx->Surface, DrawX, CenterY + Y, Color);
            }
            if (Y != 0 && !FbGfx_IsClipped(Ctx, DrawX, CenterY - Y)) {
                IFramebufferSurface_SetPixel(Ctx->Surface, DrawX, CenterY - Y, Color);
            }
        }

        /* Draw line from (-Y, X) to (Y, X) */
        if (X != Y) {
            StartX = CenterX - Y;
            EndX = CenterX + Y;
            for (INT32 DrawX = StartX; DrawX <= EndX; DrawX++) {
                if (!FbGfx_IsClipped(Ctx, DrawX, CenterY + X)) {
                    IFramebufferSurface_SetPixel(Ctx->Surface, DrawX, CenterY + X, Color);
                }
                if (X != 0 && !FbGfx_IsClipped(Ctx, DrawX, CenterY - X)) {
                    IFramebufferSurface_SetPixel(Ctx->Surface, DrawX, CenterY - X, Color);
                }
            }
        }
    } else {
        /* Draw 8 symmetric points for outline */
        if (!FbGfx_IsClipped(Ctx, CenterX + X, CenterY + Y))
            IFramebufferSurface_SetPixel(Ctx->Surface, CenterX + X, CenterY + Y, Color);
        if (!FbGfx_IsClipped(Ctx, CenterX - X, CenterY + Y))
            IFramebufferSurface_SetPixel(Ctx->Surface, CenterX - X, CenterY + Y, Color);
        if (!FbGfx_IsClipped(Ctx, CenterX + X, CenterY - Y))
            IFramebufferSurface_SetPixel(Ctx->Surface, CenterX + X, CenterY - Y, Color);
        if (!FbGfx_IsClipped(Ctx, CenterX - X, CenterY - Y))
            IFramebufferSurface_SetPixel(Ctx->Surface, CenterX - X, CenterY - Y, Color);
        if (!FbGfx_IsClipped(Ctx, CenterX + Y, CenterY + X))
            IFramebufferSurface_SetPixel(Ctx->Surface, CenterX + Y, CenterY + X, Color);
        if (!FbGfx_IsClipped(Ctx, CenterX - Y, CenterY + X))
            IFramebufferSurface_SetPixel(Ctx->Surface, CenterX - Y, CenterY + X, Color);
        if (!FbGfx_IsClipped(Ctx, CenterX + Y, CenterY - X))
            IFramebufferSurface_SetPixel(Ctx->Surface, CenterX + Y, CenterY - X, Color);
        if (!FbGfx_IsClipped(Ctx, CenterX - Y, CenterY - X))
            IFramebufferSurface_SetPixel(Ctx->Surface, CenterX - Y, CenterY - X, Color);
    }
}

static VOID
FbGfx_DrawCircleMidpoint(
    FB_GFX_CONTEXT_IMPL *Ctx,
    INT32 CenterX,
    INT32 CenterY,
    INT32 Radius,
    FB_COLOR Color,
    BOOLEAN Fill
    )
{
    INT32 X = 0;
    INT32 Y = Radius;
    INT32 D = 1 - Radius;

    FbGfx_DrawCirclePoints(Ctx, CenterX, CenterY, X, Y, Color, Fill);

    while (X < Y) {
        X++;
        if (D < 0) {
            D += 2 * X + 1;
        } else {
            Y--;
            D += 2 * (X - Y) + 1;
        }
        FbGfx_DrawCirclePoints(Ctx, CenterX, CenterY, X, Y, Color, Fill);
    }
}

/* --------------------------------------------------------------- */
/*  Path Helper Functions                                           */
/* --------------------------------------------------------------- */

/* Add point to path */
static HRESULT
FbGfx_AddPathPoint(
    FB_GFX_CONTEXT_IMPL *Ctx,
    FLOAT X,
    FLOAT Y,
    FB_PATH_COMMAND Command
    )
{
    /* Grow path buffer if needed */
    if (Ctx->PathPointCount >= Ctx->PathPointCapacity) {
        UINT32 NewCapacity = Ctx->PathPointCapacity == 0 ? 256 : Ctx->PathPointCapacity * 2;
        if (NewCapacity > FB_GFX_MAX_PATH_POINTS) {
            return E_OUTOFMEMORY;
        }

        FB_PATH_POINT *NewPoints = (FB_PATH_POINT *)ANX_REALLOC(
            Ctx->PathPoints,
            NewCapacity * sizeof(FB_PATH_POINT)
        );

        if (NewPoints == NULL) {
            return E_OUTOFMEMORY;
        }

        Ctx->PathPoints = NewPoints;
        Ctx->PathPointCapacity = NewCapacity;
    }

    /* Add point */
    Ctx->PathPoints[Ctx->PathPointCount].X = X;
    Ctx->PathPoints[Ctx->PathPointCount].Y = Y;
    Ctx->PathPoints[Ctx->PathPointCount].Command = Command;
    Ctx->PathPointCount++;

    return S_OK;
}

/* Flatten quadratic Bézier curve into line segments */
static HRESULT
FbGfx_FlattenQuadraticBezier(
    FB_GFX_CONTEXT_IMPL *Ctx,
    FLOAT X0,
    FLOAT Y0,
    FLOAT X1,
    FLOAT Y1,
    FLOAT X2,
    FLOAT Y2
    )
{
    /* Subdivide curve into line segments
     * Using recursive subdivision with flatness tolerance
     */
    CONST FLOAT TOLERANCE = 0.5f;
    UINT32 Steps = 16;  /* Number of segments */

    for (UINT32 I = 1; I <= Steps; I++) {
        FLOAT T = (FLOAT)I / (FLOAT)Steps;
        FLOAT T1 = 1.0f - T;

        /* Quadratic Bézier formula: B(t) = (1-t)²P0 + 2(1-t)tP1 + t²P2 */
        FLOAT X = T1 * T1 * X0 + 2.0f * T1 * T * X1 + T * T * X2;
        FLOAT Y = T1 * T1 * Y0 + 2.0f * T1 * T * Y1 + T * T * Y2;

        FbGfx_AddPathPoint(Ctx, X, Y, FbPathLineTo);
    }

    return S_OK;
}

/* Flatten cubic Bézier curve into line segments */
static HRESULT
FbGfx_FlattenCubicBezier(
    FB_GFX_CONTEXT_IMPL *Ctx,
    FLOAT X0,
    FLOAT Y0,
    FLOAT X1,
    FLOAT Y1,
    FLOAT X2,
    FLOAT Y2,
    FLOAT X3,
    FLOAT Y3
    )
{
    UINT32 Steps = 20;  /* Number of segments */

    for (UINT32 I = 1; I <= Steps; I++) {
        FLOAT T = (FLOAT)I / (FLOAT)Steps;
        FLOAT T1 = 1.0f - T;

        /* Cubic Bézier formula: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3 */
        FLOAT X = T1 * T1 * T1 * X0 +
                  3.0f * T1 * T1 * T * X1 +
                  3.0f * T1 * T * T * X2 +
                  T * T * T * X3;

        FLOAT Y = T1 * T1 * T1 * Y0 +
                  3.0f * T1 * T1 * T * Y1 +
                  3.0f * T1 * T * T * Y2 +
                  T * T * T * Y3;

        FbGfx_AddPathPoint(Ctx, X, Y, FbPathLineTo);
    }

    return S_OK;
}

/* Scanline edge for polygon filling */
typedef struct _FB_SCANLINE_EDGE {
    INT32   YMin, YMax;
    FLOAT   X;          /* Current X coordinate */
    FLOAT   DxDy;       /* Slope (change in X per Y) */
    INT32   WindingDir; /* +1 or -1 for winding number */
} FB_SCANLINE_EDGE;

/* Fill polygon using scanline algorithm */
static VOID
FbGfx_FillPolygonScanline(
    FB_GFX_CONTEXT_IMPL *Ctx,
    FB_PATH_POINT *Points,
    UINT32 PointCount,
    FB_COLOR Color,
    FB_FILL_RULE FillRule
    )
{
    if (PointCount < 3) {
        return;  /* Need at least 3 points */
    }

    /* Find Y bounds */
    INT32 YMin = (INT32)Points[0].Y;
    INT32 YMax = (INT32)Points[0].Y;

    for (UINT32 I = 1; I < PointCount; I++) {
        INT32 Y = (INT32)Points[I].Y;
        if (Y < YMin) YMin = Y;
        if (Y > YMax) YMax = Y;
    }

    /* Build edge table */
    FB_SCANLINE_EDGE *Edges = (FB_SCANLINE_EDGE *)ANX_MALLOC(
        sizeof(FB_SCANLINE_EDGE) * PointCount
    );

    if (Edges == NULL) {
        return;
    }

    UINT32 EdgeCount = 0;

    for (UINT32 I = 0; I < PointCount; I++) {
        UINT32 J = (I + 1) % PointCount;

        INT32 Y1 = (INT32)Points[I].Y;
        INT32 Y2 = (INT32)Points[J].Y;

        if (Y1 == Y2) {
            continue;  /* Skip horizontal edges */
        }

        /* Create edge */
        FB_SCANLINE_EDGE *Edge = &Edges[EdgeCount++];

        if (Y1 < Y2) {
            Edge->YMin = Y1;
            Edge->YMax = Y2;
            Edge->X = Points[I].X;
            Edge->DxDy = (Points[J].X - Points[I].X) / (FLOAT)(Y2 - Y1);
            Edge->WindingDir = 1;
        } else {
            Edge->YMin = Y2;
            Edge->YMax = Y1;
            Edge->X = Points[J].X;
            Edge->DxDy = (Points[I].X - Points[J].X) / (FLOAT)(Y1 - Y2);
            Edge->WindingDir = -1;
        }
    }

    /* Scanline fill */
    for (INT32 Y = YMin; Y <= YMax; Y++) {
        /* Find active edges for this scanline */
        FLOAT *Intersections = (FLOAT *)ANX_MALLOC(sizeof(FLOAT) * EdgeCount * 2);
        INT32 *Windings = (INT32 *)ANX_MALLOC(sizeof(INT32) * EdgeCount * 2);
        UINT32 IntersectionCount = 0;

        for (UINT32 I = 0; I < EdgeCount; I++) {
            FB_SCANLINE_EDGE *Edge = &Edges[I];

            if (Y >= Edge->YMin && Y < Edge->YMax) {
                Intersections[IntersectionCount] = Edge->X;
                Windings[IntersectionCount] = Edge->WindingDir;
                IntersectionCount++;

                /* Update X for next scanline */
                Edge->X += Edge->DxDy;
            }
        }

        /* Sort intersections by X */
        for (UINT32 I = 0; I < IntersectionCount - 1; I++) {
            for (UINT32 J = I + 1; J < IntersectionCount; J++) {
                if (Intersections[J] < Intersections[I]) {
                    FLOAT TempX = Intersections[I];
                    Intersections[I] = Intersections[J];
                    Intersections[J] = TempX;

                    INT32 TempW = Windings[I];
                    Windings[I] = Windings[J];
                    Windings[J] = TempW;
                }
            }
        }

        /* Fill pixels based on fill rule */
        if (FillRule == FbFillRuleEvenOdd) {
            /* Even-odd rule: toggle fill state at each intersection */
            for (UINT32 I = 0; I < IntersectionCount; I += 2) {
                if (I + 1 < IntersectionCount) {
                    INT32 X1 = (INT32)Intersections[I];
                    INT32 X2 = (INT32)Intersections[I + 1];

                    for (INT32 X = X1; X <= X2; X++) {
                        if (!FbGfx_IsClipped(Ctx, X, Y)) {
                            IFramebufferSurface_SetPixel(Ctx->Surface, X, Y, Color);
                        }
                    }
                }
            }
        } else {
            /* Non-zero winding rule */
            INT32 Winding = 0;
            UINT32 I = 0;

            while (I < IntersectionCount) {
                INT32 X1 = (INT32)Intersections[I];
                Winding += Windings[I];
                I++;

                if (Winding != 0 && I < IntersectionCount) {
                    INT32 X2 = (INT32)Intersections[I];

                    for (INT32 X = X1; X <= X2; X++) {
                        if (!FbGfx_IsClipped(Ctx, X, Y)) {
                            IFramebufferSurface_SetPixel(Ctx->Surface, X, Y, Color);
                        }
                    }
                }
            }
        }

        ANX_FREE(Intersections);
        ANX_FREE(Windings);
    }

    ANX_FREE(Edges);
}

/* --------------------------------------------------------------- */
/*  State Management Implementation                                 */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbGfx_Save(
    IFramebuffer2DContext *This
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    if (Ctx->StateStackDepth >= FB_GFX_MAX_STATE_STACK) {
        return E_OUTOFMEMORY;  /* Stack full */
    }

    /* Push current state onto stack */
    Ctx->StateStack[Ctx->StateStackDepth] = Ctx->State;

    /* AddRef font if present */
    if (Ctx->State.Font != NULL) {
        IUnknown_AddRef((IUnknown *)Ctx->State.Font);
    }

    Ctx->StateStackDepth++;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_Restore(
    IFramebuffer2DContext *This
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    if (Ctx->StateStackDepth == 0) {
        return E_FAIL;  /* Stack empty */
    }

    /* Release current font if present */
    if (Ctx->State.Font != NULL) {
        IUnknown_Release((IUnknown *)Ctx->State.Font);
    }

    /* Pop state from stack */
    Ctx->StateStackDepth--;
    Ctx->State = Ctx->StateStack[Ctx->StateStackDepth];

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_Reset(
    IFramebuffer2DContext *This
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    /* Release font if present */
    if (Ctx->State.Font != NULL) {
        IUnknown_Release((IUnknown *)Ctx->State.Font);
    }

    /* Reset to defaults */
    FbGfx_InitDefaultState(&Ctx->State);
    Ctx->StateStackDepth = 0;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Stroke and Fill State Implementation                           */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbGfx_SetStrokeColor(
    IFramebuffer2DContext *This,
    FB_COLOR Color
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    Ctx->State.StrokeColor = Color;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_SetFillColor(
    IFramebuffer2DContext *This,
    FB_COLOR Color
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    Ctx->State.FillColor = Color;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_SetLineWidth(
    IFramebuffer2DContext *This,
    FLOAT Width
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    Ctx->State.LineWidth = Width;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_SetLineCap(
    IFramebuffer2DContext *This,
    FB_LINE_CAP Cap
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    Ctx->State.LineCap = Cap;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_SetLineJoin(
    IFramebuffer2DContext *This,
    FB_LINE_JOIN Join
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    Ctx->State.LineJoin = Join;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_SetMiterLimit(
    IFramebuffer2DContext *This,
    FLOAT Limit
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    Ctx->State.MiterLimit = Limit;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_SetGlobalAlpha(
    IFramebuffer2DContext *This,
    FLOAT Alpha
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    Ctx->State.GlobalAlpha = Alpha;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_SetCompositeOp(
    IFramebuffer2DContext *This,
    FB_COMPOSITE_OP Op
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    Ctx->State.CompositeOp = Op;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_SetAntiAlias(
    IFramebuffer2DContext *This,
    BOOLEAN Enabled
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    Ctx->State.AntiAlias = Enabled;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Transform Implementation                                        */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbGfx_SetTransform(
    IFramebuffer2DContext *This,
    CONST FB_TRANSFORM *Transform
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    if (Transform == NULL) {
        return E_POINTER;
    }

    Ctx->State.Transform = *Transform;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_GetTransform(
    IFramebuffer2DContext *This,
    FB_TRANSFORM *Transform
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    if (Transform == NULL) {
        return E_POINTER;
    }

    *Transform = Ctx->State.Transform;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_ResetTransform(
    IFramebuffer2DContext *This
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    /* Identity matrix */
    Ctx->State.Transform.A = 1.0f;
    Ctx->State.Transform.B = 0.0f;
    Ctx->State.Transform.C = 0.0f;
    Ctx->State.Transform.D = 1.0f;
    Ctx->State.Transform.E = 0.0f;
    Ctx->State.Transform.F = 0.0f;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_Translate(
    IFramebuffer2DContext *This,
    FLOAT X,
    FLOAT Y
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    /* Multiply current transform by translation matrix */
    Ctx->State.Transform.E += Ctx->State.Transform.A * X + Ctx->State.Transform.C * Y;
    Ctx->State.Transform.F += Ctx->State.Transform.B * X + Ctx->State.Transform.D * Y;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_Scale(
    IFramebuffer2DContext *This,
    FLOAT ScaleX,
    FLOAT ScaleY
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    /* Multiply current transform by scale matrix */
    Ctx->State.Transform.A *= ScaleX;
    Ctx->State.Transform.B *= ScaleX;
    Ctx->State.Transform.C *= ScaleY;
    Ctx->State.Transform.D *= ScaleY;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_Rotate(
    IFramebuffer2DContext *This,
    FLOAT Angle
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    FLOAT Cos = ANX_COSF(Angle);
    FLOAT Sin = ANX_SINF(Angle);

    /* Multiply current transform by rotation matrix */
    FLOAT A = Ctx->State.Transform.A;
    FLOAT B = Ctx->State.Transform.B;
    FLOAT C = Ctx->State.Transform.C;
    FLOAT D = Ctx->State.Transform.D;

    Ctx->State.Transform.A = A * Cos + C * Sin;
    Ctx->State.Transform.B = B * Cos + D * Sin;
    Ctx->State.Transform.C = C * Cos - A * Sin;
    Ctx->State.Transform.D = D * Cos - B * Sin;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Clipping Implementation                                         */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbGfx_SetClipRect(
    IFramebuffer2DContext *This,
    CONST FB_RECT *Rect
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    if (Rect == NULL) {
        return E_POINTER;
    }

    Ctx->State.ClipRect = *Rect;
    Ctx->State.HasClip = TRUE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_IntersectClipRect(
    IFramebuffer2DContext *This,
    CONST FB_RECT *Rect
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    if (Rect == NULL) {
        return E_POINTER;
    }

    if (!Ctx->State.HasClip) {
        Ctx->State.ClipRect = *Rect;
        Ctx->State.HasClip = TRUE;
        return S_OK;
    }

    /* Intersect rectangles */
    INT32 X1 = ANX_MAX(Ctx->State.ClipRect.X, Rect->X);
    INT32 Y1 = ANX_MAX(Ctx->State.ClipRect.Y, Rect->Y);
    INT32 X2 = ANX_MIN(Ctx->State.ClipRect.X + (INT32)Ctx->State.ClipRect.Width,
                       Rect->X + (INT32)Rect->Width);
    INT32 Y2 = ANX_MIN(Ctx->State.ClipRect.Y + (INT32)Ctx->State.ClipRect.Height,
                       Rect->Y + (INT32)Rect->Height);

    if (X2 <= X1 || Y2 <= Y1) {
        /* Empty intersection */
        Ctx->State.ClipRect.Width = 0;
        Ctx->State.ClipRect.Height = 0;
    } else {
        Ctx->State.ClipRect.X = X1;
        Ctx->State.ClipRect.Y = Y1;
        Ctx->State.ClipRect.Width = X2 - X1;
        Ctx->State.ClipRect.Height = Y2 - Y1;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_ResetClip(
    IFramebuffer2DContext *This
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    Ctx->State.HasClip = FALSE;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Primitive Drawing Implementation                                */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbGfx_Clear(
    IFramebuffer2DContext *This,
    FB_COLOR Color
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;
    return IFramebufferSurface_Clear(Ctx->Surface, Color);
}

static HRESULT STDMETHODCALLTYPE
FbGfx_DrawPixel(
    IFramebuffer2DContext *This,
    INT32 X,
    INT32 Y,
    FB_COLOR Color
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    if (FbGfx_IsClipped(Ctx, X, Y)) {
        return S_OK;
    }

    return IFramebufferSurface_SetPixel(Ctx->Surface, X, Y, Color);
}

static HRESULT STDMETHODCALLTYPE
FbGfx_DrawLine(
    IFramebuffer2DContext *This,
    FLOAT X0,
    FLOAT Y0,
    FLOAT X1,
    FLOAT Y1
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    /* Apply transform */
    FbGfx_TransformPoint(&Ctx->State.Transform, &X0, &Y0);
    FbGfx_TransformPoint(&Ctx->State.Transform, &X1, &Y1);

    /* Draw line using Bresenham */
    FbGfx_DrawLineBresenham(Ctx, (INT32)X0, (INT32)Y0, (INT32)X1, (INT32)Y1,
                            Ctx->State.StrokeColor);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_StrokeRect(
    IFramebuffer2DContext *This,
    FLOAT X,
    FLOAT Y,
    FLOAT Width,
    FLOAT Height
    )
{
    /* Draw four lines forming rectangle */
    FbGfx_DrawLine(This, X, Y, X + Width, Y);
    FbGfx_DrawLine(This, X + Width, Y, X + Width, Y + Height);
    FbGfx_DrawLine(This, X + Width, Y + Height, X, Y + Height);
    FbGfx_DrawLine(This, X, Y + Height, X, Y);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_FillRect(
    IFramebuffer2DContext *This,
    FLOAT X,
    FLOAT Y,
    FLOAT Width,
    FLOAT Height
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    /* Apply transform to corner */
    FbGfx_TransformPoint(&Ctx->State.Transform, &X, &Y);

    INT32 StartX = (INT32)X;
    INT32 StartY = (INT32)Y;
    INT32 EndX = (INT32)(X + Width);
    INT32 EndY = (INT32)(Y + Height);

    /* Fill rectangle scanline by scanline */
    for (INT32 DrawY = StartY; DrawY < EndY; DrawY++) {
        for (INT32 DrawX = StartX; DrawX < EndX; DrawX++) {
            if (!FbGfx_IsClipped(Ctx, DrawX, DrawY)) {
                IFramebufferSurface_SetPixel(Ctx->Surface, DrawX, DrawY,
                                              Ctx->State.FillColor);
            }
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_StrokeCircle(
    IFramebuffer2DContext *This,
    FLOAT CenterX,
    FLOAT CenterY,
    FLOAT Radius
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    /* Apply transform */
    FbGfx_TransformPoint(&Ctx->State.Transform, &CenterX, &CenterY);

    /* Draw circle using midpoint algorithm */
    FbGfx_DrawCircleMidpoint(Ctx, (INT32)CenterX, (INT32)CenterY, (INT32)Radius,
                             Ctx->State.StrokeColor, FALSE);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_FillCircle(
    IFramebuffer2DContext *This,
    FLOAT CenterX,
    FLOAT CenterY,
    FLOAT Radius
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    /* Apply transform */
    FbGfx_TransformPoint(&Ctx->State.Transform, &CenterX, &CenterY);

    /* Draw filled circle using midpoint algorithm */
    FbGfx_DrawCircleMidpoint(Ctx, (INT32)CenterX, (INT32)CenterY, (INT32)Radius,
                             Ctx->State.FillColor, TRUE);

    return S_OK;
}

/* Ellipse and arc methods - simplified stubs for now */
static HRESULT STDMETHODCALLTYPE FbGfx_StrokeEllipse(IFramebuffer2DContext *This, FLOAT CenterX, FLOAT CenterY, FLOAT RadiusX, FLOAT RadiusY) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_FillEllipse(IFramebuffer2DContext *This, FLOAT CenterX, FLOAT CenterY, FLOAT RadiusX, FLOAT RadiusY) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_StrokeArc(IFramebuffer2DContext *This, FLOAT CenterX, FLOAT CenterY, FLOAT Radius, FLOAT StartAngle, FLOAT EndAngle) { return E_NOTIMPL; }

/* Path methods - stubs for now */
static HRESULT STDMETHODCALLTYPE FbGfx_BeginPath(IFramebuffer2DContext *This) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_ClosePath(IFramebuffer2DContext *This) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_MoveTo(IFramebuffer2DContext *This, FLOAT X, FLOAT Y) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_LineTo(IFramebuffer2DContext *This, FLOAT X, FLOAT Y) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_QuadraticCurveTo(IFramebuffer2DContext *This, FLOAT ControlX, FLOAT ControlY, FLOAT X, FLOAT Y) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_BezierCurveTo(IFramebuffer2DContext *This, FLOAT C1X, FLOAT C1Y, FLOAT C2X, FLOAT C2Y, FLOAT X, FLOAT Y) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_Arc(IFramebuffer2DContext *This, FLOAT CenterX, FLOAT CenterY, FLOAT Radius, FLOAT StartAngle, FLOAT EndAngle, BOOLEAN CCW) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_Stroke(IFramebuffer2DContext *This) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_Fill(IFramebuffer2DContext *This, FB_FILL_RULE FillRule) { return E_NOTIMPL; }

/* Polygon methods - stubs for now */
static HRESULT STDMETHODCALLTYPE FbGfx_StrokePolygon(IFramebuffer2DContext *This, CONST FB_POINT *Points, UINT32 PointCount) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_FillPolygon(IFramebuffer2DContext *This, CONST FB_POINT *Points, UINT32 PointCount, FB_FILL_RULE FillRule) { return E_NOTIMPL; }

/* Image drawing - delegate to surface */
static HRESULT STDMETHODCALLTYPE
FbGfx_DrawImage(
    IFramebuffer2DContext *This,
    IFramebufferImage *Image,
    FLOAT X,
    FLOAT Y
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    if (Image == NULL) {
        return E_POINTER;
    }

    /* Apply transform */
    FbGfx_TransformPoint(&Ctx->State.Transform, &X, &Y);

    /* Use surface's BlitImage */
    return IFramebufferSurface_BlitImage(Ctx->Surface, (INT32)X, (INT32)Y,
                                         Image, NULL, FbRopCopy);
}

static HRESULT STDMETHODCALLTYPE FbGfx_DrawImageScaled(IFramebuffer2DContext *This, IFramebufferImage *Image, FLOAT X, FLOAT Y, FLOAT Width, FLOAT Height) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_DrawImageEx(IFramebuffer2DContext *This, IFramebufferImage *Image, CONST FB_RECT *SourceRect, CONST FB_RECT *DestRect) { return E_NOTIMPL; }

/* Text methods - stubs for now */
static HRESULT STDMETHODCALLTYPE FbGfx_SetFont(IFramebuffer2DContext *This, IFramebufferFont *Font) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_GetFont(IFramebuffer2DContext *This, IFramebufferFont **Font) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_DrawText(IFramebuffer2DContext *This, CONST CHAR *Text, FLOAT X, FLOAT Y) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_MeasureText(IFramebuffer2DContext *This, CONST CHAR *Text, FLOAT *Width, FLOAT *Height) { return E_NOTIMPL; }

/* Gradient methods - stubs for now */
static HRESULT STDMETHODCALLTYPE FbGfx_SetFillGradient(IFramebuffer2DContext *This, CONST FB_GRADIENT_DESC *Gradient) { return E_NOTIMPL; }
static HRESULT STDMETHODCALLTYPE FbGfx_SetStrokeGradient(IFramebuffer2DContext *This, CONST FB_GRADIENT_DESC *Gradient) { return E_NOTIMPL; }

/* Utility */
static HRESULT STDMETHODCALLTYPE
FbGfx_GetSurface(
    IFramebuffer2DContext *This,
    IFramebufferSurface **Surface
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx = (FB_GFX_CONTEXT_IMPL *)This;

    if (Surface == NULL) {
        return E_POINTER;
    }

    IUnknown_AddRef((IUnknown *)Ctx->Surface);
    *Surface = Ctx->Surface;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbGfx_Flush(
    IFramebuffer2DContext *This
    )
{
    /* No-op for now - could batch operations in future */
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Factory Functions                                              */
/* --------------------------------------------------------------- */

IFramebuffer2DContext *
FbCreate2DContext(
    IN IFramebufferSurface *Surface
    )
{
    FB_GFX_CONTEXT_IMPL *Ctx;

    if (Surface == NULL) {
        return NULL;
    }

    /* Allocate context */
    Ctx = (FB_GFX_CONTEXT_IMPL *)ANX_MALLOC(sizeof(FB_GFX_CONTEXT_IMPL));
    if (Ctx == NULL) {
        return NULL;
    }

    ANX_MEMSET(Ctx, 0, sizeof(FB_GFX_CONTEXT_IMPL));
    Ctx->Base.lpVtbl = &gGfxVtbl;
    Ctx->RefCount.RefCount = 1;

    /* Store surface reference */
    IUnknown_AddRef((IUnknown *)Surface);
    Ctx->Surface = Surface;
    Ctx->OwnsSurface = FALSE;

    /* Initialize default state */
    FbGfx_InitDefaultState(&Ctx->State);

    return &Ctx->Base;
}

IFramebuffer2DContext *
FbCreate2DContextForScreen(
    IN IFramebufferScreen *Screen
    )
{
    IFramebufferSurface *Surface;
    IFramebuffer2DContext *Context;

    if (Screen == NULL) {
        return NULL;
    }

    /* Create surface from screen dimensions */
    /* TODO: This should create a surface wrapper around the screen */
    /* For now, return NULL */
    return NULL;
}
