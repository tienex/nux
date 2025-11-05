/*++
    Module Name:

        cursorzone.h

    Abstract:

        Cursor zone manager for automatic cursor changes based on mouse position.

        Allows defining regions (using paths) where specific cursors appear when
        the mouse enters them. Useful for UI elements like buttons, text areas,
        resize handles, etc.

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
typedef struct IFramebufferScreen IFramebufferScreen;
typedef struct IFramebufferCursor IFramebufferCursor;
typedef struct IFramebuffer2DPath IFramebuffer2DPath;

/* --------------------------------------------------------------- */
/*  Zone Definitions                                               */
/* --------------------------------------------------------------- */

/* Zone identifier (returned by AddZone) */
typedef UINT32 FB_ZONE_ID;

#define FB_ZONE_INVALID  ((FB_ZONE_ID)0)

/* Zone priority - higher values checked first for hit testing */
typedef INT32 FB_ZONE_PRIORITY;

#define FB_ZONE_PRIORITY_LOWEST     -1000
#define FB_ZONE_PRIORITY_LOW        -100
#define FB_ZONE_PRIORITY_NORMAL     0
#define FB_ZONE_PRIORITY_HIGH       100
#define FB_ZONE_PRIORITY_HIGHEST    1000

/* Zone flags */
typedef enum _FB_ZONE_FLAGS {
    FbZoneFlagNone          = 0x00,
    FbZoneFlagEnabled       = 0x01,  /* Zone is active */
    FbZoneFlagVisible       = 0x02,  /* Zone path is visible (for debugging) */
    FbZoneFlagInverted      = 0x04,  /* Cursor applies outside the path */
} FB_ZONE_FLAGS;

/* Zone descriptor */
typedef struct _FB_ZONE_DESC {
    FB_ZONE_ID          Id;          /* Zone identifier */
    IFramebuffer2DPath  *Path;       /* Region shape */
    IFramebufferCursor  *Cursor;     /* Cursor to show in this region */
    FB_ZONE_PRIORITY    Priority;    /* Hit test priority */
    FB_ZONE_FLAGS       Flags;       /* Zone flags */
    VOID                *UserData;   /* Application-defined data */
} FB_ZONE_DESC;

/* --------------------------------------------------------------- */
/*  IFramebufferCursorZone - Cursor Zone Manager                   */
/* --------------------------------------------------------------- */

/*
 * Manages cursor zones - regions where specific cursors appear.
 * Automatically changes the screen cursor based on mouse position.
 *
 * Usage pattern:
 * 1. Create zone manager: FbCreateCursorZoneManager(screen)
 * 2. Define regions with paths: AddZone(path, cursor, priority)
 * 3. On mouse move: UpdateCursor(x, y) or HitTest(x, y)
 * 4. Zones checked in priority order (highest first)
 */

#define ANX_IID_IFramebufferCursorZone "FB000022-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferCursorZone,
    0xFB000022, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferCursorZone, IUnknown,
    IID_IFramebufferCursorZone, ANX_IID_IFramebufferCursorZone)

    /* ----------------------------------------------------------------- */
    /* Zone Management                                                   */
    /* ----------------------------------------------------------------- */

    /* Add a cursor zone
     * Returns zone ID for later reference
     * Path and Cursor are AddRef'd - caller can release after adding
     */
    ANX_IFACE_METHOD(HRESULT, AddZone, (
        IN IFramebuffer2DPath *Path,
        IN IFramebufferCursor *Cursor,
        IN FB_ZONE_PRIORITY Priority,
        IN FB_ZONE_FLAGS Flags,
        IN VOID *UserData,
        OUT FB_ZONE_ID *ZoneId))

    /* Remove a zone by ID */
    ANX_IFACE_METHOD(HRESULT, RemoveZone, (
        IN FB_ZONE_ID ZoneId))

    /* Remove all zones */
    ANX_IFACE_METHOD(HRESULT, ClearZones, (
        VOID))

    /* Get zone count */
    ANX_IFACE_METHOD(HRESULT, GetZoneCount, (
        OUT UINT32 *Count))

    /* Get zone descriptor by ID */
    ANX_IFACE_METHOD(HRESULT, GetZone, (
        IN FB_ZONE_ID ZoneId,
        OUT FB_ZONE_DESC *Desc))

    /* Update zone (change cursor, priority, flags) */
    ANX_IFACE_METHOD(HRESULT, UpdateZone, (
        IN FB_ZONE_ID ZoneId,
        IN CONST FB_ZONE_DESC *Desc))

    /* Enable/disable a zone */
    ANX_IFACE_METHOD(HRESULT, SetZoneEnabled, (
        IN FB_ZONE_ID ZoneId,
        IN BOOLEAN Enabled))

    /* Change zone priority (affects hit test order) */
    ANX_IFACE_METHOD(HRESULT, SetZonePriority, (
        IN FB_ZONE_ID ZoneId,
        IN FB_ZONE_PRIORITY Priority))

    /* ----------------------------------------------------------------- */
    /* Hit Testing                                                       */
    /* ----------------------------------------------------------------- */

    /* Test which cursor should be displayed at this position
     * Checks zones in priority order, returns first match
     * Returns S_FALSE if no zone matches (uses default cursor)
     */
    ANX_IFACE_METHOD(HRESULT, HitTest, (
        IN INT32 X,
        IN INT32 Y,
        OUT IFramebufferCursor **Cursor,
        OUT FB_ZONE_ID *ZoneId))

    /* Test if point is inside a specific zone */
    ANX_IFACE_METHOD(HRESULT, IsPointInZone, (
        IN FB_ZONE_ID ZoneId,
        IN INT32 X,
        IN INT32 Y,
        OUT BOOLEAN *IsInside))

    /* Get all zones containing the point (sorted by priority) */
    ANX_IFACE_METHOD(HRESULT, GetZonesAtPoint, (
        IN INT32 X,
        IN INT32 Y,
        OUT FB_ZONE_ID *ZoneIds,
        IN UINT32 MaxZones,
        OUT UINT32 *NumZones))

    /* ----------------------------------------------------------------- */
    /* Cursor Management                                                 */
    /* ----------------------------------------------------------------- */

    /* Set default cursor (used when not in any zone) */
    ANX_IFACE_METHOD(HRESULT, SetDefaultCursor, (
        IN IFramebufferCursor *Cursor))

    /* Get default cursor */
    ANX_IFACE_METHOD(HRESULT, GetDefaultCursor, (
        OUT IFramebufferCursor **Cursor))

    /* Update screen cursor based on mouse position
     * Performs hit test and automatically changes screen cursor
     * Returns S_OK if cursor changed, S_FALSE if unchanged
     */
    ANX_IFACE_METHOD(HRESULT, UpdateCursor, (
        IN INT32 X,
        IN INT32 Y))

    /* Get current cursor position */
    ANX_IFACE_METHOD(HRESULT, GetCursorPosition, (
        OUT INT32 *X,
        OUT INT32 *Y))

    /* Set cursor position (also updates cursor based on zones) */
    ANX_IFACE_METHOD(HRESULT, SetCursorPosition, (
        IN INT32 X,
        IN INT32 Y))

    /* ----------------------------------------------------------------- */
    /* Debugging & Visualization                                         */
    /* ----------------------------------------------------------------- */

    /* Render zone boundaries for debugging
     * Draws outlines of all zones using specified color
     */
    ANX_IFACE_METHOD(HRESULT, DebugRenderZones, (
        IN IFramebuffer2DContext *Context,
        IN FB_COLOR Color))

    /* Get statistics */
    ANX_IFACE_METHOD(HRESULT, GetStatistics, (
        OUT UINT32 *TotalZones,
        OUT UINT32 *EnabledZones,
        OUT UINT32 *HitTestCount))

ANX_END_INTERFACE(IFramebufferCursorZone)

/* --------------------------------------------------------------- */
/*  Helper Macros for C (COBJMACROS style)                          */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IFramebufferCursorZone_AddZone(This, Path, Cursor, Pri, Flags, Data, Id) \
    ((This)->lpVtbl->AddZone(This, Path, Cursor, Pri, Flags, Data, Id))
#define IFramebufferCursorZone_RemoveZone(This, Id) \
    ((This)->lpVtbl->RemoveZone(This, Id))
#define IFramebufferCursorZone_ClearZones(This) \
    ((This)->lpVtbl->ClearZones(This))
#define IFramebufferCursorZone_HitTest(This, X, Y, Cursor, Id) \
    ((This)->lpVtbl->HitTest(This, X, Y, Cursor, Id))
#define IFramebufferCursorZone_UpdateCursor(This, X, Y) \
    ((This)->lpVtbl->UpdateCursor(This, X, Y))
#define IFramebufferCursorZone_SetDefaultCursor(This, Cursor) \
    ((This)->lpVtbl->SetDefaultCursor(This, Cursor))
#define IFramebufferCursorZone_SetCursorPosition(This, X, Y) \
    ((This)->lpVtbl->SetCursorPosition(This, X, Y))

#endif /* !__cplusplus */

/* --------------------------------------------------------------- */
/*  Factory Function                                               */
/* --------------------------------------------------------------- */

/*
 * Create a cursor zone manager for a screen.
 * The manager automatically updates the screen's cursor as the mouse moves.
 */
IFramebufferCursorZone *
FbCreateCursorZoneManager(
    IN IFramebufferScreen *Screen
    );

/* --------------------------------------------------------------- */
/*  Usage Example                                                  */
/* --------------------------------------------------------------- */

#if 0  /* Example code */

/* Create zone manager */
IFramebufferCursorZone *zoneManager = FbCreateCursorZoneManager(screen);

/* Create cursors for different areas */
IFramebufferCursor *arrowCursor = FbCreateMonoCursor(&arrowDesc);
IFramebufferCursor *handCursor = FbCreateColorCursor(&handDesc);
IFramebufferCursor *textCursor = FbCreateMonoCursor(&ibeamDesc);

/* Create paths for different UI regions */
IFramebuffer2DPath *buttonPath = FbCreate2DPath();
IFramebuffer2DPath_AddRectangle(buttonPath, 100, 100, 200, 50);  /* Button area */

IFramebuffer2DPath *textPath = FbCreate2DPath();
IFramebuffer2DPath_AddRectangle(textPath, 100, 200, 400, 100);  /* Text area */

/* Add zones with priorities */
FB_ZONE_ID buttonZone, textZone;

IFramebufferCursorZone_AddZone(
    zoneManager,
    buttonPath,
    handCursor,
    FB_ZONE_PRIORITY_HIGH,      /* Buttons have high priority */
    FbZoneFlagEnabled,
    NULL,
    &buttonZone
);

IFramebufferCursorZone_AddZone(
    zoneManager,
    textPath,
    textCursor,
    FB_ZONE_PRIORITY_NORMAL,
    FbZoneFlagEnabled,
    NULL,
    &textZone
);

/* Set default cursor for areas outside zones */
IFramebufferCursorZone_SetDefaultCursor(zoneManager, arrowCursor);

/* On mouse move event: */
void OnMouseMove(INT32 x, INT32 y) {
    /* Automatically updates screen cursor based on position */
    IFramebufferCursorZone_UpdateCursor(zoneManager, x, y);
}

/* Or manual hit testing: */
IFramebufferCursor *cursor;
FB_ZONE_ID zoneId;
HRESULT hr = IFramebufferCursorZone_HitTest(zoneManager, x, y, &cursor, &zoneId);
if (SUCCEEDED(hr)) {
    /* Apply cursor manually if needed */
    IFramebufferScreen_SetCursor(screen, cursor);
    IUnknown_Release((IUnknown *)cursor);
}

/* Cleanup */
IUnknown_Release((IUnknown *)buttonPath);
IUnknown_Release((IUnknown *)textPath);
IUnknown_Release((IUnknown *)arrowCursor);
IUnknown_Release((IUnknown *)handCursor);
IUnknown_Release((IUnknown *)textCursor);
IUnknown_Release((IUnknown *)zoneManager);

#endif /* Example code */
