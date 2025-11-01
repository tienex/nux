# ANANKE Framebuffer Library

A comprehensive COM-based framebuffer library with a modular backend architecture, software engine for feature emulation, and support for 30+ different graphics hardware platforms.

## Architecture Overview

The framebuffer library uses a three-tier architecture:

1. **User-Facing Interfaces** - High-level abstractions for application developers
2. **Framebuffer Engine** - Software emulation layer for unsupported features
3. **Hardware Backends** - Platform-specific implementations

```
┌─────────────────────────────────────────────────────────┐
│              User Application Code                       │
└──────────────────┬──────────────────────────────────────┘
                   │
        ┌──────────┴───────────┐
        ├─ IFramebufferManager │ (Device enumeration, mode setting)
        ├─ IFramebufferScreen  │ (Hardware framebuffer access)
        ├─ IFramebufferSurface │ (Software composition)
        ├─ IFramebufferImage   │ (Blitting with format conversion)
        └─ IFramebufferCursor  │ (Mouse cursor support)
                   │
┌──────────────────┴──────────────────────────────────────┐
│          Framebuffer Engine (Software Fallback)          │
│  - ROP operations        - Pixel format conversion       │
│  - Fill acceleration     - Automatic dithering           │
│  - Software cursor       - Feature emulation             │
└──────────────────┬──────────────────────────────────────┘
                   │
┌──────────────────┴──────────────────────────────────────┐
│              Hardware Backend (IFramebufferBackend)      │
│  CGA, EGA, VGA, VESA, UEFI GOP, Hercules, Amiga,        │
│  Atari, Mac, Sun, SGI, NeXT, Acorn, Text-mode, etc.     │
└──────────────────────────────────────────────────────────┘
```

## Features

### User-Facing Interfaces

#### IFramebufferManager
- Enumerate available framebuffer devices
- List supported graphics modes with capabilities
- Set graphics modes at runtime
- Get device capabilities (hardware acceleration, VBlank, page flipping)
- Create screens and surfaces

#### IFramebufferScreen
- Access to hardware framebuffer
- VBlank synchronization
- Page flipping / double buffering
- Hardware cursor support
- Palette management for indexed modes

#### IFramebufferSurface
- Software composition surface
- Blit to screen or other surfaces
- Off-screen rendering

#### IFramebufferImage
- Universal blitting with automatic format conversion
- Supports all pixel format transformations
- Automatic dithering for bit depth reduction
- ROP (Raster Operation) support
- Color key transparency

#### IFramebufferCursor
- Monochrome and color cursor support
- Hardware cursor when available, software fallback
- Standard cursor shapes included

### Framebuffer Engine

The engine provides software emulation for features not supported by hardware:

- **ROP Operations:** XOR, OR, AND, NOT, blend, add, subtract
- **Fill Acceleration:** Optimized rectangle filling
- **Pixel Format Conversion:** Automatic conversion between any formats
- **Dithering:** Classic Macintosh and Bayer dithering
- **Software Cursor:** When hardware cursor unavailable
- **Feature Emulation:** Any missing hardware feature

### Pixel Format Support

**Monochrome:**
- 1BPP monochrome (Hercules, Mac, Atari mono)
- 2BPP grayscale (NeXT)

**CGA Formats:**
- CGA 4-color indexed (320x200)
- CGA 2-color monochrome (640x200)

**EGA/VGA Planar:**
- EGA 16-color planar (640x350)
- VGA 16-color planar (640x480)

**VGA Linear:**
- VGA Mode 13h (320x200x256)
- 8-bit indexed (256 colors)
- 4-bit indexed (16 colors)

**RGB Formats:**
- RGB332 (8-bit)
- RGB555 (15-bit)
- RGB565 (16-bit)
- RGB888 (24-bit)
- RGBA8888 (32-bit with alpha)
- BGR888/BGRA8888 (Apple quirk mode)

**Platform-Specific:**
- Amiga OCS/ECS/AGA planar and HAM modes
- Atari ST/TT/Falcon formats
- Sun SPARC formats (cgthree, cgsix)
- SGI RGB format
- Acorn VIDC palette modes

### Text Rendering

- **Unicode Support:** Full CHAR16 support for international text
- **BIDI:** Bidirectional text algorithm for Arabic and Hebrew
- **Fonts:**
  - VGA 8x8, 8x14, 8x16 bitmap fonts (CP437)
  - Scrawl font
  - Extensible with custom TrueType-converted fonts

### Dithering

- Classic Macintosh dithering (8x8 Bayer-style ordered dithering)
- 17-level grayscale patterns
- Error diffusion for color reduction
- Automatic dithering when converting to lower bit depth

### Text-Mode Graphics (libcaca-style)

The library includes a unique text-mode graphics backend that renders graphics using characters:

- **ASCII Art Mode:** Uses ASCII characters for grayscale shading
- **Unicode Block Mode:** Uses Unicode box drawing characters
- **Half-Block Mode:** 2:1 vertical resolution using ▀ and ▄
- **Color Support:** ANSI/VT100 16-color text mode

Perfect for serial consoles, SSH sessions, and retro aesthetics!

### COM Architecture

All functionality exposed through COM interfaces:

- `IFramebufferManager` - Device and mode management
- `IFramebufferScreen` - Hardware framebuffer access
- `IFramebufferSurface` - Software surfaces
- `IFramebufferImage` - Image blitting and conversion
- `IFramebufferCursor` - Cursor management
- `IFramebufferBackend` - Backend drawing operations
- `IFramebufferBackendExt` - Extended backend capabilities
- `IFramebufferPalette` - Palette management
- `IFramebufferText` - Unicode text rendering

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

### UEFI Universal Graphics Adapter (`FbBackendUefiUga`)

Legacy UEFI/EFI 1.x framebuffer through UGA protocol.
UGA was the predecessor to GOP, used in early UEFI implementations and older Macs.
Provides hardware-accelerated Blt operations (fill, copy, video-to-buffer).

Features:
- Software buffer fallback for systems without direct framebuffer access
- Hardware-accelerated block transfers via UGA Blt
- Full 32-bit BGRA color support
- Compatible with EFI 1.10 and early UEFI 2.x

```c
IFramebufferBackend *backend = FbCreateUefiUgaBackend();
FbUefiUgaSetProtocol(backend, UgaProtocol);  // Required
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
