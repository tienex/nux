/*++
    Module Name:

        graphics2d.h

    Abstract:

        2D graphics context interface for high-level drawing operations.
        Provides primitives, paths, transforms, and state management.

        Similar to Windows GDI, Cairo, or HTML5 Canvas 2D API.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/framebuffer.h>

/* Forward declarations */
typedef struct IFramebufferSurface IFramebufferSurface;
typedef struct IFramebufferImage IFramebufferImage;
typedef struct IFramebufferFont IFramebufferFont;

/* --------------------------------------------------------------- */
/*  Graphics State Definitions                                     */
/* --------------------------------------------------------------- */

/* Line cap style */
typedef enum _FB_LINE_CAP {
    FbLineCapButt       = 0,  /* Square end at exact endpoint */
    FbLineCapRound      = 1,  /* Rounded end */
    FbLineCapSquare     = 2,  /* Square end extended by half line width */
} FB_LINE_CAP;

/* Line join style */
typedef enum _FB_LINE_JOIN {
    FbLineJoinMiter     = 0,  /* Sharp corner */
    FbLineJoinRound     = 1,  /* Rounded corner */
    FbLineJoinBevel     = 2,  /* Beveled corner */
} FB_LINE_JOIN;

/* Fill rule */
typedef enum _FB_FILL_RULE {
    FbFillRuleNonZero   = 0,  /* Non-zero winding rule */
    FbFillRuleEvenOdd   = 1,  /* Even-odd rule */
} FB_FILL_RULE;

/* Composite operation (blending mode) */
typedef enum _FB_COMPOSITE_OP {
    FbCompositeCopy         = 0,  /* Replace destination */
    FbCompositeOver         = 1,  /* Alpha blend (src over dest) */
    FbCompositeAdd          = 2,  /* Additive blend */
    FbCompositeXor          = 3,  /* XOR blend */
} FB_COMPOSITE_OP;

/* Gradient type */
typedef enum _FB_GRADIENT_TYPE {
    FbGradientLinear    = 0,  /* Linear gradient */
    FbGradientRadial    = 1,  /* Radial gradient */
} FB_GRADIENT_TYPE;

/* Gradient stop */
typedef struct _FB_GRADIENT_STOP {
    FLOAT       Offset;       /* 0.0 to 1.0 */
    FB_COLOR    Color;
} FB_GRADIENT_STOP;

/* Gradient descriptor */
typedef struct _FB_GRADIENT_DESC {
    FB_GRADIENT_TYPE    Type;
    UINT32              StopCount;
    FB_GRADIENT_STOP    *Stops;

    /* For linear gradient */
    FLOAT               X0, Y0;  /* Start point */
    FLOAT               X1, Y1;  /* End point */

    /* For radial gradient */
    FLOAT               CenterX, CenterY;
    FLOAT               Radius;
} FB_GRADIENT_DESC;

/* Transform matrix (2x3 affine transform) */
typedef struct _FB_TRANSFORM {
    FLOAT   A, B, C, D, E, F;
    /* Matrix form:
     * | A  C  E |
     * | B  D  F |
     * | 0  0  1 |
     *
     * x' = A*x + C*y + E
     * y' = B*x + D*y + F
     */
} FB_TRANSFORM;

/* Path command */
typedef enum _FB_PATH_COMMAND {
    FbPathMoveTo        = 0,
    FbPathLineTo        = 1,
    FbPathQuadraticTo   = 2,  /* Quadratic bezier */
    FbPathBezierTo      = 3,  /* Cubic bezier */
    FbPathArcTo         = 4,
    FbPathClose         = 5,
} FB_PATH_COMMAND;

/* --------------------------------------------------------------- */
/*  IFramebuffer2DPath - 2D Path Object                            */
/* --------------------------------------------------------------- */

/*
 * Path object for building complex shapes.
 * Paths are built using MoveTo, LineTo, curves, etc., then
 * rendered using Stroke or Fill on a graphics context.
 *
 * Similar to Direct2D ID2D1PathGeometry, CoreGraphics CGPath,
 * or HTML5 Canvas Path2D.
 */

#define ANX_IID_IFramebuffer2DPath "FB000021-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebuffer2DPath,
    0xFB000021, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebuffer2DPath, IUnknown,
    IID_IFramebuffer2DPath, ANX_IID_IFramebuffer2DPath)

    /* Begin a new subpath (close previous if open) */
    ANX_IFACE_METHOD(HRESULT, BeginFigure, (
        VOID))

    /* Close current subpath with a line to start point */
    ANX_IFACE_METHOD(HRESULT, CloseFigure, (
        VOID))

    /* Move to point without drawing */
    ANX_IFACE_METHOD(HRESULT, MoveTo, (
        IN FLOAT X,
        IN FLOAT Y))

    /* Draw line to point */
    ANX_IFACE_METHOD(HRESULT, LineTo, (
        IN FLOAT X,
        IN FLOAT Y))

    /* Draw quadratic Bézier curve */
    ANX_IFACE_METHOD(HRESULT, QuadraticBezierTo, (
        IN FLOAT ControlX,
        IN FLOAT ControlY,
        IN FLOAT X,
        IN FLOAT Y))

    /* Draw cubic Bézier curve */
    ANX_IFACE_METHOD(HRESULT, CubicBezierTo, (
        IN FLOAT Control1X,
        IN FLOAT Control1Y,
        IN FLOAT Control2X,
        IN FLOAT Control2Y,
        IN FLOAT X,
        IN FLOAT Y))

    /* Draw arc */
    ANX_IFACE_METHOD(HRESULT, ArcTo, (
        IN FLOAT X,
        IN FLOAT Y,
        IN FLOAT RadiusX,
        IN FLOAT RadiusY,
        IN FLOAT Rotation,
        IN BOOLEAN LargeArc,
        IN BOOLEAN Sweep))

    /* Add rectangle to path */
    ANX_IFACE_METHOD(HRESULT, AddRectangle, (
        IN FLOAT X,
        IN FLOAT Y,
        IN FLOAT Width,
        IN FLOAT Height))

    /* Add circle to path */
    ANX_IFACE_METHOD(HRESULT, AddCircle, (
        IN FLOAT CenterX,
        IN FLOAT CenterY,
        IN FLOAT Radius))

    /* Add ellipse to path */
    ANX_IFACE_METHOD(HRESULT, AddEllipse, (
        IN FLOAT CenterX,
        IN FLOAT CenterY,
        IN FLOAT RadiusX,
        IN FLOAT RadiusY))

    /* Reset path (clear all points) */
    ANX_IFACE_METHOD(HRESULT, Reset, (
        VOID))

    /* Get bounding rectangle */
    ANX_IFACE_METHOD(HRESULT, GetBounds, (
        OUT FB_RECT *Bounds))

    /* Get number of points in path */
    ANX_IFACE_METHOD(HRESULT, GetPointCount, (
        OUT UINT32 *Count))

    /* Transform path by matrix */
    ANX_IFACE_METHOD(HRESULT, Transform, (
        IN CONST FB_TRANSFORM *Transform))

    /* ----------------------------------------------------------------- */
    /* Path Data Access (for hit testing, rendering, etc.)              */
    /* ----------------------------------------------------------------- */

    /* Get flattened path points (curves converted to line segments)
     * Points array must be allocated by caller
     * Returns S_OK with point count, or E_OUTOFMEMORY if buffer too small
     */
    ANX_IFACE_METHOD(HRESULT, GetPoints, (
        OUT FB_POINT *Points,
        IN UINT32 MaxPoints,
        OUT UINT32 *NumPoints))

    /* Get path points with commands (includes curve control points)
     * Returns raw path data with command information
     */
    ANX_IFACE_METHOD(HRESULT, GetPathData, (
        OUT FB_PATH_COMMAND *Commands,
        OUT FB_POINT *Points,
        IN UINT32 MaxPoints,
        OUT UINT32 *NumPoints))

ANX_END_INTERFACE(IFramebuffer2DPath)

/* --------------------------------------------------------------- */
/*  IFramebuffer2DContext - 2D Graphics Context                    */
/* --------------------------------------------------------------- */

/*
 * 2D graphics context for high-level drawing operations.
 * Maintains rendering state (pen, brush, transform, clip region).
 * Draws to an IFramebufferSurface (or screen).
 */

#define ANX_IID_IFramebuffer2DContext "FB000020-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebuffer2DContext,
    0xFB000020, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebuffer2DContext, IUnknown,
    IID_IFramebuffer2DContext, ANX_IID_IFramebuffer2DContext)

    /* ----------------------------------------------------------------- */
    /* State Management                                                  */
    /* ----------------------------------------------------------------- */

    /* Save current graphics state to stack */
    ANX_IFACE_METHOD(HRESULT, Save, (
        VOID))

    /* Restore graphics state from stack */
    ANX_IFACE_METHOD(HRESULT, Restore, (
        VOID))

    /* Reset to default state */
    ANX_IFACE_METHOD(HRESULT, Reset, (
        VOID))

    /* ----------------------------------------------------------------- */
    /* Stroke and Fill State                                             */
    /* ----------------------------------------------------------------- */

    /* Set stroke color */
    ANX_IFACE_METHOD(HRESULT, SetStrokeColor, (
        IN FB_COLOR Color))

    /* Set fill color */
    ANX_IFACE_METHOD(HRESULT, SetFillColor, (
        IN FB_COLOR Color))

    /* Set line width */
    ANX_IFACE_METHOD(HRESULT, SetLineWidth, (
        IN FLOAT Width))

    /* Set line cap style */
    ANX_IFACE_METHOD(HRESULT, SetLineCap, (
        IN FB_LINE_CAP Cap))

    /* Set line join style */
    ANX_IFACE_METHOD(HRESULT, SetLineJoin, (
        IN FB_LINE_JOIN Join))

    /* Set miter limit (for mitered joins) */
    ANX_IFACE_METHOD(HRESULT, SetMiterLimit, (
        IN FLOAT Limit))

    /* Set global alpha (0.0 = transparent, 1.0 = opaque) */
    ANX_IFACE_METHOD(HRESULT, SetGlobalAlpha, (
        IN FLOAT Alpha))

    /* Set composite operation (blend mode) */
    ANX_IFACE_METHOD(HRESULT, SetCompositeOp, (
        IN FB_COMPOSITE_OP Op))

    /* Set anti-aliasing enabled */
    ANX_IFACE_METHOD(HRESULT, SetAntiAlias, (
        IN BOOLEAN Enabled))

    /* ----------------------------------------------------------------- */
    /* Transform Operations                                              */
    /* ----------------------------------------------------------------- */

    /* Set transform matrix */
    ANX_IFACE_METHOD(HRESULT, SetTransform, (
        IN CONST FB_TRANSFORM *Transform))

    /* Get current transform matrix */
    ANX_IFACE_METHOD(HRESULT, GetTransform, (
        OUT FB_TRANSFORM *Transform))

    /* Reset transform to identity */
    ANX_IFACE_METHOD(HRESULT, ResetTransform, (
        VOID))

    /* Translate origin */
    ANX_IFACE_METHOD(HRESULT, Translate, (
        IN FLOAT X,
        IN FLOAT Y))

    /* Scale */
    ANX_IFACE_METHOD(HRESULT, Scale, (
        IN FLOAT ScaleX,
        IN FLOAT ScaleY))

    /* Rotate (angle in radians) */
    ANX_IFACE_METHOD(HRESULT, Rotate, (
        IN FLOAT Angle))

    /* ----------------------------------------------------------------- */
    /* Clipping                                                          */
    /* ----------------------------------------------------------------- */

    /* Set clip rectangle (replaces current clip) */
    ANX_IFACE_METHOD(HRESULT, SetClipRect, (
        IN CONST FB_RECT *Rect))

    /* Intersect with clip rectangle */
    ANX_IFACE_METHOD(HRESULT, IntersectClipRect, (
        IN CONST FB_RECT *Rect))

    /* Reset clipping to full surface */
    ANX_IFACE_METHOD(HRESULT, ResetClip, (
        VOID))

    /* ----------------------------------------------------------------- */
    /* Primitive Drawing (Simple Shapes)                                */
    /* ----------------------------------------------------------------- */

    /* Clear entire surface with color */
    ANX_IFACE_METHOD(HRESULT, Clear, (
        IN FB_COLOR Color))

    /* Draw pixel */
    ANX_IFACE_METHOD(HRESULT, DrawPixel, (
        IN INT32 X,
        IN INT32 Y,
        IN FB_COLOR Color))

    /* Draw line */
    ANX_IFACE_METHOD(HRESULT, DrawLine, (
        IN FLOAT X0,
        IN FLOAT Y0,
        IN FLOAT X1,
        IN FLOAT Y1))

    /* Draw rectangle outline */
    ANX_IFACE_METHOD(HRESULT, StrokeRect, (
        IN FLOAT X,
        IN FLOAT Y,
        IN FLOAT Width,
        IN FLOAT Height))

    /* Draw filled rectangle */
    ANX_IFACE_METHOD(HRESULT, FillRect, (
        IN FLOAT X,
        IN FLOAT Y,
        IN FLOAT Width,
        IN FLOAT Height))

    /* Draw circle outline */
    ANX_IFACE_METHOD(HRESULT, StrokeCircle, (
        IN FLOAT CenterX,
        IN FLOAT CenterY,
        IN FLOAT Radius))

    /* Draw filled circle */
    ANX_IFACE_METHOD(HRESULT, FillCircle, (
        IN FLOAT CenterX,
        IN FLOAT CenterY,
        IN FLOAT Radius))

    /* Draw ellipse outline */
    ANX_IFACE_METHOD(HRESULT, StrokeEllipse, (
        IN FLOAT CenterX,
        IN FLOAT CenterY,
        IN FLOAT RadiusX,
        IN FLOAT RadiusY))

    /* Draw filled ellipse */
    ANX_IFACE_METHOD(HRESULT, FillEllipse, (
        IN FLOAT CenterX,
        IN FLOAT CenterY,
        IN FLOAT RadiusX,
        IN FLOAT RadiusY))

    /* Draw arc outline */
    ANX_IFACE_METHOD(HRESULT, StrokeArc, (
        IN FLOAT CenterX,
        IN FLOAT CenterY,
        IN FLOAT Radius,
        IN FLOAT StartAngle,
        IN FLOAT EndAngle))

    /* ----------------------------------------------------------------- */
    /* Path-based Drawing (uses IFramebuffer2DPath objects)             */
    /* ----------------------------------------------------------------- */

    /* Stroke a path with current stroke settings */
    ANX_IFACE_METHOD(HRESULT, StrokePath, (
        IN IFramebuffer2DPath *Path))

    /* Fill a path with current fill settings */
    ANX_IFACE_METHOD(HRESULT, FillPath, (
        IN IFramebuffer2DPath *Path,
        IN FB_FILL_RULE FillRule))

    /* Set clipping region to path */
    ANX_IFACE_METHOD(HRESULT, ClipToPath, (
        IN IFramebuffer2DPath *Path,
        IN FB_FILL_RULE FillRule))

    /* ----------------------------------------------------------------- */
    /* Polygon Drawing                                                   */
    /* ----------------------------------------------------------------- */

    /* Draw polygon outline */
    ANX_IFACE_METHOD(HRESULT, StrokePolygon, (
        IN CONST FB_POINT *Points,
        IN UINT32 PointCount))

    /* Draw filled polygon */
    ANX_IFACE_METHOD(HRESULT, FillPolygon, (
        IN CONST FB_POINT *Points,
        IN UINT32 PointCount,
        IN FB_FILL_RULE FillRule))

    /* ----------------------------------------------------------------- */
    /* Image Drawing                                                     */
    /* ----------------------------------------------------------------- */

    /* Draw image at position */
    ANX_IFACE_METHOD(HRESULT, DrawImage, (
        IN IFramebufferImage *Image,
        IN FLOAT X,
        IN FLOAT Y))

    /* Draw image scaled to rectangle */
    ANX_IFACE_METHOD(HRESULT, DrawImageScaled, (
        IN IFramebufferImage *Image,
        IN FLOAT X,
        IN FLOAT Y,
        IN FLOAT Width,
        IN FLOAT Height))

    /* Draw portion of image (source rect) to destination rect */
    ANX_IFACE_METHOD(HRESULT, DrawImageEx, (
        IN IFramebufferImage *Image,
        IN CONST FB_RECT *SourceRect,
        IN CONST FB_RECT *DestRect))

    /* ----------------------------------------------------------------- */
    /* Text Drawing                                                      */
    /* ----------------------------------------------------------------- */

    /* Set current font */
    ANX_IFACE_METHOD(HRESULT, SetFont, (
        IN IFramebufferFont *Font))

    /* Get current font */
    ANX_IFACE_METHOD(HRESULT, GetFont, (
        OUT IFramebufferFont **Font))

    /* Draw text at position */
    ANX_IFACE_METHOD(HRESULT, DrawText, (
        IN CONST CHAR *Text,
        IN FLOAT X,
        IN FLOAT Y))

    /* Measure text dimensions */
    ANX_IFACE_METHOD(HRESULT, MeasureText, (
        IN CONST CHAR *Text,
        OUT FLOAT *Width,
        OUT FLOAT *Height))

    /* ----------------------------------------------------------------- */
    /* Gradient Support                                                  */
    /* ----------------------------------------------------------------- */

    /* Set fill to linear gradient */
    ANX_IFACE_METHOD(HRESULT, SetFillGradient, (
        IN CONST FB_GRADIENT_DESC *Gradient))

    /* Set stroke to linear gradient */
    ANX_IFACE_METHOD(HRESULT, SetStrokeGradient, (
        IN CONST FB_GRADIENT_DESC *Gradient))

    /* ----------------------------------------------------------------- */
    /* Utility                                                           */
    /* ----------------------------------------------------------------- */

    /* Get underlying surface */
    ANX_IFACE_METHOD(HRESULT, GetSurface, (
        OUT IFramebufferSurface **Surface))

    /* Flush pending operations */
    ANX_IFACE_METHOD(HRESULT, Flush, (
        VOID))

ANX_END_INTERFACE(IFramebuffer2DContext)

/* --------------------------------------------------------------- */
/*  Helper Macros for C (COBJMACROS style)                          */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IFramebuffer2DContext_Save(This) \
    ((This)->lpVtbl->Save(This))
#define IFramebuffer2DContext_Restore(This) \
    ((This)->lpVtbl->Restore(This))
#define IFramebuffer2DContext_Reset(This) \
    ((This)->lpVtbl->Reset(This))

#define IFramebuffer2DContext_SetStrokeColor(This, Color) \
    ((This)->lpVtbl->SetStrokeColor(This, Color))
#define IFramebuffer2DContext_SetFillColor(This, Color) \
    ((This)->lpVtbl->SetFillColor(This, Color))
#define IFramebuffer2DContext_SetLineWidth(This, Width) \
    ((This)->lpVtbl->SetLineWidth(This, Width))

#define IFramebuffer2DContext_Translate(This, X, Y) \
    ((This)->lpVtbl->Translate(This, X, Y))
#define IFramebuffer2DContext_Scale(This, SX, SY) \
    ((This)->lpVtbl->Scale(This, SX, SY))
#define IFramebuffer2DContext_Rotate(This, Angle) \
    ((This)->lpVtbl->Rotate(This, Angle))

#define IFramebuffer2DContext_Clear(This, Color) \
    ((This)->lpVtbl->Clear(This, Color))
#define IFramebuffer2DContext_DrawLine(This, X0, Y0, X1, Y1) \
    ((This)->lpVtbl->DrawLine(This, X0, Y0, X1, Y1))
#define IFramebuffer2DContext_StrokeRect(This, X, Y, W, H) \
    ((This)->lpVtbl->StrokeRect(This, X, Y, W, H))
#define IFramebuffer2DContext_FillRect(This, X, Y, W, H) \
    ((This)->lpVtbl->FillRect(This, X, Y, W, H))
#define IFramebuffer2DContext_StrokeCircle(This, X, Y, R) \
    ((This)->lpVtbl->StrokeCircle(This, X, Y, R))
#define IFramebuffer2DContext_FillCircle(This, X, Y, R) \
    ((This)->lpVtbl->FillCircle(This, X, Y, R))

#define IFramebuffer2DContext_StrokePath(This, Path) \
    ((This)->lpVtbl->StrokePath(This, Path))
#define IFramebuffer2DContext_FillPath(This, Path, Rule) \
    ((This)->lpVtbl->FillPath(This, Path, Rule))
#define IFramebuffer2DContext_ClipToPath(This, Path, Rule) \
    ((This)->lpVtbl->ClipToPath(This, Path, Rule))

#define IFramebuffer2DContext_DrawImage(This, Img, X, Y) \
    ((This)->lpVtbl->DrawImage(This, Img, X, Y))
#define IFramebuffer2DContext_DrawText(This, Text, X, Y) \
    ((This)->lpVtbl->DrawText(This, Text, X, Y))

#endif /* !__cplusplus */

/* --------------------------------------------------------------- */
/*  Factory Functions                                              */
/* --------------------------------------------------------------- */

/*
 * Create a 2D path object for building complex shapes.
 * Paths can be reused for multiple draw operations.
 */
IFramebuffer2DPath *
FbCreate2DPath(
    VOID
    );

/*
 * Create a 2D graphics context for a surface.
 * Context maintains rendering state and provides high-level drawing API.
 * Delegates to surface/backend hardware accelerations where available.
 */
IFramebuffer2DContext *
FbCreate2DContext(
    IN IFramebufferSurface *Surface
    );

/* Create 2D context for a screen (wraps screen as surface) */
IFramebuffer2DContext *
FbCreate2DContextForScreen(
    IN IFramebufferScreen *Screen
    );
