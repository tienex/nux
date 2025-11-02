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
/*  IFramebufferCursor - Cursor interface                           */
/* --------------------------------------------------------------- */

#define ANX_IID_IFramebufferCursor "FB000014-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferCursor,
    0xFB000014, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferCursor, IUnknown,
    IID_IFramebufferCursor, ANX_IID_IFramebufferCursor)

    /* Show or hide cursor */
    ANX_IFACE_METHOD(HRESULT, SetVisible, (
        IN BOOLEAN Visible))

    /* Get cursor visibility */
    ANX_IFACE_METHOD(HRESULT, IsVisible, (
        OUT BOOLEAN *Visible))

    /* Set cursor position */
    ANX_IFACE_METHOD(HRESULT, SetPosition, (
        IN INT32 X,
        IN INT32 Y))

    /* Get cursor position */
    ANX_IFACE_METHOD(HRESULT, GetPosition, (
        OUT INT32 *X,
        OUT INT32 *Y))

    /* Set monochrome cursor image using descriptor */
    ANX_IFACE_METHOD(HRESULT, SetMonoCursor, (
        IN CONST FB_MONO_CURSOR_DESC *Descriptor))

    /* Set color cursor image using descriptor */
    ANX_IFACE_METHOD(HRESULT, SetColorCursor, (
        IN CONST FB_COLOR_CURSOR_DESC *Descriptor))

    /* Get cursor descriptor */
    ANX_IFACE_METHOD(HRESULT, GetDescriptor, (
        OUT FB_CURSOR_DESC *Descriptor))

    /* Check if hardware cursor is supported */
    ANX_IFACE_METHOD(HRESULT, IsHardwareCursor, (
        OUT BOOLEAN *IsHardware))

    /* Set animated cursor using descriptor */
    ANX_IFACE_METHOD(HRESULT, SetAnimatedCursor, (
        IN CONST FB_ANIMATED_CURSOR_DESC *Descriptor))

    /* Attach a timer for animation
     * The timer will call OnTimer() to advance frames
     */
    ANX_IFACE_METHOD(HRESULT, AttachTimer, (
        IN IFramebufferTimer *Timer))

    /* Detach the timer (stops animation) */
    ANX_IFACE_METHOD(HRESULT, DetachTimer, (
        VOID))

    /* Manually advance to next frame (for manual animation) */
    ANX_IFACE_METHOD(HRESULT, NextFrame, (
        VOID))

    /* Set current frame index */
    ANX_IFACE_METHOD(HRESULT, SetFrame, (
        IN UINT32 FrameIndex))

ANX_END_INTERFACE(IFramebufferCursor)

/* --------------------------------------------------------------- */
/*  Helper Macros for C (COBJMACROS style)                          */
/* --------------------------------------------------------------- */

#ifndef __cplusplus

#define IFramebufferCursor_SetVisible(This, Visible) \
    ((This)->lpVtbl->SetVisible(This, Visible))
#define IFramebufferCursor_IsVisible(This, Visible) \
    ((This)->lpVtbl->IsVisible(This, Visible))
#define IFramebufferCursor_SetPosition(This, X, Y) \
    ((This)->lpVtbl->SetPosition(This, X, Y))
#define IFramebufferCursor_GetPosition(This, X, Y) \
    ((This)->lpVtbl->GetPosition(This, X, Y))
#define IFramebufferCursor_SetMonoCursor(This, Desc) \
    ((This)->lpVtbl->SetMonoCursor(This, Desc))
#define IFramebufferCursor_SetColorCursor(This, Desc) \
    ((This)->lpVtbl->SetColorCursor(This, Desc))
#define IFramebufferCursor_SetAnimatedCursor(This, Desc) \
    ((This)->lpVtbl->SetAnimatedCursor(This, Desc))
#define IFramebufferCursor_GetDescriptor(This, Desc) \
    ((This)->lpVtbl->GetDescriptor(This, Desc))
#define IFramebufferCursor_IsHardwareCursor(This, IsHw) \
    ((This)->lpVtbl->IsHardwareCursor(This, IsHw))
#define IFramebufferCursor_AttachTimer(This, Timer) \
    ((This)->lpVtbl->AttachTimer(This, Timer))
#define IFramebufferCursor_DetachTimer(This) \
    ((This)->lpVtbl->DetachTimer(This))
#define IFramebufferCursor_NextFrame(This) \
    ((This)->lpVtbl->NextFrame(This))
#define IFramebufferCursor_SetFrame(This, Frame) \
    ((This)->lpVtbl->SetFrame(This, Frame))

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
/*  Cursor Constructor                                              */
/* --------------------------------------------------------------- */

/*
 * Create a cursor for a backend.
 * The cursor is initially hidden with a default arrow shape.
 */
IFramebufferCursor *
FbCreateCursor(
    IN IFramebufferBackend *Backend
    );
