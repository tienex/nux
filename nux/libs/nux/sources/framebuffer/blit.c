/** @file
  NUX Framebuffer Bitmap Blitting

  Software bitmap blitting to framebuffer. Handles monochrome bitmap
  rendering with bit-per-pixel processing.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <nux/framebuffer/internal.h>

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
  @param[in] Data   Bitmap data (1 bit per pixel).
  @param[in] Width   Bitmap width in pixels.
  @param[in] Height  Bitmap height in pixels.
**/
VOID
FramebufferBlt (
  IN UINT32  X,
  IN UINT32  Y,
  IN UINT32  Color,
  IN VOID    *Data,
  IN UINTN  Width,
  IN UINTN  Height
  )
{
  UINT32 BytesPerPixel = gFbDesc->bpp / 8;
  UINTN HeightRemaining;

  if (gFbDesc->type == FB_INVALID)
    return;

  HeightRemaining = Height;
  while (HeightRemaining)
    {
      UINTN Offset = Y * gFbDesc->pitch + X * BytesPerPixel;
      UINTN WidthRemainingBytes = (Width + 7) / 8;
      UINTN WidthRemaining = Width;

      while (WidthRemainingBytes)
	{
	  UINT8 Byte = *(UINT8 *) Data++;
	  UINTN BitsRemaining = WidthRemaining < 8 ? WidthRemaining : 8;

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
