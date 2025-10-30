/** @file
  NUX Framebuffer Font Data

  Font data structures and font selection for framebuffer text rendering.
  Supports multiple embedded fonts: 8x8, 8x14, 8x16 (VGA), and scrawl.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __nux_framebuffer_font_h__
#define __nux_framebuffer_font_h__

#include <stdint.h>

//
// Font selection
//
#define S_FONT 0        // 8x8 font
#define T_FONT 1        // 8x14 font
#define SCRAWL_FONT 2   // Scrawl font
#define O_FONT 3        // 8x16 VGA font (default)

#ifndef FBFONT
#define FBFONT O_FONT
#endif

//
// Font data (defined in selected font file)
//
extern UINT8 fontdata[];

#endif // __FONT_H__
