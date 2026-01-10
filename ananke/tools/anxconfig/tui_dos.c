/*
 * DOS Console TUI Backend
 *
 * Implements the Ananke TUI interfaces using BIOS interrupts and
 * direct VGA text mode access for DOS systems.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ananke/tui.h>
#include <ananke/anxconfig.h>

#ifdef __MSDOS__
#include <dos.h>
#include <conio.h>

/* Video memory address for text mode */
#define VIDEO_MEMORY 0xB8000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

/* TuiScreen structure */
typedef struct {
    ITuiScreen Interface;
    UINTN RefCount;
    BOOLEAN Initialized;
    UINT16 far *VideoMem;
    UINT8 CurrentAttr;
} TuiScreen;

/* Color mapping to VGA attributes */
static UINT8 MapColor(TUI_COLOR Fg, TUI_COLOR Bg)
{
    UINT8 fg = 0, bg = 0;

    switch (Fg) {
        case TUI_COLOR_BLACK:   fg = 0; break;
        case TUI_COLOR_BLUE:    fg = 1; break;
        case TUI_COLOR_GREEN:   fg = 2; break;
        case TUI_COLOR_CYAN:    fg = 3; break;
        case TUI_COLOR_RED:     fg = 4; break;
        case TUI_COLOR_MAGENTA: fg = 5; break;
        case TUI_COLOR_YELLOW:  fg = 6; break;
        case TUI_COLOR_WHITE:   fg = 7; break;
        default:                fg = 7; break;
    }

    switch (Bg) {
        case TUI_COLOR_BLACK:   bg = 0; break;
        case TUI_COLOR_BLUE:    bg = 1; break;
        case TUI_COLOR_GREEN:   bg = 2; break;
        case TUI_COLOR_CYAN:    bg = 3; break;
        case TUI_COLOR_RED:     bg = 4; break;
        case TUI_COLOR_MAGENTA: bg = 5; break;
        case TUI_COLOR_YELLOW:  bg = 6; break;
        case TUI_COLOR_WHITE:   bg = 7; break;
        default:                bg = 0; break;
    }

    return (bg << 4) | fg;
}

/* Key mapping from BIOS scan codes */
static TUI_KEY MapBiosKey(int ch, int scan)
{
    /* ASCII characters */
    if (ch != 0) {
        switch (ch) {
            case 13:  return TUI_KEY_ENTER;
            case 27:  return TUI_KEY_ESC;
            case 8:   return TUI_KEY_BACKSPACE;
            case 9:   return TUI_KEY_TAB;
            default:
                if (ch >= 32 && ch <= 126) {
                    return (TUI_KEY)ch;
                }
        }
    }

    /* Extended keys (scan codes) */
    switch (scan) {
        case 0x48: return TUI_KEY_UP;
        case 0x50: return TUI_KEY_DOWN;
        case 0x4B: return TUI_KEY_LEFT;
        case 0x4D: return TUI_KEY_RIGHT;
        case 0x3B: return TUI_KEY_F1;
        case 0x3C: return TUI_KEY_F2;
        case 0x3D: return TUI_KEY_F3;
        case 0x3E: return TUI_KEY_F4;
        case 0x3F: return TUI_KEY_F5;
        case 0x40: return TUI_KEY_F6;
        case 0x41: return TUI_KEY_F7;
        case 0x42: return TUI_KEY_F8;
        case 0x43: return TUI_KEY_F9;
        case 0x44: return TUI_KEY_F10;
        case 0x85: return TUI_KEY_F11;
        case 0x86: return TUI_KEY_F12;
        case 0x53: return TUI_KEY_DELETE;
        case 0x52: return TUI_KEY_INSERT;
        case 0x47: return TUI_KEY_HOME;
        case 0x4F: return TUI_KEY_END;
        case 0x49: return TUI_KEY_PAGEUP;
        case 0x51: return TUI_KEY_PAGEDOWN;
        default:   return TUI_KEY_UNKNOWN;
    }
}

/* IUnknown methods */
static HRESULT ANXAPI Screen_QueryInterface(
    ITuiScreen *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) {
        return E_POINTER;
    }

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_ITuiScreen)) {
        *ppvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Screen_AddRef(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    return ++screen->RefCount;
}

static UINTN ANXAPI Screen_Release(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    UINTN refCount = --screen->RefCount;

    if (refCount == 0) {
        if (screen->Initialized) {
            /* Restore text mode */
            _setvideomode(_TEXTC80);
        }
        free(screen);
    }

    return refCount;
}

/* ITuiScreen methods */
static HRESULT ANXAPI Screen_Initialize(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    union REGS regs;

    if (screen->Initialized) {
        return S_OK;
    }

    /* Set video mode to 80x25 text mode */
    regs.h.ah = 0x00;
    regs.h.al = 0x03;  /* 80x25 color text */
    int86(0x10, &regs, &regs);

    /* Hide cursor */
    regs.h.ah = 0x01;
    regs.h.ch = 0x20;  /* Cursor start */
    regs.h.cl = 0x00;  /* Cursor end */
    int86(0x10, &regs, &regs);

    /* Get video memory pointer */
    screen->VideoMem = (UINT16 far *)MK_FP(0xB800, 0x0000);
    screen->CurrentAttr = 0x07;  /* White on black */
    screen->Initialized = TRUE;

    return S_OK;
}

static HRESULT ANXAPI Screen_GetDimensions(
    ITuiScreen *This,
    UINT32 *Width,
    UINT32 *Height
)
{
    if (Width == NULL || Height == NULL) {
        return E_POINTER;
    }

    *Width = SCREEN_WIDTH;
    *Height = SCREEN_HEIGHT;
    return S_OK;
}

static HRESULT ANXAPI Screen_Clear(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    int i;

    if (!screen->Initialized) {
        return E_FAIL;
    }

    /* Clear screen by writing spaces to video memory */
    for (i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        screen->VideoMem[i] = (screen->CurrentAttr << 8) | ' ';
    }

    return S_OK;
}

static HRESULT ANXAPI Screen_WriteText(
    ITuiScreen *This,
    INT32 X,
    INT32 Y,
    CONST CHAR8 *Text,
    TUI_COLOR Fg,
    TUI_COLOR Bg
)
{
    TuiScreen *screen = (TuiScreen *)This;
    UINT8 attr;
    int offset;
    const char *p;

    if (!screen->Initialized || Text == NULL) {
        return E_POINTER;
    }

    if (X < 0 || Y < 0 || Y >= SCREEN_HEIGHT) {
        return E_INVALIDARG;
    }

    attr = MapColor(Fg, Bg);
    offset = Y * SCREEN_WIDTH + X;

    /* Write text to video memory */
    for (p = Text; *p && offset < SCREEN_WIDTH * SCREEN_HEIGHT; p++, offset++) {
        if ((offset % SCREEN_WIDTH) >= SCREEN_WIDTH) {
            break;  /* Don't wrap to next line */
        }
        screen->VideoMem[offset] = (attr << 8) | *p;
    }

    return S_OK;
}

static HRESULT ANXAPI Screen_DrawBox(
    ITuiScreen *This,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height,
    TUI_COLOR Fg,
    TUI_COLOR Bg
)
{
    TuiScreen *screen = (TuiScreen *)This;
    UINT8 attr;
    UINT32 i;
    int offset;

    if (!screen->Initialized) {
        return E_FAIL;
    }

    attr = MapColor(Fg, Bg);

    /* Draw top border */
    offset = Y * SCREEN_WIDTH + X;
    screen->VideoMem[offset] = (attr << 8) | 0xDA;  /* ┌ */
    for (i = 1; i < Width - 1; i++) {
        screen->VideoMem[offset + i] = (attr << 8) | 0xC4;  /* ─ */
    }
    screen->VideoMem[offset + Width - 1] = (attr << 8) | 0xBF;  /* ┐ */

    /* Draw sides */
    for (i = 1; i < Height - 1; i++) {
        offset = (Y + i) * SCREEN_WIDTH + X;
        screen->VideoMem[offset] = (attr << 8) | 0xB3;  /* │ */
        screen->VideoMem[offset + Width - 1] = (attr << 8) | 0xB3;  /* │ */
    }

    /* Draw bottom border */
    offset = (Y + Height - 1) * SCREEN_WIDTH + X;
    screen->VideoMem[offset] = (attr << 8) | 0xC0;  /* └ */
    for (i = 1; i < Width - 1; i++) {
        screen->VideoMem[offset + i] = (attr << 8) | 0xC4;  /* ─ */
    }
    screen->VideoMem[offset + Width - 1] = (attr << 8) | 0xD9;  /* ┘ */

    return S_OK;
}

static HRESULT ANXAPI Screen_GetKey(ITuiScreen *This, TUI_KEY *Key)
{
    TuiScreen *screen = (TuiScreen *)This;
    int ch, scan;

    if (!screen->Initialized || Key == NULL) {
        return E_POINTER;
    }

    /* Use BIOS keyboard input */
    ch = getch();
    scan = 0;

    /* Extended key? */
    if (ch == 0 || ch == 0xE0) {
        scan = getch();
    }

    *Key = MapBiosKey(ch, scan);
    return S_OK;
}

static HRESULT ANXAPI Screen_Refresh(ITuiScreen *This)
{
    /* No refresh needed for direct video memory access */
    (void)This;
    return S_OK;
}

/* Vtable */
static CONST ITuiScreen_Vtbl ScreenVtbl = {
    Screen_QueryInterface,
    Screen_AddRef,
    Screen_Release,
    Screen_Initialize,
    Screen_GetDimensions,
    Screen_Clear,
    Screen_WriteText,
    Screen_DrawBox,
    Screen_GetKey,
    Screen_Refresh
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateScreen(OUT ITuiScreen **Screen)
{
    TuiScreen *screen;

    if (Screen == NULL) {
        return E_POINTER;
    }

    screen = (TuiScreen *)calloc(1, sizeof(TuiScreen));
    if (screen == NULL) {
        *Screen = NULL;
        return E_OUTOFMEMORY;
    }

    screen->Interface.Vtbl = &ScreenVtbl;
    screen->RefCount = 1;
    screen->Initialized = FALSE;

    *Screen = &screen->Interface;
    return S_OK;
}

#endif /* __MSDOS__ */
