/** @file
  NUX Framebuffer Text Rendering

  Character rendering using embedded bitmap fonts. Renders individual
  characters at specified pixel coordinates.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <nux/framebuffer/internal.h>
#include <nux/framebuffer/font.h>

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
  )
{
  VOID *Data = fontdata + Char * 16;

  FramebufferBlt (X, Y, Color, Data, 8, 16);
}
