/*++
    Module Name:

        cursorzone.c

    Abstract:

        IFramebufferCursorZone implementation.

        Manages cursor zones for automatic cursor changes based on mouse position.
        Uses ray casting algorithm for point-in-polygon testing.

--*/

#include <ananke/framebuffer/cursorzone.h>
#include <ananke/framebuffer/graphics2d.h>
#include <ananke/framebuffer/screen.h>
#include <ananke/framebuffer/cursor.h>
#include <ananke/framebuffer/com_helpers.h>
#include <ananke/atomics.h>
#include <ananke/hresult.h>

/* --------------------------------------------------------------- */
/*  Constants                                                       */
/* --------------------------------------------------------------- */

#define FB_MAX_ZONES        256
#define FB_MAX_PATH_POINTS  4096

/* --------------------------------------------------------------- */
/*  Zone Entry                                                      */
/* --------------------------------------------------------------- */

typedef struct _FB_ZONE_ENTRY {
    FB_ZONE_ID              Id;
    IFramebuffer2DPath      *Path;
    IFramebufferCursor      *Cursor;
    FB_ZONE_PRIORITY        Priority;
    FB_ZONE_FLAGS           Flags;
    VOID                    *UserData;

    /* Cached flattened points for hit testing */
    FB_POINT                *Points;
    UINT32                  PointCount;
    BOOLEAN                 PointsCached;
} FB_ZONE_ENTRY;

/* --------------------------------------------------------------- */
/*  Cursor Zone Manager Implementation                             */
/* --------------------------------------------------------------- */

typedef struct _FB_CURSORZONE_IMPL {
    IFramebufferCursorZone  Base;
    REFOBJ                  RefCount;

    /* Associated screen */
    IFramebufferScreen      *Screen;

    /* Zones (sorted by priority) */
    FB_ZONE_ENTRY           Zones[FB_MAX_ZONES];
    UINT32                  ZoneCount;
    FB_ZONE_ID              NextZoneId;

    /* Default cursor */
    IFramebufferCursor      *DefaultCursor;

    /* Current state */
    INT32                   CurrentX;
    INT32                   CurrentY;
    IFramebufferCursor      *CurrentCursor;
    FB_ZONE_ID              CurrentZoneId;

    /* Statistics */
    UINT32                  HitTestCount;
} FB_CURSORZONE_IMPL;

/* --------------------------------------------------------------- */
/*  Forward Declarations                                            */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE FbZone_QueryInterface(
    IFramebufferCursorZone *This, REFIID riid, VOID **ppvObject);
static UINT32 STDMETHODCALLTYPE FbZone_AddRef(IFramebufferCursorZone *This);
static UINT32 STDMETHODCALLTYPE FbZone_Release(IFramebufferCursorZone *This);

static HRESULT STDMETHODCALLTYPE FbZone_AddZone(
    IFramebufferCursorZone *This, IFramebuffer2DPath *Path,
    IFramebufferCursor *Cursor, FB_ZONE_PRIORITY Priority,
    FB_ZONE_FLAGS Flags, VOID *UserData, FB_ZONE_ID *ZoneId);
static HRESULT STDMETHODCALLTYPE FbZone_RemoveZone(
    IFramebufferCursorZone *This, FB_ZONE_ID ZoneId);
static HRESULT STDMETHODCALLTYPE FbZone_ClearZones(
    IFramebufferCursorZone *This);
static HRESULT STDMETHODCALLTYPE FbZone_GetZoneCount(
    IFramebufferCursorZone *This, UINT32 *Count);
static HRESULT STDMETHODCALLTYPE FbZone_GetZone(
    IFramebufferCursorZone *This, FB_ZONE_ID ZoneId, FB_ZONE_DESC *Desc);
static HRESULT STDMETHODCALLTYPE FbZone_UpdateZone(
    IFramebufferCursorZone *This, FB_ZONE_ID ZoneId, CONST FB_ZONE_DESC *Desc);
static HRESULT STDMETHODCALLTYPE FbZone_SetZoneEnabled(
    IFramebufferCursorZone *This, FB_ZONE_ID ZoneId, BOOLEAN Enabled);
static HRESULT STDMETHODCALLTYPE FbZone_SetZonePriority(
    IFramebufferCursorZone *This, FB_ZONE_ID ZoneId, FB_ZONE_PRIORITY Priority);

static HRESULT STDMETHODCALLTYPE FbZone_HitTest(
    IFramebufferCursorZone *This, INT32 X, INT32 Y,
    IFramebufferCursor **Cursor, FB_ZONE_ID *ZoneId);
static HRESULT STDMETHODCALLTYPE FbZone_IsPointInZone(
    IFramebufferCursorZone *This, FB_ZONE_ID ZoneId,
    INT32 X, INT32 Y, BOOLEAN *IsInside);
static HRESULT STDMETHODCALLTYPE FbZone_GetZonesAtPoint(
    IFramebufferCursorZone *This, INT32 X, INT32 Y,
    FB_ZONE_ID *ZoneIds, UINT32 MaxZones, UINT32 *NumZones);

static HRESULT STDMETHODCALLTYPE FbZone_SetDefaultCursor(
    IFramebufferCursorZone *This, IFramebufferCursor *Cursor);
static HRESULT STDMETHODCALLTYPE FbZone_GetDefaultCursor(
    IFramebufferCursorZone *This, IFramebufferCursor **Cursor);
static HRESULT STDMETHODCALLTYPE FbZone_UpdateCursor(
    IFramebufferCursorZone *This, INT32 X, INT32 Y);
static HRESULT STDMETHODCALLTYPE FbZone_GetCursorPosition(
    IFramebufferCursorZone *This, INT32 *X, INT32 *Y);
static HRESULT STDMETHODCALLTYPE FbZone_SetCursorPosition(
    IFramebufferCursorZone *This, INT32 X, INT32 Y);

static HRESULT STDMETHODCALLTYPE FbZone_DebugRenderZones(
    IFramebufferCursorZone *This, IFramebuffer2DContext *Context, FB_COLOR Color);
static HRESULT STDMETHODCALLTYPE FbZone_GetStatistics(
    IFramebufferCursorZone *This, UINT32 *TotalZones,
    UINT32 *EnabledZones, UINT32 *HitTestCount);

/* --------------------------------------------------------------- */
/*  VTable                                                          */
/* --------------------------------------------------------------- */

static CONST IFramebufferCursorZoneVtbl gZoneVtbl = {
    .QueryInterface         = FbZone_QueryInterface,
    .AddRef                 = FbZone_AddRef,
    .Release                = FbZone_Release,
    .AddZone                = FbZone_AddZone,
    .RemoveZone             = FbZone_RemoveZone,
    .ClearZones             = FbZone_ClearZones,
    .GetZoneCount           = FbZone_GetZoneCount,
    .GetZone                = FbZone_GetZone,
    .UpdateZone             = FbZone_UpdateZone,
    .SetZoneEnabled         = FbZone_SetZoneEnabled,
    .SetZonePriority        = FbZone_SetZonePriority,
    .HitTest                = FbZone_HitTest,
    .IsPointInZone          = FbZone_IsPointInZone,
    .GetZonesAtPoint        = FbZone_GetZonesAtPoint,
    .SetDefaultCursor       = FbZone_SetDefaultCursor,
    .GetDefaultCursor       = FbZone_GetDefaultCursor,
    .UpdateCursor           = FbZone_UpdateCursor,
    .GetCursorPosition      = FbZone_GetCursorPosition,
    .SetCursorPosition      = FbZone_SetCursorPosition,
    .DebugRenderZones       = FbZone_DebugRenderZones,
    .GetStatistics          = FbZone_GetStatistics,
};

/* --------------------------------------------------------------- */
/*  IUnknown Implementation                                         */
/* --------------------------------------------------------------- */

FB_IMPLEMENT_IUNKNOWN(FbZone, FB_CURSORZONE_IMPL, IFramebufferCursorZone, IID_IFramebufferCursorZone)

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

/* Ray casting algorithm for point-in-polygon test
 * Returns TRUE if point (x, y) is inside the polygon
 */
static BOOLEAN
FbZone_PointInPolygon(
    INT32 X,
    INT32 Y,
    CONST FB_POINT *Points,
    UINT32 PointCount
    )
{
    if (PointCount < 3) {
        return FALSE;
    }

    BOOLEAN Inside = FALSE;
    INT32 X1, Y1, X2, Y2;

    /* Ray casting: cast ray from point to infinity and count intersections */
    for (UINT32 I = 0, J = PointCount - 1; I < PointCount; J = I++) {
        X1 = Points[I].X;
        Y1 = Points[I].Y;
        X2 = Points[J].X;
        Y2 = Points[J].Y;

        /* Check if ray crosses edge */
        if (((Y1 > Y) != (Y2 > Y)) &&
            (X < (X2 - X1) * (Y - Y1) / (Y2 - Y1) + X1)) {
            Inside = !Inside;
        }
    }

    return Inside;
}

/* Cache flattened path points for a zone */
static HRESULT
FbZone_CachePoints(
    FB_ZONE_ENTRY *Zone
    )
{
    HRESULT Hr;

    if (Zone->PointsCached) {
        return S_OK;  /* Already cached */
    }

    if (Zone->Path == NULL) {
        return E_POINTER;
    }

    /* Get point count */
    Hr = IFramebuffer2DPath_GetPointCount(Zone->Path, &Zone->PointCount);
    if (FAILED(Hr)) {
        return Hr;
    }

    if (Zone->PointCount == 0) {
        return S_OK;
    }

    if (Zone->PointCount > FB_MAX_PATH_POINTS) {
        Zone->PointCount = FB_MAX_PATH_POINTS;
    }

    /* Allocate point buffer */
    Zone->Points = (FB_POINT *)ANX_MALLOC(sizeof(FB_POINT) * Zone->PointCount);
    if (Zone->Points == NULL) {
        return E_OUTOFMEMORY;
    }

    /* Get flattened points */
    UINT32 NumPoints;
    Hr = IFramebuffer2DPath_GetPoints(Zone->Path, Zone->Points,
                                      Zone->PointCount, &NumPoints);
    if (FAILED(Hr)) {
        ANX_FREE(Zone->Points);
        Zone->Points = NULL;
        return Hr;
    }

    Zone->PointCount = NumPoints;
    Zone->PointsCached = TRUE;

    return S_OK;
}

/* Free cached points */
static VOID
FbZone_FreeCachedPoints(
    FB_ZONE_ENTRY *Zone
    )
{
    if (Zone->Points != NULL) {
        ANX_FREE(Zone->Points);
        Zone->Points = NULL;
    }
    Zone->PointCount = 0;
    Zone->PointsCached = FALSE;
}

/* Compare function for zone sorting (by priority, descending) */
static INT32
FbZone_ComparePriority(
    CONST VOID *A,
    CONST VOID *B
    )
{
    CONST FB_ZONE_ENTRY *ZoneA = (CONST FB_ZONE_ENTRY *)A;
    CONST FB_ZONE_ENTRY *ZoneB = (CONST FB_ZONE_ENTRY *)B;

    /* Higher priority comes first */
    if (ZoneA->Priority > ZoneB->Priority) return -1;
    if (ZoneA->Priority < ZoneB->Priority) return 1;
    return 0;
}

/* Sort zones by priority */
static VOID
FbZone_SortByPriority(
    FB_CURSORZONE_IMPL *Manager
    )
{
    if (Manager->ZoneCount < 2) {
        return;  /* Already sorted */
    }

    /* Simple bubble sort (zones typically < 100) */
    for (UINT32 I = 0; I < Manager->ZoneCount - 1; I++) {
        for (UINT32 J = I + 1; J < Manager->ZoneCount; J++) {
            if (Manager->Zones[I].Priority < Manager->Zones[J].Priority) {
                /* Swap */
                FB_ZONE_ENTRY Temp = Manager->Zones[I];
                Manager->Zones[I] = Manager->Zones[J];
                Manager->Zones[J] = Temp;
            }
        }
    }
}

/* Find zone by ID */
static FB_ZONE_ENTRY *
FbZone_FindZone(
    FB_CURSORZONE_IMPL *Manager,
    FB_ZONE_ID ZoneId
    )
{
    for (UINT32 I = 0; I < Manager->ZoneCount; I++) {
        if (Manager->Zones[I].Id == ZoneId) {
            return &Manager->Zones[I];
        }
    }
    return NULL;
}

/* --------------------------------------------------------------- */
/*  Zone Management Implementation                                  */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbZone_AddZone(
    IFramebufferCursorZone *This,
    IFramebuffer2DPath *Path,
    IFramebufferCursor *Cursor,
    FB_ZONE_PRIORITY Priority,
    FB_ZONE_FLAGS Flags,
    VOID *UserData,
    FB_ZONE_ID *ZoneId
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;
    FB_ZONE_ENTRY *Zone;

    if (Path == NULL || Cursor == NULL) {
        return E_POINTER;
    }

    if (Manager->ZoneCount >= FB_MAX_ZONES) {
        return E_OUTOFMEMORY;
    }

    /* Add zone */
    Zone = &Manager->Zones[Manager->ZoneCount];
    ANX_MEMSET(Zone, 0, sizeof(FB_ZONE_ENTRY));

    Zone->Id = Manager->NextZoneId++;
    Zone->Path = Path;
    Zone->Cursor = Cursor;
    Zone->Priority = Priority;
    Zone->Flags = Flags;
    Zone->UserData = UserData;

    /* AddRef interfaces */
    IUnknown_AddRef((IUnknown *)Path);
    IUnknown_AddRef((IUnknown *)Cursor);

    /* Cache path points */
    FbZone_CachePoints(Zone);

    Manager->ZoneCount++;

    /* Sort by priority */
    FbZone_SortByPriority(Manager);

    if (ZoneId != NULL) {
        *ZoneId = Zone->Id;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbZone_RemoveZone(
    IFramebufferCursorZone *This,
    FB_ZONE_ID ZoneId
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;
    FB_ZONE_ENTRY *Zone;
    UINT32 Index;

    /* Find zone */
    for (Index = 0; Index < Manager->ZoneCount; Index++) {
        if (Manager->Zones[Index].Id == ZoneId) {
            break;
        }
    }

    if (Index >= Manager->ZoneCount) {
        return E_INVALIDARG;
    }

    Zone = &Manager->Zones[Index];

    /* Release interfaces */
    if (Zone->Path != NULL) {
        IUnknown_Release((IUnknown *)Zone->Path);
    }
    if (Zone->Cursor != NULL) {
        IUnknown_Release((IUnknown *)Zone->Cursor);
    }

    /* Free cached points */
    FbZone_FreeCachedPoints(Zone);

    /* Remove from array (shift remaining zones) */
    for (UINT32 I = Index; I < Manager->ZoneCount - 1; I++) {
        Manager->Zones[I] = Manager->Zones[I + 1];
    }
    Manager->ZoneCount--;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbZone_ClearZones(
    IFramebufferCursorZone *This
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;

    /* Remove all zones */
    for (UINT32 I = 0; I < Manager->ZoneCount; I++) {
        FB_ZONE_ENTRY *Zone = &Manager->Zones[I];

        if (Zone->Path != NULL) {
            IUnknown_Release((IUnknown *)Zone->Path);
        }
        if (Zone->Cursor != NULL) {
            IUnknown_Release((IUnknown *)Zone->Cursor);
        }

        FbZone_FreeCachedPoints(Zone);
    }

    Manager->ZoneCount = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbZone_GetZoneCount(
    IFramebufferCursorZone *This,
    UINT32 *Count
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;

    if (Count == NULL) {
        return E_POINTER;
    }

    *Count = Manager->ZoneCount;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbZone_GetZone(
    IFramebufferCursorZone *This,
    FB_ZONE_ID ZoneId,
    FB_ZONE_DESC *Desc
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;
    FB_ZONE_ENTRY *Zone;

    if (Desc == NULL) {
        return E_POINTER;
    }

    Zone = FbZone_FindZone(Manager, ZoneId);
    if (Zone == NULL) {
        return E_INVALIDARG;
    }

    /* Fill descriptor */
    Desc->Id = Zone->Id;
    Desc->Path = Zone->Path;
    Desc->Cursor = Zone->Cursor;
    Desc->Priority = Zone->Priority;
    Desc->Flags = Zone->Flags;
    Desc->UserData = Zone->UserData;

    /* AddRef interfaces */
    if (Desc->Path != NULL) {
        IUnknown_AddRef((IUnknown *)Desc->Path);
    }
    if (Desc->Cursor != NULL) {
        IUnknown_AddRef((IUnknown *)Desc->Cursor);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbZone_UpdateZone(
    IFramebufferCursorZone *This,
    FB_ZONE_ID ZoneId,
    CONST FB_ZONE_DESC *Desc
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;
    FB_ZONE_ENTRY *Zone;

    if (Desc == NULL) {
        return E_POINTER;
    }

    Zone = FbZone_FindZone(Manager, ZoneId);
    if (Zone == NULL) {
        return E_INVALIDARG;
    }

    /* Update cursor */
    if (Desc->Cursor != NULL && Desc->Cursor != Zone->Cursor) {
        if (Zone->Cursor != NULL) {
            IUnknown_Release((IUnknown *)Zone->Cursor);
        }
        Zone->Cursor = Desc->Cursor;
        IUnknown_AddRef((IUnknown *)Zone->Cursor);
    }

    /* Update priority */
    if (Desc->Priority != Zone->Priority) {
        Zone->Priority = Desc->Priority;
        FbZone_SortByPriority(Manager);
    }

    /* Update flags */
    Zone->Flags = Desc->Flags;
    Zone->UserData = Desc->UserData;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbZone_SetZoneEnabled(
    IFramebufferCursorZone *This,
    FB_ZONE_ID ZoneId,
    BOOLEAN Enabled
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;
    FB_ZONE_ENTRY *Zone;

    Zone = FbZone_FindZone(Manager, ZoneId);
    if (Zone == NULL) {
        return E_INVALIDARG;
    }

    if (Enabled) {
        Zone->Flags |= FbZoneFlagEnabled;
    } else {
        Zone->Flags &= ~FbZoneFlagEnabled;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbZone_SetZonePriority(
    IFramebufferCursorZone *This,
    FB_ZONE_ID ZoneId,
    FB_ZONE_PRIORITY Priority
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;
    FB_ZONE_ENTRY *Zone;

    Zone = FbZone_FindZone(Manager, ZoneId);
    if (Zone == NULL) {
        return E_INVALIDARG;
    }

    Zone->Priority = Priority;
    FbZone_SortByPriority(Manager);

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Hit Testing Implementation                                      */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbZone_HitTest(
    IFramebufferCursorZone *This,
    INT32 X,
    INT32 Y,
    IFramebufferCursor **Cursor,
    FB_ZONE_ID *ZoneId
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;

    if (Cursor == NULL) {
        return E_POINTER;
    }

    Manager->HitTestCount++;

    /* Check zones in priority order (already sorted) */
    for (UINT32 I = 0; I < Manager->ZoneCount; I++) {
        FB_ZONE_ENTRY *Zone = &Manager->Zones[I];

        /* Skip disabled zones */
        if (!(Zone->Flags & FbZoneFlagEnabled)) {
            continue;
        }

        /* Ensure points are cached */
        if (!Zone->PointsCached) {
            FbZone_CachePoints(Zone);
        }

        /* Hit test */
        BOOLEAN Inside = FbZone_PointInPolygon(X, Y, Zone->Points, Zone->PointCount);

        /* Handle inverted zones */
        if (Zone->Flags & FbZoneFlagInverted) {
            Inside = !Inside;
        }

        if (Inside) {
            *Cursor = Zone->Cursor;
            IUnknown_AddRef((IUnknown *)Zone->Cursor);

            if (ZoneId != NULL) {
                *ZoneId = Zone->Id;
            }

            return S_OK;
        }
    }

    /* No zone matched - use default cursor */
    if (Manager->DefaultCursor != NULL) {
        *Cursor = Manager->DefaultCursor;
        IUnknown_AddRef((IUnknown *)Manager->DefaultCursor);

        if (ZoneId != NULL) {
            *ZoneId = FB_ZONE_INVALID;
        }

        return S_FALSE;  /* Using default */
    }

    return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE
FbZone_IsPointInZone(
    IFramebufferCursorZone *This,
    FB_ZONE_ID ZoneId,
    INT32 X,
    INT32 Y,
    BOOLEAN *IsInside
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;
    FB_ZONE_ENTRY *Zone;

    if (IsInside == NULL) {
        return E_POINTER;
    }

    Zone = FbZone_FindZone(Manager, ZoneId);
    if (Zone == NULL) {
        return E_INVALIDARG;
    }

    /* Ensure points are cached */
    if (!Zone->PointsCached) {
        FbZone_CachePoints(Zone);
    }

    *IsInside = FbZone_PointInPolygon(X, Y, Zone->Points, Zone->PointCount);

    if (Zone->Flags & FbZoneFlagInverted) {
        *IsInside = !(*IsInside);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbZone_GetZonesAtPoint(
    IFramebufferCursorZone *This,
    INT32 X,
    INT32 Y,
    FB_ZONE_ID *ZoneIds,
    UINT32 MaxZones,
    UINT32 *NumZones
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;
    UINT32 Count = 0;

    if (ZoneIds == NULL || NumZones == NULL) {
        return E_POINTER;
    }

    /* Find all zones containing point (in priority order) */
    for (UINT32 I = 0; I < Manager->ZoneCount && Count < MaxZones; I++) {
        FB_ZONE_ENTRY *Zone = &Manager->Zones[I];

        if (!(Zone->Flags & FbZoneFlagEnabled)) {
            continue;
        }

        if (!Zone->PointsCached) {
            FbZone_CachePoints(Zone);
        }

        BOOLEAN Inside = FbZone_PointInPolygon(X, Y, Zone->Points, Zone->PointCount);

        if (Zone->Flags & FbZoneFlagInverted) {
            Inside = !Inside;
        }

        if (Inside) {
            ZoneIds[Count++] = Zone->Id;
        }
    }

    *NumZones = Count;
    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Cursor Management Implementation                                */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbZone_SetDefaultCursor(
    IFramebufferCursorZone *This,
    IFramebufferCursor *Cursor
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;

    if (Manager->DefaultCursor != NULL) {
        IUnknown_Release((IUnknown *)Manager->DefaultCursor);
    }

    Manager->DefaultCursor = Cursor;

    if (Cursor != NULL) {
        IUnknown_AddRef((IUnknown *)Cursor);
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbZone_GetDefaultCursor(
    IFramebufferCursorZone *This,
    IFramebufferCursor **Cursor
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;

    if (Cursor == NULL) {
        return E_POINTER;
    }

    *Cursor = Manager->DefaultCursor;

    if (Manager->DefaultCursor != NULL) {
        IUnknown_AddRef((IUnknown *)Manager->DefaultCursor);
        return S_OK;
    }

    return S_FALSE;
}

static HRESULT STDMETHODCALLTYPE
FbZone_UpdateCursor(
    IFramebufferCursorZone *This,
    INT32 X,
    INT32 Y
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;
    IFramebufferCursor *NewCursor = NULL;
    FB_ZONE_ID NewZoneId = FB_ZONE_INVALID;
    HRESULT Hr;

    /* Hit test to find cursor */
    Hr = FbZone_HitTest(This, X, Y, &NewCursor, &NewZoneId);
    if (FAILED(Hr)) {
        return Hr;
    }

    /* Update position */
    Manager->CurrentX = X;
    Manager->CurrentY = Y;

    /* Check if cursor changed */
    if (NewCursor != Manager->CurrentCursor || NewZoneId != Manager->CurrentZoneId) {
        /* Update screen cursor */
        if (Manager->Screen != NULL && NewCursor != NULL) {
            IFramebufferScreen_SetCursor(Manager->Screen, NewCursor);
            IFramebufferScreen_SetCursorPosition(Manager->Screen, X, Y);
        }

        /* Update current state */
        if (Manager->CurrentCursor != NULL) {
            IUnknown_Release((IUnknown *)Manager->CurrentCursor);
        }

        Manager->CurrentCursor = NewCursor;
        Manager->CurrentZoneId = NewZoneId;

        /* NewCursor already has ref from HitTest */

        return S_OK;  /* Cursor changed */
    }

    /* Cursor didn't change, just update position */
    if (Manager->Screen != NULL) {
        IFramebufferScreen_SetCursorPosition(Manager->Screen, X, Y);
    }

    if (NewCursor != NULL) {
        IUnknown_Release((IUnknown *)NewCursor);
    }

    return S_FALSE;  /* Cursor unchanged */
}

static HRESULT STDMETHODCALLTYPE
FbZone_GetCursorPosition(
    IFramebufferCursorZone *This,
    INT32 *X,
    INT32 *Y
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;

    if (X == NULL || Y == NULL) {
        return E_POINTER;
    }

    *X = Manager->CurrentX;
    *Y = Manager->CurrentY;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbZone_SetCursorPosition(
    IFramebufferCursorZone *This,
    INT32 X,
    INT32 Y
    )
{
    /* Set position and update cursor */
    return FbZone_UpdateCursor(This, X, Y);
}

/* --------------------------------------------------------------- */
/*  Debugging & Statistics                                          */
/* --------------------------------------------------------------- */

static HRESULT STDMETHODCALLTYPE
FbZone_DebugRenderZones(
    IFramebufferCursorZone *This,
    IFramebuffer2DContext *Context,
    FB_COLOR Color
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;

    if (Context == NULL) {
        return E_POINTER;
    }

    /* Draw each zone's path */
    for (UINT32 I = 0; I < Manager->ZoneCount; I++) {
        FB_ZONE_ENTRY *Zone = &Manager->Zones[I];

        if (Zone->Path != NULL && (Zone->Flags & FbZoneFlagVisible)) {
            IFramebuffer2DContext_SetStrokeColor(Context, Color);
            IFramebuffer2DContext_StrokePath(Context, Zone->Path);
        }
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
FbZone_GetStatistics(
    IFramebufferCursorZone *This,
    UINT32 *TotalZones,
    UINT32 *EnabledZones,
    UINT32 *HitTestCount
    )
{
    FB_CURSORZONE_IMPL *Manager = (FB_CURSORZONE_IMPL *)This;

    if (TotalZones != NULL) {
        *TotalZones = Manager->ZoneCount;
    }

    if (EnabledZones != NULL) {
        UINT32 Count = 0;
        for (UINT32 I = 0; I < Manager->ZoneCount; I++) {
            if (Manager->Zones[I].Flags & FbZoneFlagEnabled) {
                Count++;
            }
        }
        *EnabledZones = Count;
    }

    if (HitTestCount != NULL) {
        *HitTestCount = Manager->HitTestCount;
    }

    return S_OK;
}

/* --------------------------------------------------------------- */
/*  Factory Function                                               */
/* --------------------------------------------------------------- */

IFramebufferCursorZone *
FbCreateCursorZoneManager(
    IN IFramebufferScreen *Screen
    )
{
    FB_CURSORZONE_IMPL *Manager;

    if (Screen == NULL) {
        return NULL;
    }

    /* Allocate manager */
    Manager = (FB_CURSORZONE_IMPL *)ANX_MALLOC(sizeof(FB_CURSORZONE_IMPL));
    if (Manager == NULL) {
        return NULL;
    }

    ANX_MEMSET(Manager, 0, sizeof(FB_CURSORZONE_IMPL));
    Manager->Base.lpVtbl = &gZoneVtbl;
    Manager->RefCount.RefCount = 1;

    /* Store screen reference */
    IUnknown_AddRef((IUnknown *)Screen);
    Manager->Screen = Screen;

    Manager->NextZoneId = 1;
    Manager->CurrentZoneId = FB_ZONE_INVALID;

    return &Manager->Base;
}
