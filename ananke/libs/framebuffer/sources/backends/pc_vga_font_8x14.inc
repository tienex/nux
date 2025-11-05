/* VGA 8x14 Font - 3584 bytes (256 chars × 14 bytes)
 *
 * TODO: Replace with actual EGA/VGA font data
 *
 * This should contain the IBM EGA/VGA ROM 8x14 font (public domain).
 * Each character is 8 pixels wide × 14 pixels tall.
 * Each byte represents one scanline: bit 7 = left pixel, bit 0 = right pixel
 *
 * Standard sources for IBM EGA/VGA font data:
 * - VGABIOS source code (public domain)
 * - DOSBox VGA font tables
 * - SeaBIOS vgafonts.c
 *
 * The 8x14 font is the standard EGA text mode font.
 * VGA also supports this font for compatibility with EGA modes.
 */
static CONST UINT8 gVgaFont8x14[256 * 14] = { 0 };
