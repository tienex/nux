/** @file
  NUX Framebuffer Console

  Multi-column scrolling text console. Manages cursor position,
  line wrapping, and column scrolling for text output.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include "fb_internal.h"

/**
  Blank a line of text.

  Clears a row of characters by drawing null characters.

  @param[in] Column     Screen column.
  @param[in] X          Starting character position in row.
  @param[in] Y          Row number.
  @param[in] CharCount  Number of characters to blank.
**/
static VOID
BlankLine (
  IN UINT32  Column,
  IN UINT32  X,
  IN UINT32  Y,
  IN UINT32  CharCount
  )
{
  UINT32 i;

  for (i = 0; i < CharCount; i++)
    {
      FramebufferPutCharXY (8 * (Column * (FB_ROWCHARS + 1) + X + i), 16 * Y, 0,
			   '\0');
    }
}

/**
  Advance to new line.

  Moves cursor to beginning of next line, scrolling to next
  column if needed.
**/
static VOID
Newline (
  VOID
  )
{
  INT32 ScreenColumn, X, Y;

  spinlock (&gFbLock);
  gFbX = 0;
  gFbY += 1;
  if (gFbY >= gFbScreenRows)
    {
      gFbY = 0;
      gFbScreenColumn = (gFbScreenColumn + 1) % gFbScreenCols;
    }

  X = gFbX;
  Y = gFbY;
  ScreenColumn = gFbScreenColumn;
  spinunlock (&gFbLock);

  BlankLine (ScreenColumn, X, Y, FB_ROWCHARS);
}

/**
  Allocate next character position.

  Returns the screen position for the next character to be
  drawn and advances the cursor.

  @param[out] pScreenColumn  Pointer to receive screen column.
  @param[out] pX             Pointer to receive X position.
  @param[out] pY             Pointer to receive Y position.
**/
static VOID
AllocateCharacter (
  OUT INT32  *pScreenColumn,
  OUT INT32  *pX,
  OUT INT32  *pY
  )
{
  INT32 ScreenColumn, X, Y;

  spinlock (&gFbLock);
  if (gFbX >= FB_ROWCHARS)
    {
      gFbX = 0;
      gFbY += 1;
      if (gFbY >= gFbScreenRows)
	{
	  gFbY = 0;
	  gFbScreenColumn = (gFbScreenColumn + 1) % gFbScreenCols;
	}
      BlankLine (gFbScreenColumn, gFbX, gFbY, FB_ROWCHARS);
    }
  ScreenColumn = gFbScreenColumn;
  X = gFbX;
  Y = gFbY;
  gFbX++;
  spinunlock (&gFbLock);

  *pScreenColumn = ScreenColumn;
  *pX = X;
  *pY = Y;
}

/**
  Output character to framebuffer.

  Draws a character at the current cursor position with
  specified color and advances cursor. Handles newlines.

  @param[in] Char   Character to output.
  @param[in] Color  Character color.

  @return The character output.
**/
INT32
FramebufferPutChar (
  IN INT32   Char,
  IN UINT32  Color
  )
{
  INT32 X, Y, ScreenColumn;
  UINT32 PixelX, PixelY;
  UINT8 CharByte = (UINT8) Char;


  if (CharByte == '\n')
    {
      Newline ();
      return Char;
    }

  AllocateCharacter (&ScreenColumn, &X, &Y);

  PixelX = 8 * (ScreenColumn * (FB_ROWCHARS + 1) + X);
  PixelY = Y * 16;
  FramebufferPutCharXY (PixelX, PixelY, Color, CharByte);
  return Char;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use FramebufferPutChar instead **/
int framebuffer_putc (int ch, uint32_t color) {
  return FramebufferPutChar (ch, color);
}
