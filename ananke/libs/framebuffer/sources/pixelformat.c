/*++
    Module Name:

        pixelformat.c

    Abstract:

        Pixel format conversion implementation.

--*/

#include <ananke/framebuffer/pixelformat.h>
#include <ananke/framebuffer/palette.h>  /* For gVga16Palette */

/* --------------------------------------------------------------- */
/*  Helper Functions                                                */
/* --------------------------------------------------------------- */

UINT32
FbCountTrailingZeros(
    IN UINT32 Value
    )
{
    UINT32 Count = 0;

    if (Value == 0) {
        return 32;
    }

    while ((Value & 1) == 0) {
        Value >>= 1;
        Count++;
    }

    return Count;
}

UINT32
FbCountLeadingZeros(
    IN UINT32 Value
    )
{
    UINT32 Count = 0;

    if (Value == 0) {
        return 32;
    }

    while ((Value & 0x80000000) == 0) {
        Value <<= 1;
        Count++;
    }

    return Count;
}

VOID
FbGetMaskInfo(
    IN UINT32 Mask,
    OUT UINT8 *Position,
    OUT UINT8 *Size
    )
{
    UINT32 Pos;
    UINT32 Count = 0;

    if (Mask == 0) {
        *Position = 0;
        *Size = 0;
        return;
    }

    /* Find position (trailing zeros) */
    Pos = FbCountTrailingZeros(Mask);
    *Position = (UINT8)Pos;

    /* Count bits */
    Mask >>= Pos;
    while (Mask & 1) {
        Count++;
        Mask >>= 1;
    }

    *Size = (UINT8)Count;
}

/* --------------------------------------------------------------- */
/*  Color Conversion                                                */
/* --------------------------------------------------------------- */

UINT32
FbPackPixel(
    IN FB_COLOR Color,
    IN FB_PIXEL_FORMAT Format,
    IN UINT32 RedMask,
    IN UINT32 GreenMask,
    IN UINT32 BlueMask
    )
{
    UINT8 RedPos, RedSize;
    UINT8 GreenPos, GreenSize;
    UINT8 BluePos, BlueSize;
    UINT32 Pixel = 0;

    switch (Format) {
        case FbPixelFormatRgb888:
            /* Standard 24-bit RGB */
            FbGetMaskInfo(RedMask, &RedPos, &RedSize);
            FbGetMaskInfo(GreenMask, &GreenPos, &GreenSize);
            FbGetMaskInfo(BlueMask, &BluePos, &BlueSize);

            Pixel |= ((UINT32)Color.Red   >> (8 - RedSize))   << RedPos;
            Pixel |= ((UINT32)Color.Green >> (8 - GreenSize)) << GreenPos;
            Pixel |= ((UINT32)Color.Blue  >> (8 - BlueSize))  << BluePos;
            break;

        case FbPixelFormatRgb555:
            /* 5:5:5 RGB (15-bit) */
            Pixel = ((UINT32)(Color.Red   >> 3) << 10) |
                    ((UINT32)(Color.Green >> 3) <<  5) |
                    ((UINT32)(Color.Blue  >> 3) <<  0);
            break;

        case FbPixelFormatRgb565:
            /* 5:6:5 RGB (16-bit) */
            Pixel = ((UINT32)(Color.Red   >> 3) << 11) |
                    ((UINT32)(Color.Green >> 2) <<  5) |
                    ((UINT32)(Color.Blue  >> 3) <<  0);
            break;

        case FbPixelFormatVga16Planar:
            /* Map to closest VGA16 color */
            Pixel = FbGetVga16Color(Color);
            break;

        case FbPixelFormat1Bpp:
            /* Convert to grayscale and threshold */
            Pixel = (FbRgbToGray(Color) >= 128) ? 1 : 0;
            break;

        default:
            Pixel = 0;
            break;
    }

    return Pixel;
}

FB_COLOR
FbUnpackPixel(
    IN UINT32 Pixel,
    IN FB_PIXEL_FORMAT Format,
    IN UINT32 RedMask,
    IN UINT32 GreenMask,
    IN UINT32 BlueMask
    )
{
    FB_COLOR Color = { 0, 0, 0, 255 };
    UINT8 RedPos, RedSize;
    UINT8 GreenPos, GreenSize;
    UINT8 BluePos, BlueSize;
    UINT32 Component;

    switch (Format) {
        case FbPixelFormatRgb888:
            FbGetMaskInfo(RedMask, &RedPos, &RedSize);
            FbGetMaskInfo(GreenMask, &GreenPos, &GreenSize);
            FbGetMaskInfo(BlueMask, &BluePos, &BlueSize);

            Component = (Pixel & RedMask) >> RedPos;
            Color.Red = (UINT8)((Component << (8 - RedSize)) | (Component >> (RedSize - (8 - RedSize))));

            Component = (Pixel & GreenMask) >> GreenPos;
            Color.Green = (UINT8)((Component << (8 - GreenSize)) | (Component >> (GreenSize - (8 - GreenSize))));

            Component = (Pixel & BlueMask) >> BluePos;
            Color.Blue = (UINT8)((Component << (8 - BlueSize)) | (Component >> (BlueSize - (8 - BlueSize))));
            break;

        case FbPixelFormatRgb555:
            Color.Red   = (UINT8)(((Pixel >> 10) & 0x1F) << 3);
            Color.Green = (UINT8)(((Pixel >>  5) & 0x1F) << 3);
            Color.Blue  = (UINT8)(((Pixel >>  0) & 0x1F) << 3);
            /* Replicate top bits for better accuracy */
            Color.Red   |= Color.Red   >> 5;
            Color.Green |= Color.Green >> 5;
            Color.Blue  |= Color.Blue  >> 5;
            break;

        case FbPixelFormatRgb565:
            Color.Red   = (UINT8)(((Pixel >> 11) & 0x1F) << 3);
            Color.Green = (UINT8)(((Pixel >>  5) & 0x3F) << 2);
            Color.Blue  = (UINT8)(((Pixel >>  0) & 0x1F) << 3);
            /* Replicate top bits for better accuracy */
            Color.Red   |= Color.Red   >> 5;
            Color.Green |= Color.Green >> 6;
            Color.Blue  |= Color.Blue  >> 5;
            break;

        case FbPixelFormatVga16Planar:
            if (Pixel < 16) {
                Color.Red   = gVga16Palette[Pixel].Red;
                Color.Green = gVga16Palette[Pixel].Green;
                Color.Blue  = gVga16Palette[Pixel].Blue;
                Color.Alpha = 255;
            }
            break;

        case FbPixelFormat1Bpp:
            Color.Red = Color.Green = Color.Blue = Pixel ? 255 : 0;
            break;

        default:
            break;
    }

    return Color;
}

UINT8
FbRgbToGray(
    IN FB_COLOR Color
    )
{
    /* ITU-R BT.601 luma coefficients */
    return (UINT8)((Color.Red * 77 + Color.Green * 150 + Color.Blue * 29) >> 8);
}

UINT8
FbFindClosestPaletteEntry(
    IN FB_COLOR Color,
    IN CONST FB_PALETTE_ENTRY *Palette,
    IN UINT32 PaletteSize
    )
{
    UINT32 i;
    UINT8 BestIndex = 0;
    UINT32 BestDistance = 0xFFFFFFFF;

    for (i = 0; i < PaletteSize; i++) {
        INT32 dr = (INT32)Color.Red   - (INT32)Palette[i].Red;
        INT32 dg = (INT32)Color.Green - (INT32)Palette[i].Green;
        INT32 db = (INT32)Color.Blue  - (INT32)Palette[i].Blue;

        /* Euclidean distance (squared, to avoid sqrt) */
        UINT32 Distance = (UINT32)(dr * dr + dg * dg + db * db);

        if (Distance < BestDistance) {
            BestDistance = Distance;
            BestIndex = (UINT8)i;

            /* Exact match - early exit */
            if (Distance == 0) {
                break;
            }
        }
    }

    return BestIndex;
}

UINT8
FbGetVga16Color(
    IN FB_COLOR Color
    )
{
    return FbFindClosestPaletteEntry(Color, gVga16Palette, 16);
}
