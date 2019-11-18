#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <stdint.h>

/*
  Framebuffer description.

  Similar in principle and structure to the Multiboot 2
  specification's framebuffer description.
*/
struct fbdesc
{
#define FB_INVALID -1
#define FB_RGB     0
  int type; 

  uint64_t addr;
  uint64_t size;

  uint32_t pitch;
  uint32_t width;
  uint32_t height;
  uint16_t bpp;

  uint32_t r_mask;
  uint32_t g_mask;
  uint32_t b_mask;
};

#endif
