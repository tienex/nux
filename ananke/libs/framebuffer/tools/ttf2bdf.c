/*++
    Module Name:

        ttf2bdf.c

    Abstract:

        TrueType font to bitmap font converter.
        Converts TrueType fonts to C source arrays for embedding.

    Usage:

        ttf2bdf <input.ttf> <size> <output.c> <array_name>

    Example:

        ttf2bdf Inconsolata.ttf 16 font_inconsolata_16.c gFontInconsolata16Data

    Dependencies:

        Requires FreeType 2 library for TrueType rendering.
        Build with: gcc -o ttf2bdf ttf2bdf.c -lfreetype

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H

#define UNICODE_BASIC_LATIN_START    0x0000
#define UNICODE_BASIC_LATIN_END      0x007F
#define UNICODE_LATIN_1_END          0x00FF
#define UNICODE_LATIN_EXT_A_END      0x017F
#define UNICODE_CYRILLIC_END         0x04FF
#define UNICODE_ARABIC_END           0x06FF
#define UNICODE_HEBREW_END           0x05FF

typedef struct {
    uint16_t codepoint;
    uint8_t width;
    uint8_t height;
    uint8_t *bitmap;
} Glyph;

static void
PrintUsage(const char *progname)
{
    fprintf(stderr, "Usage: %s <input.ttf> <size> <output.c> <array_name>\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "Converts a TrueType font to a C source file with bitmap glyphs.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "  %s Inconsolata.ttf 16 font_inconsolata_16.c gFontInconsolata16Data\n", progname);
}

static int
RenderGlyph(
    FT_Face face,
    uint16_t codepoint,
    Glyph *glyph
    )
{
    FT_UInt glyph_index;
    FT_Error error;
    FT_Bitmap *bitmap;
    uint32_t x, y;
    uint8_t *dest;

    glyph_index = FT_Get_Char_Index(face, codepoint);
    if (glyph_index == 0) {
        return -1;  /* Glyph not found */
    }

    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if (error) {
        return -1;
    }

    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
    if (error) {
        return -1;
    }

    bitmap = &face->glyph->bitmap;

    glyph->codepoint = codepoint;
    glyph->width = bitmap->width;
    glyph->height = bitmap->rows;

    /* Allocate bitmap buffer */
    glyph->bitmap = calloc(((bitmap->width + 7) / 8) * bitmap->rows, 1);
    if (!glyph->bitmap) {
        return -1;
    }

    /* Convert FreeType bitmap to our format (1 bit per pixel, MSB first) */
    for (y = 0; y < bitmap->rows; y++) {
        for (x = 0; x < bitmap->width; x++) {
            uint8_t pixel;
            uint32_t src_byte = x / 8;
            uint32_t src_bit = 7 - (x % 8);
            uint32_t dst_byte = y * ((bitmap->width + 7) / 8) + (x / 8);
            uint32_t dst_bit = 7 - (x % 8);

            pixel = (bitmap->buffer[y * bitmap->pitch + src_byte] >> src_bit) & 1;
            if (pixel) {
                glyph->bitmap[dst_byte] |= (1 << dst_bit);
            }
        }
    }

    return 0;
}

int
main(int argc, char **argv)
{
    FT_Library library;
    FT_Face face;
    FT_Error error;
    const char *font_file;
    const char *output_file;
    const char *array_name;
    int font_size;
    FILE *output;
    uint16_t codepoint;
    Glyph *glyphs;
    uint32_t glyph_count = 0;
    uint32_t i, j;

    if (argc != 5) {
        PrintUsage(argv[0]);
        return 1;
    }

    font_file = argv[1];
    font_size = atoi(argv[2]);
    output_file = argv[3];
    array_name = argv[4];

    if (font_size <= 0 || font_size > 128) {
        fprintf(stderr, "Error: Font size must be between 1 and 128\n");
        return 1;
    }

    /* Initialize FreeType */
    error = FT_Init_FreeType(&library);
    if (error) {
        fprintf(stderr, "Error: Could not initialize FreeType library\n");
        return 1;
    }

    /* Load font */
    error = FT_New_Face(library, font_file, 0, &face);
    if (error) {
        fprintf(stderr, "Error: Could not load font file: %s\n", font_file);
        FT_Done_FreeType(library);
        return 1;
    }

    /* Set font size */
    error = FT_Set_Pixel_Sizes(face, 0, font_size);
    if (error) {
        fprintf(stderr, "Error: Could not set font size\n");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 1;
    }

    /* Allocate glyph array (support Basic Latin + Latin-1 Supplement for now) */
    glyphs = calloc(256, sizeof(Glyph));
    if (!glyphs) {
        fprintf(stderr, "Error: Out of memory\n");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 1;
    }

    /* Render all glyphs */
    printf("Rendering glyphs from 0x00 to 0xFF...\n");
    for (codepoint = 0; codepoint < 256; codepoint++) {
        if (RenderGlyph(face, codepoint, &glyphs[glyph_count]) == 0) {
            glyph_count++;
        } else {
            /* Use '?' as fallback for missing glyphs */
            if (codepoint != '?') {
                memcpy(&glyphs[glyph_count], &glyphs['?'], sizeof(Glyph));
                glyphs[glyph_count].codepoint = codepoint;
            }
            glyph_count++;
        }
    }

    printf("Rendered %u glyphs\n", glyph_count);

    /* Open output file */
    output = fopen(output_file, "w");
    if (!output) {
        fprintf(stderr, "Error: Could not open output file: %s\n", output_file);
        free(glyphs);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 1;
    }

    /* Write C source file */
    fprintf(output, "/*\n");
    fprintf(output, " * Bitmap font generated from TrueType font: %s\n", font_file);
    fprintf(output, " * Font size: %d pixels\n", font_size);
    fprintf(output, " * Generated by ttf2bdf\n");
    fprintf(output, " */\n\n");
    fprintf(output, "#include <ananke/types.h>\n\n");
    fprintf(output, "CONST UINT8 %s[] = {\n", array_name);

    /* Write glyph data */
    for (i = 0; i < glyph_count; i++) {
        uint32_t bytes_per_row = (glyphs[i].width + 7) / 8;
        fprintf(output, "    /* U+%04X (codepoint %u) */\n", glyphs[i].codepoint, i);
        for (j = 0; j < glyphs[i].height; j++) {
            fprintf(output, "    ");
            for (uint32_t k = 0; k < bytes_per_row; k++) {
                fprintf(output, "0x%02x, ", glyphs[i].bitmap[j * bytes_per_row + k]);
            }
            fprintf(output, "\n");
        }
    }

    fprintf(output, "};\n");

    fclose(output);

    printf("Font conversion complete: %s\n", output_file);
    printf("Array name: %s\n", array_name);
    printf("Glyphs: %u (0x00-0xFF)\n", glyph_count);
    printf("Font size: %dx%d pixels\n", glyphs[0].width, glyphs[0].height);

    /* Cleanup */
    for (i = 0; i < glyph_count; i++) {
        free(glyphs[i].bitmap);
    }
    free(glyphs);

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}

#else /* !HAVE_FREETYPE */

int
main(int argc, char **argv)
{
    fprintf(stderr, "Error: This tool requires FreeType 2 library\n");
    fprintf(stderr, "Please install FreeType 2 development package and rebuild with:\n");
    fprintf(stderr, "  gcc -DHAVE_FREETYPE -o ttf2bdf ttf2bdf.c -lfreetype\n");
    return 1;
}

#endif /* HAVE_FREETYPE */
