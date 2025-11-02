/*++
    Module Name:

        cursor.h

    Abstract:

        Cursor interface for mouse cursor support (mono and color).
        Supports hardware cursor when available, with software fallback.

    Environment:

        C and C++ compatible.
--*/

#pragma once

#include <ananke/types.h>
#include <ananke/com.h>
#include <ananke/hresult.h>
#include <ananke/guid.h>
#include <ananke/framebuffer.h>

/* --------------------------------------------------------------- */
/*  Cursor Definitions                                              */
/* --------------------------------------------------------------- */

#define FB_CURSOR_MAX_WIDTH     64
#define FB_CURSOR_MAX_HEIGHT    64
#define FB_CURSOR_MAX_FRAMES    32

typedef enum _FB_CURSOR_TYPE {
    FbCursorMono        = 0,  /* 1-bit monochrome cursor with mask */
    FbCursorColor       = 1,  /* Full color cursor with alpha */
    FbCursorAnimated    = 2,  /* Animated cursor (multiple frames) */
} FB_CURSOR_TYPE;

typedef struct _FB_CURSOR_DESC {
    FB_CURSOR_TYPE  Type;
    UINT32          Width;          /* Cursor width in pixels */
    UINT32          Height;         /* Cursor height in pixels */
    INT32           HotSpotX;       /* Hot spot X coordinate */
    INT32           HotSpotY;       /* Hot spot Y coordinate */
    UINT32          FrameCount;     /* Number of frames (for animated) */
    UINT32          CurrentFrame;   /* Current frame index (for animated) */
} FB_CURSOR_DESC;

/* Animated cursor frame */
typedef struct _FB_CURSOR_FRAME {
    UINT8           *Data;          /* Frame data (RGBA or mono masks) */
    UINT32          DisplayTime;    /* Display time in milliseconds */
} FB_CURSOR_FRAME;

/* --------------------------------------------------------------- */
/*  Cursor Set Descriptors (for SetMonoCursor, SetColorCursor, etc) */
/* --------------------------------------------------------------- */

/* Monochrome cursor descriptor */
typedef struct _FB_MONO_CURSOR_DESC {
    CONST UINT8     *AndMask;       /* AND mask (1 = use pixel, 0 = transparent) */
    CONST UINT8     *XorMask;       /* XOR mask (1 = invert, 0 = black) */
    UINT32          Width;          /* Cursor width in pixels */
    UINT32          Height;         /* Cursor height in pixels */
    INT32           HotSpotX;       /* Hot spot X coordinate */
    INT32           HotSpotY;       /* Hot spot Y coordinate */
} FB_MONO_CURSOR_DESC;

/* Color cursor descriptor */
typedef struct _FB_COLOR_CURSOR_DESC {
    CONST UINT8     *Data;          /* RGBA data (4 bytes per pixel) */
    UINT32          Width;          /* Cursor width in pixels */
    UINT32          Height;         /* Cursor height in pixels */
    INT32           HotSpotX;       /* Hot spot X coordinate */
    INT32           HotSpotY;       /* Hot spot Y coordinate */
} FB_COLOR_CURSOR_DESC;

/* Animated cursor descriptor */
typedef struct _FB_ANIMATED_CURSOR_DESC {
    CONST FB_CURSOR_FRAME *Frames;  /* Array of animation frames */
    UINT32          FrameCount;     /* Number of frames */
    UINT32          Width;          /* Cursor width in pixels */
    UINT32          Height;         /* Cursor height in pixels */
    INT32           HotSpotX;       /* Hot spot X coordinate */
    INT32           HotSpotY;       /* Hot spot Y coordinate */
    FB_CURSOR_TYPE  FrameType;      /* Type of frames (mono or color) */
} FB_ANIMATED_CURSOR_DESC;

/* --------------------------------------------------------------- */
/*  IFramebufferTimerSink - Timer callback sink interface           */
/* --------------------------------------------------------------- */

/*
 * Timer sink interface for receiving timer callbacks.
 * Objects that need periodic timer notifications implement this interface
 * and register with IFramebufferTimer using Advise().
 *
 * This follows the COM connection point pattern where:
 * - Timer is the event SOURCE
 * - TimerSink is the event CONSUMER
 */

#define ANX_IID_IFramebufferTimerSink "FB000016-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferTimerSink,
    0xFB000016, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferTimerSink, IUnknown,
    IID_IFramebufferTimerSink, ANX_IID_IFramebufferTimerSink)

    /* Called periodically when the timer fires */
    ANX_IFACE_METHOD(HRESULT, OnTimer, (
        IN VOID *Context))

ANX_END_INTERFACE(IFramebufferTimerSink)

/* --------------------------------------------------------------- */
/*  IFramebufferTimer - Timer service interface                     */
/* --------------------------------------------------------------- */

/*
 * Timer service interface for animated cursor and other periodic tasks.
 * The timer calls OnTimer() on registered sinks at specified intervals.
 *
 * This is the event SOURCE that calls sinks.
 */

#define ANX_IID_IFramebufferTimer "FB000015-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferTimer,
    0xFB000015, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferTimer, IUnknown,
    IID_IFramebufferTimer, ANX_IID_IFramebufferTimer)

    /* Register a sink to receive timer callbacks
     * Returns a cookie that can be used to unregister
     */
    ANX_IFACE_METHOD(HRESULT, Advise, (
        IN IFramebufferTimerSink *Sink,
        IN VOID *Context,
        OUT UINT32 *Cookie))

    /* Unregister a previously registered sink */
    ANX_IFACE_METHOD(HRESULT, Unadvise, (
        IN UINT32 Cookie))

    /* Get timer interval in milliseconds */
    ANX_IFACE_METHOD(HRESULT, GetInterval, (
        OUT UINT32 *Milliseconds))

    /* Set timer interval in milliseconds */
    ANX_IFACE_METHOD(HRESULT, SetInterval, (
        IN UINT32 Milliseconds))

    /* Start the timer */
    ANX_IFACE_METHOD(HRESULT, Start, (
        VOID))

    /* Stop the timer */
    ANX_IFACE_METHOD(HRESULT, Stop, (
        VOID))

ANX_END_INTERFACE(IFramebufferTimer)

/* --------------------------------------------------------------- */
/*  IFramebufferCursor - Cursor data interface                      */
/* --------------------------------------------------------------- */

/*
 * IFramebufferCursor represents cursor appearance data (immutable).
 * This is a pure data interface - implementations decide how to store
 * the cursor image data (could use IFramebufferImage internally, etc).
 *
 * For display control (position, visibility, animation), see IFramebufferScreen.
 */

#define ANX_IID_IFramebufferCursor "FB000014-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferCursor,
    0xFB000014, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferCursor, IUnknown,
    IID_IFramebufferCursor, ANX_IID_IFramebufferCursor)

    /* Get cursor metadata (type, dimensions, hotspot, frame count) */
    ANX_IFACE_METHOD(HRESULT, GetDescriptor, (
        OUT FB_CURSOR_DESC *Descriptor))

    /* Get cursor type (mono, color, animated) */
    ANX_IFACE_METHOD(HRESULT, GetType, (
        OUT FB_CURSOR_TYPE *Type))

    /* Get hotspot coordinates */
    ANX_IFACE_METHOD(HRESULT, GetHotSpot, (
        OUT INT32 *X,
        OUT INT32 *Y))

    /* Get cursor image (for static cursors)
     * Returns the cursor's image. For animated cursors, returns current frame.
     */
    ANX_IFACE_METHOD(HRESULT, GetImage, (
        OUT IFramebufferImage **Image))

    /* Get number of animation frames (1 for static cursors) */
    ANX_IFACE_METHOD(HRESULT, GetFrameCount, (
        OUT UINT32 *Count))

    /* Get a specific animation frame image
     * FrameIndex must be < GetFrameCount()
     */
    ANX_IFACE_METHOD(HRESULT, GetFrame, (
        IN UINT32 FrameIndex,
        OUT IFramebufferImage **Image,
        OUT UINT32 *DisplayTimeMs))

ANX_END_INTERFACE(IFramebufferCursor)

/* --------------------------------------------------------------- */
/*  Helper Macros for C (COBJMACROS style)                          */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IFramebufferCursor_GetDescriptor(This, Desc) \
    ((This)->lpVtbl->GetDescriptor(This, Desc))
#define IFramebufferCursor_GetType(This, Type) \
    ((This)->lpVtbl->GetType(This, Type))
#define IFramebufferCursor_GetHotSpot(This, X, Y) \
    ((This)->lpVtbl->GetHotSpot(This, X, Y))
#define IFramebufferCursor_GetImage(This, Image) \
    ((This)->lpVtbl->GetImage(This, Image))
#define IFramebufferCursor_GetFrameCount(This, Count) \
    ((This)->lpVtbl->GetFrameCount(This, Count))
#define IFramebufferCursor_GetFrame(This, Index, Image, Time) \
    ((This)->lpVtbl->GetFrame(This, Index, Image, Time))

#endif /* !__cplusplus */

/* --------------------------------------------------------------- */
/*  Standard Cursor Shapes                                          */
/* --------------------------------------------------------------- */

/* Standard arrow cursor (monochrome) */
extern CONST UINT8 gStandardArrowCursorAnd[16 * 16 / 8];
extern CONST UINT8 gStandardArrowCursorXor[16 * 16 / 8];

/* Standard I-beam cursor (monochrome) */
extern CONST UINT8 gStandardIBeamCursorAnd[16 * 16 / 8];
extern CONST UINT8 gStandardIBeamCursorXor[16 * 16 / 8];

/* Standard wait/hourglass cursor (monochrome) */
extern CONST UINT8 gStandardWaitCursorAnd[16 * 16 / 8];
extern CONST UINT8 gStandardWaitCursorXor[16 * 16 / 8];

/* Standard crosshair cursor (monochrome) */
extern CONST UINT8 gStandardCrosshairCursorAnd[16 * 16 / 8];
extern CONST UINT8 gStandardCrosshairCursorXor[16 * 16 / 8];

/* --------------------------------------------------------------- */
/*  Cursor Factory Functions                                        */
/* --------------------------------------------------------------- */

/*
 * Create a monochrome cursor from descriptor.
 * Returns an immutable cursor data object.
 */
IFramebufferCursor *
FbCreateMonoCursor(
    IN CONST FB_MONO_CURSOR_DESC *Descriptor
    );

/*
 * Create a color cursor from descriptor.
 * Returns an immutable cursor data object.
 */
IFramebufferCursor *
FbCreateColorCursor(
    IN CONST FB_COLOR_CURSOR_DESC *Descriptor
    );

/*
 * Create an animated cursor from descriptor.
 * Returns an immutable cursor data object.
 */
IFramebufferCursor *
FbCreateAnimatedCursor(
    IN CONST FB_ANIMATED_CURSOR_DESC *Descriptor
    );

/*
 * Create a cursor from an existing image.
 * Returns an immutable cursor data object.
 */
IFramebufferCursor *
FbCreateCursorFromImage(
    IN IFramebufferImage *Image,
    IN INT32 HotSpotX,
    IN INT32 HotSpotY
    );
