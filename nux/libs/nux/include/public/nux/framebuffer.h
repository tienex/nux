#ifndef __nux_framebuffer_h__
#define __nux_framebuffer_h__

#include <stdint.h>

/**
  Framebuffer Type
**/
typedef enum _FRAMEBUFFER_TYPE {
  FramebufferInvalid = -1,  ///< Invalid/no framebuffer
  FramebufferRgb     = 0    ///< RGB framebuffer
} FRAMEBUFFER_TYPE;

/** Legacy compatibility **/
#define FB_INVALID FramebufferInvalid
#define FB_RGB     FramebufferRgb

/**
  Framebuffer Descriptor

  Similar in principle and structure to the Multiboot 2
  specification's framebuffer description.

  NOTE: This struct must look the same when compiled both
  in 32 and 64 bit. Which is why it is packed and uses
  only fixed-bits fields.
**/
typedef struct _FRAMEBUFFER_DESC
{
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
} __packed FRAMEBUFFER_DESC, *PFRAMEBUFFER_DESC, *PCFRAMEBUFFER_DESC;

/** Legacy compatibility **/
#define fbdesc FRAMEBUFFER_DESC

int FramebufferInit (FRAMEBUFFER_DESC *Desc);
uint32_t FramebufferColor (unsigned R, unsigned G, unsigned B);
void FramebufferBlt (unsigned X, unsigned Y, uint32_t Color,
		      void *Data, size_t Width, size_t Height);
int FramebufferPutc (int Ch, uint32_t Color);
void FramebufferPutcXy (unsigned X, unsigned Y, uint32_t Color,
			  unsigned char C);

void FramebufferReset (void);

/** Legacy compatibility **/
#define framebuffer_init FramebufferInit
#define framebuffer_color FramebufferColor
#define framebuffer_blt FramebufferBlt
#define framebuffer_putc FramebufferPutc
#define framebuffer_putc_xy FramebufferPutcXy
#define framebuffer_reset FramebufferReset

#endif
