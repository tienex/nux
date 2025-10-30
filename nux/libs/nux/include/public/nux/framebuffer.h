#ifndef __nux_framebuffer_h__
#define __nux_framebuffer_h__

#include <nux/types.h>

/**
  Framebuffer Type
**/
typedef enum _FRAMEBUFFER_TYPE {
  FramebufferInvalid = -1,  ///< Invalid/no framebuffer
  FramebufferRgb     = 0    ///< RGB framebuffer
} FRAMEBUFFER_TYPE;

/**
  Framebuffer Descriptor

  Similar in principle and structure to the Multiboot 2
  specification's framebuffer description.

  NOTE: This struct must look the same when compiled both
  in 32 and 64 bit. Which is why it is packed and uses
  only fixed-width UEFI types.
**/
typedef struct _FRAMEBUFFER_DESC
{
  INT16 Type;
  UINT16 Bpp;

  UINT32 Pitch;
  UINT32 Width;
  UINT32 Height;

  UINT64 Addr;
  UINT64 Size;

  UINT32 RMask;
  UINT32 GMask;
  UINT32 BMask;
} __packed FRAMEBUFFER_DESC, *PFRAMEBUFFER_DESC, *PCFRAMEBUFFER_DESC;

INT32 FramebufferInit (IN FRAMEBUFFER_DESC *Desc);
UINT32 FramebufferColor (IN UINT32 R, IN UINT32 G, IN UINT32 B);
VOID FramebufferBlt (IN UINT32 X, IN UINT32 Y, IN UINT32 Color,
		      IN VOID *Data, IN UINTN Width, IN UINTN Height);
INT32 FramebufferPutc (IN INT32 Ch, IN UINT32 Color);
VOID FramebufferPutcXy (IN UINT32 X, IN UINT32 Y, IN UINT32 Color,
			  IN UINT8 C);

VOID FramebufferReset (VOID);

#endif
