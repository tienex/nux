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

All backends are fully implemented and available through the factory function or direct constructors.

### Generic Backend (`FbBackendGeneric`)

Software renderer supporting all RGB formats (RGB888, RGB565, RGB555) and indexed color modes.
Best for modern systems with linear framebuffers.

```c
IFramebufferBackend *backend = FbCreateGenericBackend();
// Or: FbCreateBackend(FbBackendGeneric);
```

### Hercules Graphics Card (`FbBackendHercules`)

720×348 monochrome (1BPP) display with 4-bank interleaved memory layout.
Physical address: 0xB0000. Supports classic Mac dithering for grayscale simulation.

```c
IFramebufferBackend *backend = FbCreateHerculesBackend();
```

### VGA 16-Color Planar (`FbBackendVga16`)

Standard VGA planar mode (640×480×16) with 4 bit planes.
Uses VGA hardware registers for plane selection. Supports all VGA graphics modes.

```c
IFramebufferBackend *backend = FbCreateVga16Backend();
```

### VESA Linear Framebuffer (`FbBackendVesaLinear`)

VESA 2.0+ linear framebuffer modes (LFB). Direct memory access without banking.
Supports high-resolution modes with RGB555/565/888 formats.

```c
IFramebufferBackend *backend = FbCreateVesaLinearBackend();
```

### VESA Banked/Segmented (`FbBackendVesaBanked`)

VESA 1.x/2.0 banked modes with 64KB window at 0xA0000.
Requires bank switching function for accessing memory beyond 64KB.

```c
IFramebufferBackend *backend = FbCreateVesaBankedBackend();
FbVesaBankedSetBankFunction(backend, MyBankSwitchFunc);
```

### UEFI Graphics Output Protocol (`FbBackendUefiGop`)

Modern UEFI framebuffer through GOP protocol.
Optionally accepts GOP protocol pointer for hardware-accelerated Blt operations.

```c
IFramebufferBackend *backend = FbCreateUefiGopBackend();
FbUefiGopSetProtocol(backend, GopProtocol);  // Optional
```

### Apple EFI (`FbBackendAppleEfi`)

Apple Mac EFI framebuffer with quirk handling:
- Automatic BGR vs RGB detection
- Retina display detection (>2560 width)
- Apple-specific pixel format handling

```c
IFramebufferBackend *backend = FbCreateAppleEfiBackend();
```

## Backend Selection

Use the factory function for runtime backend selection:

```c
FB_BACKEND_TYPE type = DetectHardware();
IFramebufferBackend *backend = FbCreateBackend(type);
```

## Font Conversion

See `tools/README.md` for information on converting TrueType fonts to bitmap format.

## License

BSD-2-Clause

## Copyright

Copyright (C) 2025 ANANKE Project
