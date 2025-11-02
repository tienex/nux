/* VGA 8x8 Font - 2048 bytes (256 chars × 8 bytes)
 *
 * TODO: Replace with actual VGA font data
 *
 * This should contain the IBM VGA ROM 8x8 font (public domain).
 * Each character is 8 pixels wide × 8 pixels tall.
 * Each byte represents one scanline: bit 7 = left pixel, bit 0 = right pixel
 *
 * Standard sources for IBM VGA font data:
 * - VGABIOS source code (public domain)
 * - DOSBox VGA font tables
 * - SeaBIOS vgafonts.c
 *
 * The 8x8 font is typically used for 43/50-line EGA/VGA text modes.
 * Standard VGA text modes (25-line) use the 8x16 font.
 */
static CONST UINT8 gVgaFont8x8[256 * 8] = { 0 };
