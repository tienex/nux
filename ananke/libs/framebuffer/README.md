  # ANANKE Framebuffer Library

A COM-based framebuffer library with support for multiple pixel formats, Unicode text rendering, and bidirectional text (BIDI).

## Features

### Pixel Format Support

- **RGB Modes:**
  - RGB888 (24-bit true color)
  - RGB565 (16-bit high color)
  - RGB555 (15-bit high color)

- **Indexed Color:**
  - 256-color palette mode
  - VGA 16-color planar mode
  - Standard VGA palette included

- **Monochrome:**
  - 1BPP for Hercules graphics

### Dithering

Implements classic Macintosh dithering algorithms:
- Bayer-style 8x8 ordered dithering
- 17-level grayscale patterns
- Error diffusion for color reduction

### Text Rendering

- **Unicode Support:** Full CHAR16 support for international text
- **BIDI:** Bidirectional text algorithm for Arabic and Hebrew
- **Fonts:**
  - VGA 8x8, 8x14, 8x16 bitmap fonts (CP437)
  - Scrawl font
  - Extensible with custom TrueType-converted fonts

### COM Architecture

All functionality exposed through COM interfaces:

- `IFramebufferBackend` - Drawing operations
- `IFramebufferText` - Unicode text rendering
- `IFramebufferPalette` - Palette management

## Building

The framebuffer library is built as part of `libananke.a`:

```bash
cd ananke
make
```

## Usage Example

```c
#include <ananke/framebuffer.h>
#include <ananke/framebuffer/backends.h>

/* Setup framebuffer descriptor */
FRAMEBUFFER_DESC desc = {
    .PixelFormat = FbPixelFormatRgb888,
    .Width = 1024,
    .Height = 768,
    .Pitch = 1024 * 4,
    .PhysicalBase = 0xE0000000,
    .Size = 1024 * 768 * 4,
    .RedMask   = 0x00FF0000,
    .GreenMask = 0x0000FF00,
    .BlueMask  = 0x000000FF,
};

/* Create backend */
IFramebufferBackend *backend = FbCreateGenericBackend();
IFramebufferBackend_Initialize(backend, &desc);

/* Clear screen */
FB_COLOR black = { 0, 0, 0, 255 };
IFramebufferBackend_Clear(backend, black);

/* Draw text */
IFramebufferText *text = FbCreateTextRenderer(backend);
FB_COLOR white = { 255, 255, 255, 255 };

const CHAR16 str[] = u"Hello, World!";
IFramebufferText_DrawString(text, 10, 10, str, 13, white, black, FbTextDirectionLTR);

/* Cleanup */
IUnknown_Release((IUnknown *)text);
IUnknown_Release((IUnknown *)backend);
```

## Backend Implementations

### Generic Backend

Supports all RGB and indexed color modes with software rendering.

### Planned Backends

- **UEFI GOP** - Graphics Output Protocol for UEFI systems
- **Apple EFI** - Apple-specific EFI graphics
- **Hercules** - Hercules Graphics Card (720×348 monochrome)
- **CGA/VGA** - IBM CGA/VGA text and graphics modes
- **VESA** - VESA BIOS Extensions (linear and banked)

## Font Conversion

See `tools/README.md` for information on converting TrueType fonts to bitmap format.

## License

BSD-2-Clause

## Copyright

Copyright (C) 2025 ANANKE Project
