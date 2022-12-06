#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

#include <stdint.h>

/*
  Framebuffer description.

  Similar in principle and structure to the Multiboot 2
  specification's framebuffer description.

  NOTE: This struct must look the same when compiled both
  in 32 and 64 bit. Which is why it is packed and uses
  only fixed-bits fields.
*/
struct fbdesc
{
#define FB_INVALID -1
#define FB_RGB     0
  int16_t type;
  uint16_t bpp;

  uint32_t pitch;
  uint32_t width;
  uint32_t height;

  uint64_t addr;
  uint64_t size;

  uint32_t r_mask;
  uint32_t g_mask;
  uint32_t b_mask;
} __packed;

int framebuffer_init (struct fbdesc *desc);
uint32_t framebuffer_color (unsigned r, unsigned g, unsigned b);
void framebuffer_blt (unsigned x, unsigned y, uint32_t color,
		      void *data, size_t width, size_t height);
int framebuffer_putc (int ch, uint32_t color);
void framebuffer_putc_xy (unsigned x, unsigned y, uint32_t color,
			  unsigned char c);

#endif
