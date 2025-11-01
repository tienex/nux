# Font Conversion Tools

## ttf2bdf - TrueType to Bitmap Font Converter

This tool converts TrueType fonts to C source arrays suitable for embedding in the framebuffer library.

### Building

Requires FreeType 2 library:

```bash
# On Debian/Ubuntu
sudo apt-get install libfreetype6-dev

# On Fedora/RHEL
sudo dnf install freetype-devel

# Build the tool
gcc -DHAVE_FREETYPE -o ttf2bdf ttf2bdf.c -lfreetype -I/usr/include/freetype2
```

### Usage

```bash
./ttf2bdf <input.ttf> <size> <output.c> <array_name>
```

### Examples

Convert Inconsolata to 16-pixel bitmap font:

```bash
./ttf2bdf /usr/share/fonts/truetype/inconsolata/Inconsolata-Regular.ttf 16 \
    ../sources/fonts/inconsolata_16.c gFontInconsolata16Data
```

Convert Consolas to 14-pixel bitmap font:

```bash
./ttf2bdf /path/to/consola.ttf 14 \
    ../sources/fonts/consolas_14.c gFontConsolas14Data
```

### Adding New Fonts

1. Convert the font using ttf2bdf
2. Add the generated .c file to `ananke/libs/framebuffer/sources/fonts/`
3. Add declaration in `ananke/libs/framebuffer/sources/font.c`
4. Export the font in `ananke/libs/framebuffer/include/ananke/framebuffer/font.h`
5. Add to `SRCS` in `ananke/Makefile.in`

### Supported Character Ranges

Currently supports:
- U+0000 to U+00FF (Basic Latin + Latin-1 Supplement)

Future enhancements will support:
- Extended Unicode ranges (Cyrillic, Arabic, Hebrew, etc.)
- Variable-width fonts
- Anti-aliasing support
