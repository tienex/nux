/*++
    Module Name:

        pattern.h

    Abstract:

        Pattern objects for tiled drawing.
        Similar to Core Graphics CGPattern.

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
typedef struct IFramebuffer2DContext IFramebuffer2DContext;

/* --------------------------------------------------------------- */
/*  Pattern Types                                                  */
/* --------------------------------------------------------------- */

/* Pattern tiling mode */
typedef enum _FB_PATTERN_TILING {
    FbPatternTilingNoDistortion         = 0,  /* No distortion */
    FbPatternTilingConstantSpacingMinimalDistortion = 1,
    FbPatternTilingConstantSpacing      = 2,
} FB_PATTERN_TILING;

/* Pattern callback for drawing pattern cell */
typedef VOID (CALLBACK *FB_PATTERN_DRAW_CALLBACK)(
    IN VOID *UserData,
    IN IFramebuffer2DContext *Context
    );

/* Pattern descriptor */
typedef struct _FB_PATTERN_DESC {
    UINT32                      Width;          /* Pattern cell width */
    UINT32                      Height;         /* Pattern cell height */
    FLOAT                       XStep;          /* Horizontal spacing */
    FLOAT                       YStep;          /* Vertical spacing */
    FB_PATTERN_TILING           Tiling;         /* Tiling mode */
    BOOLEAN                     IsColored;      /* TRUE if pattern draws in color */
    FB_PATTERN_DRAW_CALLBACK    DrawCallback;   /* Function to draw cell */
    VOID                        *UserData;      /* User data for callback */
} FB_PATTERN_DESC;

/* --------------------------------------------------------------- */
/*  IFramebufferPattern - Pattern Object (like CGPattern)          */
/* --------------------------------------------------------------- */

/*
 * Pattern object for tiled drawing.
 *
 * Similar to CGPattern in Core Graphics. Patterns are used for:
 * - Repeating textures
 * - Custom fill patterns
 * - Hatching and stippling
 * - Procedurally generated tiles
 *
 * Patterns can be colored (draw with their own colors) or
 * uncolored (drawn with current fill/stroke color).
 */

#define ANX_IID_IFramebufferPattern "FB000026-0000-0000-C000-000000000046"
ANX_DEFINE_GUID(IID_IFramebufferPattern,
    0xFB000026, 0x0000, 0x0000,
    0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

ANX_BEGIN_INTERFACE(IFramebufferPattern, IUnknown,
    IID_IFramebufferPattern, ANX_IID_IFramebufferPattern)

    /* Get pattern descriptor */
    ANX_IFACE_METHOD(HRESULT, GetDescriptor, (
        OUT FB_PATTERN_DESC *Descriptor))

    /* Get pattern cell size */
    ANX_IFACE_METHOD(HRESULT, GetCellSize, (
        OUT UINT32 *Width,
        OUT UINT32 *Height))

    /* Get pattern spacing */
    ANX_IFACE_METHOD(HRESULT, GetSpacing, (
        OUT FLOAT *XStep,
        OUT FLOAT *YStep))

    /* Check if pattern is colored */
    ANX_IFACE_METHOD(HRESULT, IsColored, (
        OUT BOOLEAN *Colored))

    /* Draw pattern cell (calls draw callback) */
    ANX_IFACE_METHOD(HRESULT, DrawCell, (
        IN IFramebuffer2DContext *Context))

ANX_END_INTERFACE(IFramebufferPattern)

/* --------------------------------------------------------------- */
/*  Factory Functions                                              */
/* --------------------------------------------------------------- */

/*
 * Create pattern from descriptor.
 *
 * Similar to CGPatternCreate().
 *
 * Parameters:
 *   Descriptor - Pattern configuration
 *
 * Returns:
 *   New pattern object, or NULL on failure
 *
 * Example:
 *   FB_PATTERN_DESC desc = {
 *       .Width = 16,
 *       .Height = 16,
 *       .XStep = 16.0f,
 *       .YStep = 16.0f,
 *       .Tiling = FbPatternTilingConstantSpacing,
 *       .IsColored = TRUE,
 *       .DrawCallback = MyDrawFunc,
 *       .UserData = myData
 *   };
 *   IFramebufferPattern *pattern = FbCreatePattern(&desc);
 */
IFramebufferPattern *
FbCreatePattern(
    IN CONST FB_PATTERN_DESC *Descriptor
    );

/*
 * Create pattern from image.
 *
 * Convenience function for creating a simple image-based pattern.
 */
IFramebufferPattern *
FbCreatePatternFromImage(
    IN IFramebufferImage *Image,
    IN FLOAT XStep,
    IN FLOAT YStep
    );
