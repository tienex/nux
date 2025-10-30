/** @file
  NUX Framebuffer Internal Definitions

  Internal header for framebuffer subsystem. Contains shared global
  variables, constants, and internal function prototypes used across
  fb_core.c, fb_blit.c, fb_text.c, and fb_console.c.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_framebuffer_internal_h__
#define __nux_framebuffer_internal_h__

#include <nux/internal.h>
#include <nux/locks.h>
#include <framebuffer.h>

//
// Global framebuffer state
//
extern FRAMEBUFFER_DESC *gFbDesc;
extern SPINLOCK gFbLock;

//
// Console state
//
#define FB_ROWCHARS 79
extern VOLATILE INT32 gFbScreenColumn;
extern VOLATILE INT32 gFbX;
extern VOLATILE INT32 gFbY;
extern INT32 gFbScreenCols;
extern INT32 gFbScreenRows;

//
// Internal function prototypes
//

/**
  Draw character at specific position.

  Renders a character at the specified pixel coordinates using
  the embedded font.

  @param[in] X      X coordinate in pixels.
  @param[in] Y      Y coordinate in pixels.
  @param[in] Color  Character color.
  @param[in] Char   Character to draw.
**/
VOID
FramebufferPutCharXY (
  IN UINT32  X,
  IN UINT32  Y,
  IN UINT32  Color,
  IN UINT8   Char
  );

#endif // __FB_INTERNAL_H__
