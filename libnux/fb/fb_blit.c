/** @file
  NUX Framebuffer Bitmap Blitting

  Software bitmap blitting to framebuffer. Handles monochrome bitmap
  rendering with bit-per-pixel processing.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "fb_internal.h"

/**
  Blit bitmap to framebuffer.

  Draws a monochrome bitmap to framebuffer at specified location
  with given color. Each bit in the bitmap data represents one
  pixel (1=colored, 0=black).

  Possibly the slowest software blitter.

  XXX: Doesn't check boundaries.
  XXX: COMPLETE REWRITE CLEARLY NEEDED.

  @param[in] X       X coordinate in pixels.
  @param[in] Y       Y coordinate in pixels.
  @param[in] Color   Foreground color.
  @param[in] pData   Bitmap data (1 bit per pixel).
  @param[in] Width   Bitmap width in pixels.
  @param[in] Height  Bitmap height in pixels.
**/
VOID
FramebufferBlt (
  IN UINT32  X,
  IN UINT32  Y,
  IN UINT32  Color,
  IN VOID    *pData,
  IN size_t  Width,
  IN size_t  Height
  )
{
  UINT32 BytesPerPixel = gFbDesc->bpp / 8;
  size_t HeightRemaining;

  if (gFbDesc->type == FB_INVALID)
    return;

  HeightRemaining = Height;
  while (HeightRemaining)
    {
      size_t Offset = Y * gFbDesc->pitch + X * BytesPerPixel;
      size_t WidthRemainingBytes = (Width + 7) / 8;
      size_t WidthRemaining = Width;

      while (WidthRemainingBytes)
	{
	  UINT8 Byte = *(UINT8 *) pData++;
	  size_t BitsRemaining = WidthRemaining < 8 ? WidthRemaining : 8;

	  while (BitsRemaining)
	    {
	      if (Byte & 0x80)
		*(volatile UINT32 *) (UINTN) (gFbDesc->addr + Offset) =
		  Color;
	      else
		*(volatile UINT32 *) (UINTN) (gFbDesc->addr + Offset) = 0;

	      Offset += BytesPerPixel;
	      Byte <<= 1;
	      BitsRemaining--;
	      WidthRemaining--;
	    }

	  WidthRemainingBytes--;
	}
      Y += 1;
      HeightRemaining--;
    }
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use FramebufferBlt instead **/
void framebuffer_blt (unsigned x, unsigned y, uint32_t color,
		 void *data, size_t width, size_t height) {
  FramebufferBlt (x, y, color, data, width, height);
}
