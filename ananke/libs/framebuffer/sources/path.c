/*++
    Module Name:

        path.c

    Abstract:

        Implementation of IFramebuffer2DPath for building complex 2D shapes.
        Supports lines, curves, arcs, and transformations.

    Environment:

        C implementation.
--*/

#include <ananke/framebuffer/graphics2d.h>
#include <ananke/memory.h>
#include <ananke/math.h>

/* Path segment structure */
typedef struct _FB_PATH_SEGMENT {
    FB_PATH_COMMAND Command;
    FLOAT           Points[6];  /* Max 6 floats for cubic bezier */
} FB_PATH_SEGMENT;

/* Path implementation */
typedef struct _FB_PATH_IMPL {
    IFramebuffer2DPath  Base;
    REFOBJ              RefCount;

    /* Path segments */
    FB_PATH_SEGMENT     *Segments;
    UINT32              SegmentCount;
    UINT32              SegmentCapacity;

    /* Current position */
    FLOAT               CurrentX;
    FLOAT               CurrentY;

    /* Start of current figure */
    FLOAT               FigureStartX;
    FLOAT               FigureStartY;
    BOOLEAN             FigureOpen;

    /* Cached bounds */
    FB_RECT             Bounds;
    BOOLEAN             BoundsDirty;

    /* Cached flattened points */
    FB_POINT            *FlattenedPoints;
    UINT32              FlattenedCount;
    BOOLEAN             FlattenedDirty;
} FB_PATH_IMPL;

/* Forward declarations */
static VOID FbPath_UpdateBounds(FB_PATH_IMPL *Path);
static VOID FbPath_FlattenPath(FB_PATH_IMPL *Path);
static VOID FbPath_FlattenQuadraticBezier(
    FB_PATH_IMPL *Path, FLOAT X0, FLOAT Y0, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2);
static VOID FbPath_FlattenCubicBezier(
    FB_PATH_IMPL *Path, FLOAT X0, FLOAT Y0, FLOAT X1, FLOAT Y1,
    FLOAT X2, FLOAT Y2, FLOAT X3, FLOAT Y3);

/* --------------------------------------------------------------- */
/*  IUnknown Methods                                               */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbPath_QueryInterface(
    IFramebuffer2DPath *This,
    REFIID Riid,
    VOID **Object
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    if (AnxIsEqualGuid(Riid, &IID_IUnknown) ||
        AnxIsEqualGuid(Riid, &IID_IFramebuffer2DPath)) {
        IUnknown_AddRef((IUnknown *)This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE
FbPath_AddRef(
    IFramebuffer2DPath *This
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;
    return AnxInterlockedIncrement(&Path->RefCount);
}

static ULONG STDMETHODCALLTYPE
FbPath_Release(
    IFramebuffer2DPath *This
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;
    ULONG RefCount = AnxInterlockedDecrement(&Path->RefCount);

    if (RefCount == 0) {
        if (Path->Segments != NULL) {
            AnxFree(Path->Segments);
        }
        if (Path->FlattenedPoints != NULL) {
            AnxFree(Path->FlattenedPoints);
        }
        AnxFree(Path);
    }

    return RefCount;
}

/* --------------------------------------------------------------- */
/*  Helper Functions                                               */
/* --------------------------------------------------------------- */

static HRESULT
FbPath_EnsureCapacity(
    FB_PATH_IMPL *Path,
    UINT32 RequiredCapacity
    )
{
    if (RequiredCapacity <= Path->SegmentCapacity) {
        return S_OK;
    }

    /* Grow by 1.5x */
    UINT32 NewCapacity = Path->SegmentCapacity * 3 / 2;
    if (NewCapacity < RequiredCapacity) {
        NewCapacity = RequiredCapacity;
    }
    if (NewCapacity < 16) {
        NewCapacity = 16;
    }

    FB_PATH_SEGMENT *NewSegments = (FB_PATH_SEGMENT *)AnxRealloc(
        Path->Segments,
        NewCapacity * sizeof(FB_PATH_SEGMENT));

    if (NewSegments == NULL) {
        return E_OUTOFMEMORY;
    }

    Path->Segments = NewSegments;
    Path->SegmentCapacity = NewCapacity;
    return S_OK;
}

static HRESULT
FbPath_AddSegment(
    FB_PATH_IMPL *Path,
    FB_PATH_COMMAND Command,
    CONST FLOAT *Points,
    UINT32 PointCount
    )
{
    HRESULT Hr = FbPath_EnsureCapacity(Path, Path->SegmentCount + 1);
    if (FAILED(Hr)) {
        return Hr;
    }

    FB_PATH_SEGMENT *Seg = &Path->Segments[Path->SegmentCount++];
    Seg->Command = Command;

    for (UINT32 I = 0; I < PointCount * 2; I++) {
        Seg->Points[I] = Points[I];
    }

    Path->BoundsDirty = TRUE;
    Path->FlattenedDirty = TRUE;

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  IFramebuffer2DPath Methods                                     */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbPath_BeginFigure(
    IFramebuffer2DPath *This
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    /* Close previous figure if open */
    if (Path->FigureOpen) {
        FbPath_CloseFigure(This);
    }

    Path->FigureOpen = TRUE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPath_CloseFigure(
    IFramebuffer2DPath *This
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    if (!Path->FigureOpen) {
        return S_FALSE;
    }

    /* Add close command */
    FLOAT Points[2] = { Path->FigureStartX, Path->FigureStartY };
    FbPath_AddSegment(Path, FbPathClose, Points, 1);

    Path->CurrentX = Path->FigureStartX;
    Path->CurrentY = Path->FigureStartY;
    Path->FigureOpen = FALSE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPath_MoveTo(
    IFramebuffer2DPath *This,
    FLOAT X,
    FLOAT Y
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    FLOAT Points[2] = { X, Y };
    HRESULT Hr = FbPath_AddSegment(Path, FbPathMoveTo, Points, 1);

    if (SUCCEEDED(Hr)) {
        Path->CurrentX = X;
        Path->CurrentY = Y;
        Path->FigureStartX = X;
        Path->FigureStartY = Y;
        Path->FigureOpen = TRUE;
    }

    return Hr;
}

static HRESULT STDMETHODCALLTYPE
FbPath_LineTo(
    IFramebuffer2DPath *This,
    FLOAT X,
    FLOAT Y
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    FLOAT Points[2] = { X, Y };
    HRESULT Hr = FbPath_AddSegment(Path, FbPathLineTo, Points, 1);

    if (SUCCEEDED(Hr)) {
        Path->CurrentX = X;
        Path->CurrentY = Y;
    }

    return Hr;
}

static HRESULT STDMETHODCALLTYPE
FbPath_QuadraticBezierTo(
    IFramebuffer2DPath *This,
    FLOAT ControlX,
    FLOAT ControlY,
    FLOAT X,
    FLOAT Y
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    FLOAT Points[4] = { ControlX, ControlY, X, Y };
    HRESULT Hr = FbPath_AddSegment(Path, FbPathQuadraticTo, Points, 2);

    if (SUCCEEDED(Hr)) {
        Path->CurrentX = X;
        Path->CurrentY = Y;
    }

    return Hr;
}

static HRESULT STDMETHODCALLTYPE
FbPath_CubicBezierTo(
    IFramebuffer2DPath *This,
    FLOAT Control1X,
    FLOAT Control1Y,
    FLOAT Control2X,
    FLOAT Control2Y,
    FLOAT X,
    FLOAT Y
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    FLOAT Points[6] = { Control1X, Control1Y, Control2X, Control2Y, X, Y };
    HRESULT Hr = FbPath_AddSegment(Path, FbPathBezierTo, Points, 3);

    if (SUCCEEDED(Hr)) {
        Path->CurrentX = X;
        Path->CurrentY = Y;
    }

    return Hr;
}

/*
 * Convert SVG arc to cubic Bezier curves.
 * Based on the algorithm from SVG specification.
 */
static HRESULT STDMETHODCALLTYPE
FbPath_ArcTo(
    IFramebuffer2DPath *This,
    FLOAT X,
    FLOAT Y,
    FLOAT RadiusX,
    FLOAT RadiusY,
    FLOAT Rotation,
    BOOLEAN LargeArc,
    BOOLEAN Sweep
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    /* Current point */
    FLOAT X0 = Path->CurrentX;
    FLOAT Y0 = Path->CurrentY;

    /* If start and end are the same, draw nothing */
    if (X0 == X && Y0 == Y) {
        return S_OK;
    }

    /* If radius is zero, draw line */
    if (RadiusX == 0.0f || RadiusY == 0.0f) {
        return FbPath_LineTo(This, X, Y);
    }

    /* Ensure radii are positive */
    RadiusX = ANX_ABS(RadiusX);
    RadiusY = ANX_ABS(RadiusY);

    /* Convert rotation to radians */
    FLOAT Phi = Rotation * ANX_PI / 180.0f;
    FLOAT CosPhi = ANX_COSF(Phi);
    FLOAT SinPhi = ANX_SINF(Phi);

    /* Transform to unit circle */
    FLOAT DX = (X0 - X) / 2.0f;
    FLOAT DY = (Y0 - Y) / 2.0f;
    FLOAT X1 = CosPhi * DX + SinPhi * DY;
    FLOAT Y1 = -SinPhi * DX + CosPhi * DY;

    /* Correct radii if needed */
    FLOAT Lambda = (X1 * X1) / (RadiusX * RadiusX) + (Y1 * Y1) / (RadiusY * RadiusY);
    if (Lambda > 1.0f) {
        FLOAT SqrtLambda = ANX_SQRTF(Lambda);
        RadiusX *= SqrtLambda;
        RadiusY *= SqrtLambda;
    }

    /* Calculate center of arc */
    FLOAT SqRx = RadiusX * RadiusX;
    FLOAT SqRy = RadiusY * RadiusY;
    FLOAT SqX1 = X1 * X1;
    FLOAT SqY1 = Y1 * Y1;

    FLOAT Radicand = (SqRx * SqRy - SqRx * SqY1 - SqRy * SqX1) /
                     (SqRx * SqY1 + SqRy * SqX1);

    if (Radicand < 0.0f) {
        Radicand = 0.0f;
    }

    FLOAT Sq = ANX_SQRTF(Radicand);
    if (LargeArc == Sweep) {
        Sq = -Sq;
    }

    FLOAT CX1 = Sq * RadiusX * Y1 / RadiusY;
    FLOAT CY1 = -Sq * RadiusY * X1 / RadiusX;

    /* Transform center back */
    FLOAT CX = CosPhi * CX1 - SinPhi * CY1 + (X0 + X) / 2.0f;
    FLOAT CY = SinPhi * CX1 + CosPhi * CY1 + (Y0 + Y) / 2.0f;

    /* Calculate start and end angles */
    FLOAT Theta1 = ANX_ATAN2F((Y1 - CY1) / RadiusY, (X1 - CX1) / RadiusX);
    FLOAT Theta2 = ANX_ATAN2F((-Y1 - CY1) / RadiusY, (-X1 - CX1) / RadiusX);

    FLOAT DTheta = Theta2 - Theta1;

    /* Adjust for sweep direction */
    if (Sweep && DTheta < 0.0f) {
        DTheta += 2.0f * ANX_PI;
    } else if (!Sweep && DTheta > 0.0f) {
        DTheta -= 2.0f * ANX_PI;
    }

    /* Split arc into segments (max 90 degrees each) */
    INT32 Segments = (INT32)ANX_CEIL(ANX_ABS(DTheta) / (ANX_PI / 2.0f));
    FLOAT DeltaTheta = DTheta / (FLOAT)Segments;

    /* Magic constant for approximating quarter circle with cubic Bezier */
    FLOAT Alpha = ANX_SINF(DeltaTheta) * (ANX_SQRTF(4.0f + 3.0f * ANX_TANF(DeltaTheta / 2.0f) *
                                                     ANX_TANF(DeltaTheta / 2.0f)) - 1.0f) / 3.0f;

    FLOAT CosEta1 = ANX_COSF(Theta1);
    FLOAT SinEta1 = ANX_SINF(Theta1);

    /* Generate Bezier curves for each segment */
    for (INT32 I = 0; I < Segments; I++) {
        FLOAT Eta2 = Theta1 + (I + 1) * DeltaTheta;
        FLOAT CosEta2 = ANX_COSF(Eta2);
        FLOAT SinEta2 = ANX_SINF(Eta2);

        /* Control points in unit space */
        FLOAT Q1X = CosEta1;
        FLOAT Q1Y = SinEta1;
        FLOAT Q2X = CosEta2;
        FLOAT Q2Y = SinEta2;

        FLOAT CP1X = Q1X - Q1Y * Alpha;
        FLOAT CP1Y = Q1Y + Q1X * Alpha;
        FLOAT CP2X = Q2X + Q2Y * Alpha;
        FLOAT CP2Y = Q2Y - Q2X * Alpha;

        /* Transform back to ellipse */
        FLOAT CP1XE = RadiusX * CP1X;
        FLOAT CP1YE = RadiusY * CP1Y;
        FLOAT CP2XE = RadiusX * CP2X;
        FLOAT CP2YE = RadiusY * CP2Y;
        FLOAT P2XE = RadiusX * Q2X;
        FLOAT P2YE = RadiusY * Q2Y;

        /* Apply rotation and translation */
        FLOAT CP1XF = CosPhi * CP1XE - SinPhi * CP1YE + CX;
        FLOAT CP1YF = SinPhi * CP1XE + CosPhi * CP1YE + CY;
        FLOAT CP2XF = CosPhi * CP2XE - SinPhi * CP2YE + CX;
        FLOAT CP2YF = SinPhi * CP2XE + CosPhi * CP2YE + CY;
        FLOAT P2XF = CosPhi * P2XE - SinPhi * P2YE + CX;
        FLOAT P2YF = SinPhi * P2XE + CosPhi * P2YE + CY;

        /* Add cubic Bezier curve */
        HRESULT Hr = FbPath_CubicBezierTo(This, CP1XF, CP1YF, CP2XF, CP2YF, P2XF, P2YF);
        if (FAILED(Hr)) {
            return Hr;
        }

        /* Update for next segment */
        CosEta1 = CosEta2;
        SinEta1 = SinEta2;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPath_AddRectangle(
    IFramebuffer2DPath *This,
    FLOAT X,
    FLOAT Y,
    FLOAT Width,
    FLOAT Height
    )
{
    HRESULT Hr;

    Hr = FbPath_MoveTo(This, X, Y);
    if (FAILED(Hr)) return Hr;

    Hr = FbPath_LineTo(This, X + Width, Y);
    if (FAILED(Hr)) return Hr;

    Hr = FbPath_LineTo(This, X + Width, Y + Height);
    if (FAILED(Hr)) return Hr;

    Hr = FbPath_LineTo(This, X, Y + Height);
    if (FAILED(Hr)) return Hr;

    Hr = FbPath_CloseFigure(This);
    return Hr;
}

static HRESULT STDMETHODCALLTYPE
FbPath_AddCircle(
    IFramebuffer2DPath *This,
    FLOAT CenterX,
    FLOAT CenterY,
    FLOAT Radius
    )
{
    /* Use 4 cubic Bezier curves to approximate circle
     * Magic constant for circle approximation with Bezier */
    CONST FLOAT K = 0.5522847498f;
    FLOAT Offset = Radius * K;

    HRESULT Hr;

    /* Start at right point */
    Hr = FbPath_MoveTo(This, CenterX + Radius, CenterY);
    if (FAILED(Hr)) return Hr;

    /* Top right quadrant */
    Hr = FbPath_CubicBezierTo(This,
        CenterX + Radius, CenterY - Offset,
        CenterX + Offset, CenterY - Radius,
        CenterX, CenterY - Radius);
    if (FAILED(Hr)) return Hr;

    /* Top left quadrant */
    Hr = FbPath_CubicBezierTo(This,
        CenterX - Offset, CenterY - Radius,
        CenterX - Radius, CenterY - Offset,
        CenterX - Radius, CenterY);
    if (FAILED(Hr)) return Hr;

    /* Bottom left quadrant */
    Hr = FbPath_CubicBezierTo(This,
        CenterX - Radius, CenterY + Offset,
        CenterX - Offset, CenterY + Radius,
        CenterX, CenterY + Radius);
    if (FAILED(Hr)) return Hr;

    /* Bottom right quadrant */
    Hr = FbPath_CubicBezierTo(This,
        CenterX + Offset, CenterY + Radius,
        CenterX + Radius, CenterY + Offset,
        CenterX + Radius, CenterY);
    if (FAILED(Hr)) return Hr;

    Hr = FbPath_CloseFigure(This);
    return Hr;
}

static HRESULT STDMETHODCALLTYPE
FbPath_AddEllipse(
    IFramebuffer2DPath *This,
    FLOAT CenterX,
    FLOAT CenterY,
    FLOAT RadiusX,
    FLOAT RadiusY
    )
{
    CONST FLOAT K = 0.5522847498f;
    FLOAT OffsetX = RadiusX * K;
    FLOAT OffsetY = RadiusY * K;

    HRESULT Hr;

    /* Start at right point */
    Hr = FbPath_MoveTo(This, CenterX + RadiusX, CenterY);
    if (FAILED(Hr)) return Hr;

    /* Top right quadrant */
    Hr = FbPath_CubicBezierTo(This,
        CenterX + RadiusX, CenterY - OffsetY,
        CenterX + OffsetX, CenterY - RadiusY,
        CenterX, CenterY - RadiusY);
    if (FAILED(Hr)) return Hr;

    /* Top left quadrant */
    Hr = FbPath_CubicBezierTo(This,
        CenterX - OffsetX, CenterY - RadiusY,
        CenterX - RadiusX, CenterY - OffsetY,
        CenterX - RadiusX, CenterY);
    if (FAILED(Hr)) return Hr;

    /* Bottom left quadrant */
    Hr = FbPath_CubicBezierTo(This,
        CenterX - RadiusX, CenterY + OffsetY,
        CenterX - OffsetX, CenterY + RadiusY,
        CenterX, CenterY + RadiusY);
    if (FAILED(Hr)) return Hr;

    /* Bottom right quadrant */
    Hr = FbPath_CubicBezierTo(This,
        CenterX + OffsetX, CenterY + RadiusY,
        CenterX + RadiusX, CenterY + OffsetY,
        CenterX + RadiusX, CenterY);
    if (FAILED(Hr)) return Hr;

    Hr = FbPath_CloseFigure(This);
    return Hr;
}

static HRESULT STDMETHODCALLTYPE
FbPath_Reset(
    IFramebuffer2DPath *This
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    Path->SegmentCount = 0;
    Path->CurrentX = 0.0f;
    Path->CurrentY = 0.0f;
    Path->FigureStartX = 0.0f;
    Path->FigureStartY = 0.0f;
    Path->FigureOpen = FALSE;
    Path->BoundsDirty = TRUE;
    Path->FlattenedDirty = TRUE;

    if (Path->FlattenedPoints != NULL) {
        AnxFree(Path->FlattenedPoints);
        Path->FlattenedPoints = NULL;
        Path->FlattenedCount = 0;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPath_GetBounds(
    IFramebuffer2DPath *This,
    FB_RECT *Bounds
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    if (Path->SegmentCount == 0) {
        Bounds->X = 0;
        Bounds->Y = 0;
        Bounds->Width = 0;
        Bounds->Height = 0;
        return S_FALSE;
    }

    if (Path->BoundsDirty) {
        FbPath_UpdateBounds(Path);
    }

    *Bounds = Path->Bounds;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPath_GetPointCount(
    IFramebuffer2DPath *This,
    UINT32 *Count
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    if (Path->FlattenedDirty) {
        FbPath_FlattenPath(Path);
    }

    *Count = Path->FlattenedCount;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPath_Transform(
    IFramebuffer2DPath *This,
    CONST FB_TRANSFORM *Transform
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    /* Transform all points in all segments */
    for (UINT32 I = 0; I < Path->SegmentCount; I++) {
        FB_PATH_SEGMENT *Seg = &Path->Segments[I];
        UINT32 PointCount = 0;

        switch (Seg->Command) {
            case FbPathMoveTo:
            case FbPathLineTo:
            case FbPathClose:
                PointCount = 1;
                break;
            case FbPathQuadraticTo:
                PointCount = 2;
                break;
            case FbPathBezierTo:
                PointCount = 3;
                break;
            case FbPathArcTo:
                PointCount = 3;  /* endpoint + 2 radii */
                break;
        }

        for (UINT32 J = 0; J < PointCount; J++) {
            FLOAT X = Seg->Points[J * 2];
            FLOAT Y = Seg->Points[J * 2 + 1];

            /* Apply transform: x' = A*x + C*y + E, y' = B*x + D*y + F */
            Seg->Points[J * 2] = Transform->A * X + Transform->C * Y + Transform->E;
            Seg->Points[J * 2 + 1] = Transform->B * X + Transform->D * Y + Transform->F;
        }
    }

    Path->BoundsDirty = TRUE;
    Path->FlattenedDirty = TRUE;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPath_GetPoints(
    IFramebuffer2DPath *This,
    FB_POINT *Points,
    UINT32 MaxPoints,
    UINT32 *NumPoints
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    if (Path->FlattenedDirty) {
        FbPath_FlattenPath(Path);
    }

    if (NumPoints != NULL) {
        *NumPoints = Path->FlattenedCount;
    }

    if (Points == NULL) {
        /* Just return count */
        return S_OK;
    }

    if (MaxPoints < Path->FlattenedCount) {
        return E_OUTOFMEMORY;
    }

    /* Copy flattened points */
    for (UINT32 I = 0; I < Path->FlattenedCount; I++) {
        Points[I] = Path->FlattenedPoints[I];
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbPath_GetPathData(
    IFramebuffer2DPath *This,
    FB_PATH_COMMAND *Commands,
    FB_POINT *Points,
    UINT32 MaxPoints,
    UINT32 *NumPoints
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)This;

    if (NumPoints != NULL) {
        *NumPoints = Path->SegmentCount;
    }

    if (Commands == NULL || Points == NULL) {
        /* Just return count */
        return S_OK;
    }

    if (MaxPoints < Path->SegmentCount) {
        return E_OUTOFMEMORY;
    }

    /* Copy segment data */
    UINT32 PointIndex = 0;
    for (UINT32 I = 0; I < Path->SegmentCount; I++) {
        FB_PATH_SEGMENT *Seg = &Path->Segments[I];
        Commands[I] = Seg->Command;

        UINT32 PointCount = 0;
        switch (Seg->Command) {
            case FbPathMoveTo:
            case FbPathLineTo:
            case FbPathClose:
                PointCount = 1;
                break;
            case FbPathQuadraticTo:
                PointCount = 2;
                break;
            case FbPathBezierTo:
            case FbPathArcTo:
                PointCount = 3;
                break;
        }

        for (UINT32 J = 0; J < PointCount; J++) {
            Points[PointIndex].X = (INT32)Seg->Points[J * 2];
            Points[PointIndex].Y = (INT32)Seg->Points[J * 2 + 1];
            PointIndex++;
        }
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Helper Implementations                                         */
/* --------------------------------------------------------------- */

static VOID
FbPath_UpdateBounds(
    FB_PATH_IMPL *Path
    )
{
    if (Path->SegmentCount == 0) {
        Path->Bounds.X = 0;
        Path->Bounds.Y = 0;
        Path->Bounds.Width = 0;
        Path->Bounds.Height = 0;
        Path->BoundsDirty = FALSE;
        return;
    }

    FLOAT MinX = 1e10f, MinY = 1e10f;
    FLOAT MaxX = -1e10f, MaxY = -1e10f;

    for (UINT32 I = 0; I < Path->SegmentCount; I++) {
        FB_PATH_SEGMENT *Seg = &Path->Segments[I];
        UINT32 PointCount = 0;

        switch (Seg->Command) {
            case FbPathMoveTo:
            case FbPathLineTo:
            case FbPathClose:
                PointCount = 1;
                break;
            case FbPathQuadraticTo:
                PointCount = 2;
                break;
            case FbPathBezierTo:
            case FbPathArcTo:
                PointCount = 3;
                break;
        }

        for (UINT32 J = 0; J < PointCount; J++) {
            FLOAT X = Seg->Points[J * 2];
            FLOAT Y = Seg->Points[J * 2 + 1];

            if (X < MinX) MinX = X;
            if (X > MaxX) MaxX = X;
            if (Y < MinY) MinY = Y;
            if (Y > MaxY) MaxY = Y;
        }
    }

    Path->Bounds.X = (INT32)MinX;
    Path->Bounds.Y = (INT32)MinY;
    Path->Bounds.Width = (UINT32)(MaxX - MinX);
    Path->Bounds.Height = (UINT32)(MaxY - MinY);
    Path->BoundsDirty = FALSE;
}

/* Allocate/expand flattened points array */
static HRESULT
FbPath_EnsureFlattenedCapacity(
    FB_PATH_IMPL *Path,
    UINT32 AdditionalPoints
    )
{
    UINT32 RequiredCapacity = Path->FlattenedCount + AdditionalPoints;

    /* Round up to nearest 64 */
    UINT32 NewCapacity = (RequiredCapacity + 63) & ~63;

    if (Path->FlattenedPoints == NULL) {
        Path->FlattenedPoints = (FB_POINT *)AnxAllocate(NewCapacity * sizeof(FB_POINT));
        if (Path->FlattenedPoints == NULL) {
            return E_OUTOFMEMORY;
        }
    } else {
        FB_POINT *NewPoints = (FB_POINT *)AnxRealloc(
            Path->FlattenedPoints,
            NewCapacity * sizeof(FB_POINT));
        if (NewPoints == NULL) {
            return E_OUTOFMEMORY;
        }
        Path->FlattenedPoints = NewPoints;
    }

    return S_OK;
}

/* Add point to flattened array */
static VOID
FbPath_AddFlattenedPoint(
    FB_PATH_IMPL *Path,
    FLOAT X,
    FLOAT Y
    )
{
    if (SUCCEEDED(FbPath_EnsureFlattenedCapacity(Path, 1))) {
        Path->FlattenedPoints[Path->FlattenedCount].X = (INT32)X;
        Path->FlattenedPoints[Path->FlattenedCount].Y = (INT32)Y;
        Path->FlattenedCount++;
    }
}

/* Flatten quadratic Bezier curve to line segments */
static VOID
FbPath_FlattenQuadraticBezier(
    FB_PATH_IMPL *Path,
    FLOAT X0, FLOAT Y0,
    FLOAT X1, FLOAT Y1,
    FLOAT X2, FLOAT Y2
    )
{
    /* Use 16 segments for now - should be adaptive based on curvature */
    CONST UINT32 Segments = 16;

    for (UINT32 I = 1; I <= Segments; I++) {
        FLOAT T = (FLOAT)I / (FLOAT)Segments;
        FLOAT T1 = 1.0f - T;

        /* Quadratic Bezier formula: P = (1-t)^2*P0 + 2*(1-t)*t*P1 + t^2*P2 */
        FLOAT X = T1 * T1 * X0 + 2.0f * T1 * T * X1 + T * T * X2;
        FLOAT Y = T1 * T1 * Y0 + 2.0f * T1 * T * Y1 + T * T * Y2;

        FbPath_AddFlattenedPoint(Path, X, Y);
    }
}

/* Flatten cubic Bezier curve to line segments */
static VOID
FbPath_FlattenCubicBezier(
    FB_PATH_IMPL *Path,
    FLOAT X0, FLOAT Y0,
    FLOAT X1, FLOAT Y1,
    FLOAT X2, FLOAT Y2,
    FLOAT X3, FLOAT Y3
    )
{
    /* Use 20 segments for cubic curves */
    CONST UINT32 Segments = 20;

    for (UINT32 I = 1; I <= Segments; I++) {
        FLOAT T = (FLOAT)I / (FLOAT)Segments;
        FLOAT T1 = 1.0f - T;

        /* Cubic Bezier formula */
        FLOAT X = T1*T1*T1*X0 + 3.0f*T1*T1*T*X1 + 3.0f*T1*T*T*X2 + T*T*T*X3;
        FLOAT Y = T1*T1*T1*Y0 + 3.0f*T1*T1*T*Y1 + 3.0f*T1*T*T*Y2 + T*T*T*Y3;

        FbPath_AddFlattenedPoint(Path, X, Y);
    }
}

/* Flatten entire path to line segments */
static VOID
FbPath_FlattenPath(
    FB_PATH_IMPL *Path
    )
{
    /* Clear existing flattened data */
    Path->FlattenedCount = 0;

    FLOAT CurrentX = 0.0f, CurrentY = 0.0f;

    for (UINT32 I = 0; I < Path->SegmentCount; I++) {
        FB_PATH_SEGMENT *Seg = &Path->Segments[I];

        switch (Seg->Command) {
            case FbPathMoveTo:
                CurrentX = Seg->Points[0];
                CurrentY = Seg->Points[1];
                FbPath_AddFlattenedPoint(Path, CurrentX, CurrentY);
                break;

            case FbPathLineTo:
                CurrentX = Seg->Points[0];
                CurrentY = Seg->Points[1];
                FbPath_AddFlattenedPoint(Path, CurrentX, CurrentY);
                break;

            case FbPathQuadraticTo:
                FbPath_FlattenQuadraticBezier(Path,
                    CurrentX, CurrentY,
                    Seg->Points[0], Seg->Points[1],
                    Seg->Points[2], Seg->Points[3]);
                CurrentX = Seg->Points[2];
                CurrentY = Seg->Points[3];
                break;

            case FbPathBezierTo:
                FbPath_FlattenCubicBezier(Path,
                    CurrentX, CurrentY,
                    Seg->Points[0], Seg->Points[1],
                    Seg->Points[2], Seg->Points[3],
                    Seg->Points[4], Seg->Points[5]);
                CurrentX = Seg->Points[4];
                CurrentY = Seg->Points[5];
                break;

            case FbPathClose:
                /* Line back to start point is implicit in polygon filling */
                break;

            case FbPathArcTo:
                /* Arc is already converted to cubic Bezier curves by ArcTo method */
                CurrentX = Seg->Points[0];
                CurrentY = Seg->Points[1];
                FbPath_AddFlattenedPoint(Path, CurrentX, CurrentY);
                break;
        }
    }

    Path->FlattenedDirty = FALSE;
}

/* --------------------------------------------------------------- */
/*  VTable                                                         */
/* --------------------------------------------------------------- */

static IFramebuffer2DPathVtbl FbPath_Vtbl = {
    /* IUnknown */
    FbPath_QueryInterface,
    FbPath_AddRef,
    FbPath_Release,

    /* IFramebuffer2DPath */
    FbPath_BeginFigure,
    FbPath_CloseFigure,
    FbPath_MoveTo,
    FbPath_LineTo,
    FbPath_QuadraticBezierTo,
    FbPath_CubicBezierTo,
    FbPath_ArcTo,
    FbPath_AddRectangle,
    FbPath_AddCircle,
    FbPath_AddEllipse,
    FbPath_Reset,
    FbPath_GetBounds,
    FbPath_GetPointCount,
    FbPath_Transform,
    FbPath_GetPoints,
    FbPath_GetPathData,
};

/* --------------------------------------------------------------- */
/*  Factory Function                                               */
/* --------------------------------------------------------------- */

IFramebuffer2DPath *
FbCreate2DPath(
    VOID
    )
{
    FB_PATH_IMPL *Path = (FB_PATH_IMPL *)AnxAllocate(sizeof(FB_PATH_IMPL));
    if (Path == NULL) {
        return NULL;
    }

    AnxZeroMemory(Path, sizeof(FB_PATH_IMPL));

    Path->Base.lpVtbl = &FbPath_Vtbl;
    Path->RefCount = 1;
    Path->BoundsDirty = TRUE;
    Path->FlattenedDirty = TRUE;

    return (IFramebuffer2DPath *)Path;
}
