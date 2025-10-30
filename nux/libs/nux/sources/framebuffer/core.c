/** @file
  NUX Framebuffer Core

  Core framebuffer initialization and basic operations. Manages
  framebuffer descriptor and display state.

  Copyright (C) 2019 Gianluca Guida, glguida@tlbflush.org

  SPDX-License-Identifier: BSD-2-Clause
**/

#include <nux/framebuffer/internal.h>
#include <assert.h>
#include <string.h>

//
// Global framebuffer state
//
struct fbdesc *gFbDesc;
lock_t gFbLock;

//
// Console state
//
volatile INT32 gFbScreenColumn = 0;
volatile INT32 gFbX = 0;
volatile INT32 gFbY = 0;
INT32 gFbScreenCols;
INT32 gFbScreenRows;

/**
  Initialize framebuffer.

  Sets up framebuffer descriptor and clears display. Only RGB
  framebuffers are supported.

  @param[in] Desc  Framebuffer descriptor.

  @retval 1  Framebuffer initialized successfully.
  @retval 0  Invalid framebuffer type.
**/
INT32
FramebufferInitialize (
  IN struct fbdesc  *Desc
  )
{
  if (Desc->type == FB_INVALID)
    return 0;

  assert (Desc->type == FB_RGB);
  gFbDesc = Desc;
  memset ((void *) (UINTN) gFbDesc->addr, 0, gFbDesc->size);
  FramebufferReset ();
  return 1;
}

/**
  Convert RGB to framebuffer color.

  Converts RGB color components to framebuffer pixel format.
  Currently returns white (0xffffff) regardless of input.

  @param[in] Red    Red component.
  @param[in] Green  Green component.
  @param[in] Blue   Blue component.

  @return Framebuffer color value.
**/
UINT32
FramebufferColor (
  IN UINT32  Red,
  IN UINT32  Green,
  IN UINT32  Blue
  )
{
  /* XXX: Use rgb masks. */
  return 0xffffff;
}

/**
  Reset framebuffer text display state.

  Initializes framebuffer lock and calculates screen layout
  based on framebuffer dimensions.
**/
VOID
FramebufferReset (
  VOID
  )
{
  spinlock_init (&gFbLock);
  gFbScreenCols = (gFbDesc->width / 8) / (FB_ROWCHARS + 1);
  gFbScreenCols = gFbScreenCols == 0 ? 1 : gFbScreenCols;
  gFbScreenRows = gFbDesc->height / 16;
}

//
// Legacy Function Wrappers (for backward compatibility)
//

/** @deprecated Use FramebufferInitialize instead **/
int framebuffer_init (struct fbdesc *desc) {
  return FramebufferInitialize (desc);
}

/** @deprecated Use FramebufferColor instead **/
UINT32 framebuffer_color (unsigned r, unsigned g, unsigned b) {
  return FramebufferColor (r, g, b);
}

/** @deprecated Use FramebufferReset instead **/
void framebuffer_reset (void) {
  FramebufferReset ();
}

// Legacy global variable aliases
static struct fbdesc *fbdesc __attribute__((alias("gFbDesc")));
static lock_t fblock __attribute__((alias("gFbLock")));
static volatile int fb_sc __attribute__((alias("gFbScreenColumn")));
static volatile int fb_x __attribute__((alias("gFbX")));
static volatile int fb_y __attribute__((alias("gFbY")));
static int fb_scrcols __attribute__((alias("gFbScreenCols")));
static int fb_scrrows __attribute__((alias("gFbScreenRows")));
