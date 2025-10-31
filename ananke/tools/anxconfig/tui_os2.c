/*
 * OS/2 Console TUI Backend
 *
 * Implements the Ananke TUI interfaces using OS/2 VIO (Video I/O) API.
 * Used on OS/2 and eComStation systems.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ananke/tui.h>
#include <ananke/anxconfig.h>

#ifdef __OS2__
#define INCL_VIO
#define INCL_KBD
#define INCL_DOSPROCESS
#include <os2.h>

/* TuiScreen structure */
typedef struct {
    ITuiScreen Interface;
    UINTN RefCount;
    BOOLEAN Initialized;
    VIOMODEINFO OriginalMode;
    VIOCURSORINFO OriginalCursor;
} TuiScreen;

/* Color mapping to OS/2 VIO attributes */
static BYTE MapColor(TUI_COLOR Fg, TUI_COLOR Bg)
{
    BYTE fg = 0, bg = 0;

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

/* Key mapping from OS/2 keyboard API */
static TUI_KEY MapOS2Key(KBDKEYINFO *keyInfo)
{
    /* ASCII characters */
    if (keyInfo->chChar >= 32 && keyInfo->chChar <= 126) {
        return (TUI_KEY)keyInfo->chChar;
    }

    switch (keyInfo->chChar) {
        case 13:  return TUI_KEY_ENTER;
        case 27:  return TUI_KEY_ESC;
        case 8:   return TUI_KEY_BACKSPACE;
        case 9:   return TUI_KEY_TAB;
    }

    /* Extended keys (scan codes) */
    if (keyInfo->chChar == 0 || keyInfo->chChar == 0xE0) {
        switch (keyInfo->chScan) {
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

    return TUI_KEY_UNKNOWN;
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
            /* Restore original cursor */
            VioSetCurType(&screen->OriginalCursor, 0);
        }
        free(screen);
    }

    return refCount;
}

/* ITuiScreen methods */
static HRESULT ANXAPI Screen_Initialize(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    VIOCURSORINFO cursorInfo;

    if (screen->Initialized) {
        return S_OK;
    }

    /* Get original video mode */
    screen->OriginalMode.cb = sizeof(VIOMODEINFO);
    VioGetMode(&screen->OriginalMode, 0);

    /* Get original cursor info */
    VioGetCurType(&screen->OriginalCursor, 0);

    /* Hide cursor */
    cursorInfo.yStart = 0;
    cursorInfo.cEnd = 0;
    cursorInfo.cx = 0;
    cursorInfo.attr = -1;  /* Hidden */
    VioSetCurType(&cursorInfo, 0);

    screen->Initialized = TRUE;
    return S_OK;
}

static HRESULT ANXAPI Screen_GetDimensions(
    ITuiScreen *This,
    UINT32 *Width,
    UINT32 *Height
)
{
    TuiScreen *screen = (TuiScreen *)This;
    VIOMODEINFO modeInfo;

    if (Width == NULL || Height == NULL) {
        return E_POINTER;
    }

    if (!screen->Initialized) {
        return E_FAIL;
    }

    modeInfo.cb = sizeof(VIOMODEINFO);
    VioGetMode(&modeInfo, 0);

    *Width = modeInfo.col;
    *Height = modeInfo.row;

    return S_OK;
}

static HRESULT ANXAPI Screen_Clear(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    BYTE cell[2];

    if (!screen->Initialized) {
        return E_FAIL;
    }

    /* Clear screen with spaces */
    cell[0] = ' ';
    cell[1] = 0x07;  /* White on black */
    VioScrollDn(0, 0, -1, -1, -1, cell, 0);

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
    BYTE attr;
    USHORT len;

    if (!screen->Initialized || Text == NULL) {
        return E_POINTER;
    }

    attr = MapColor(Fg, Bg);
    len = (USHORT)strlen(Text);

    /* Write text with attribute */
    VioWrtCharStrAtt((PCH)Text, len, (USHORT)Y, (USHORT)X, &attr, 0);

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
    BYTE attr;
    UINT32 i;
    CHAR line[256];

    if (!screen->Initialized) {
        return E_FAIL;
    }

    attr = MapColor(Fg, Bg);

    /* Draw top border */
    line[0] = (CHAR)0xDA;  /* ┌ */
    for (i = 1; i < Width - 1; i++) {
        line[i] = (CHAR)0xC4;  /* ─ */
    }
    line[Width - 1] = (CHAR)0xBF;  /* ┐ */
    VioWrtCharStrAtt(line, (USHORT)Width, (USHORT)Y, (USHORT)X, &attr, 0);

    /* Draw sides */
    for (i = 1; i < Height - 1; i++) {
        line[0] = (CHAR)0xB3;  /* │ */
        VioWrtCharStrAtt(line, 1, (USHORT)(Y + i), (USHORT)X, &attr, 0);

        line[0] = (CHAR)0xB3;  /* │ */
        VioWrtCharStrAtt(line, 1, (USHORT)(Y + i), (USHORT)(X + Width - 1), &attr, 0);
    }

    /* Draw bottom border */
    line[0] = (CHAR)0xC0;  /* └ */
    for (i = 1; i < Width - 1; i++) {
        line[i] = (CHAR)0xC4;  /* ─ */
    }
    line[Width - 1] = (CHAR)0xD9;  /* ┘ */
    VioWrtCharStrAtt(line, (USHORT)Width, (USHORT)(Y + Height - 1), (USHORT)X, &attr, 0);

    return S_OK;
}

static HRESULT ANXAPI Screen_GetKey(ITuiScreen *This, TUI_KEY *Key)
{
    TuiScreen *screen = (TuiScreen *)This;
    KBDKEYINFO keyInfo;

    if (!screen->Initialized || Key == NULL) {
        return E_POINTER;
    }

    /* Wait for key press */
    if (KbdCharIn(&keyInfo, IO_WAIT, 0) != 0) {
        return E_FAIL;
    }

    *Key = MapOS2Key(&keyInfo);
    return S_OK;
}

static HRESULT ANXAPI Screen_Refresh(ITuiScreen *This)
{
    /* No explicit refresh needed for OS/2 VIO */
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

#endif /* __OS2__ */
