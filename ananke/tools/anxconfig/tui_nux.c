/*
 * NUX Kernel TUI Backend
 *
 * Implements the Ananke TUI interfaces using NUX kernel console.
 * Used for runtime kernel configuration via framebuffer or serial console.
 * This backend can be used in both userspace (via /dev/console) and
 * kernel space (direct framebuffer/serial access).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ananke/tui.h>
#include <ananke/anxconfig.h>

#ifdef __NUX__
#include <nux/console.h>
#include <nux/framebuffer.h>
#include <nux/keyboard.h>

/* TuiScreen structure */
typedef struct {
    ITuiScreen Interface;
    UINTN RefCount;
    BOOLEAN Initialized;
    NUX_CONSOLE *Console;
    UINT32 Width;
    UINT32 Height;
} TuiScreen;

/* Color mapping to NUX console colors */
static NUX_COLOR MapColor(TUI_COLOR Color)
{
    switch (Color) {
        case TUI_COLOR_BLACK:   return NUX_COLOR_BLACK;
        case TUI_COLOR_BLUE:    return NUX_COLOR_BLUE;
        case TUI_COLOR_GREEN:   return NUX_COLOR_GREEN;
        case TUI_COLOR_CYAN:    return NUX_COLOR_CYAN;
        case TUI_COLOR_RED:     return NUX_COLOR_RED;
        case TUI_COLOR_MAGENTA: return NUX_COLOR_MAGENTA;
        case TUI_COLOR_YELLOW:  return NUX_COLOR_YELLOW;
        case TUI_COLOR_WHITE:   return NUX_COLOR_WHITE;
        default:                return NUX_COLOR_WHITE;
    }
}

/* Key mapping from NUX keyboard */
static TUI_KEY MapNuxKey(NUX_KEY nuxKey)
{
    switch (nuxKey) {
        case NUX_KEY_UP:       return TUI_KEY_UP;
        case NUX_KEY_DOWN:     return TUI_KEY_DOWN;
        case NUX_KEY_LEFT:     return TUI_KEY_LEFT;
        case NUX_KEY_RIGHT:    return TUI_KEY_RIGHT;
        case NUX_KEY_ENTER:    return TUI_KEY_ENTER;
        case NUX_KEY_ESC:      return TUI_KEY_ESC;
        case NUX_KEY_F1:       return TUI_KEY_F1;
        case NUX_KEY_F2:       return TUI_KEY_F2;
        case NUX_KEY_F3:       return TUI_KEY_F3;
        case NUX_KEY_F4:       return TUI_KEY_F4;
        case NUX_KEY_F5:       return TUI_KEY_F5;
        case NUX_KEY_F6:       return TUI_KEY_F6;
        case NUX_KEY_F7:       return TUI_KEY_F7;
        case NUX_KEY_F8:       return TUI_KEY_F8;
        case NUX_KEY_F9:       return TUI_KEY_F9;
        case NUX_KEY_F10:      return TUI_KEY_F10;
        case NUX_KEY_F11:      return TUI_KEY_F11;
        case NUX_KEY_F12:      return TUI_KEY_F12;
        case NUX_KEY_BACKSPACE: return TUI_KEY_BACKSPACE;
        case NUX_KEY_DELETE:   return TUI_KEY_DELETE;
        case NUX_KEY_INSERT:   return TUI_KEY_INSERT;
        case NUX_KEY_HOME:     return TUI_KEY_HOME;
        case NUX_KEY_END:      return TUI_KEY_END;
        case NUX_KEY_PAGEUP:   return TUI_KEY_PAGEUP;
        case NUX_KEY_PAGEDOWN: return TUI_KEY_PAGEDOWN;
        case NUX_KEY_TAB:      return TUI_KEY_TAB;
        default:
            /* ASCII characters */
            if (nuxKey >= 32 && nuxKey <= 126) {
                return (TUI_KEY)nuxKey;
            }
            return TUI_KEY_UNKNOWN;
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
        if (screen->Initialized && screen->Console) {
            /* Restore console to normal mode */
            NuxConsoleSetMode(screen->Console, NUX_CONSOLE_MODE_NORMAL);
            NuxConsoleRelease(screen->Console);
        }
        free(screen);
    }

    return refCount;
}

/* ITuiScreen methods */
static HRESULT ANXAPI Screen_Initialize(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    NTSTATUS status;

    if (screen->Initialized) {
        return S_OK;
    }

    /* Get or create kernel console */
    status = NuxConsoleAcquire(&screen->Console);
    if (!NT_SUCCESS(status)) {
        return E_FAIL;
    }

    /* Set TUI mode (full screen, no scrolling) */
    status = NuxConsoleSetMode(screen->Console, NUX_CONSOLE_MODE_TUI);
    if (!NT_SUCCESS(status)) {
        NuxConsoleRelease(screen->Console);
        return E_FAIL;
    }

    /* Get console dimensions */
    status = NuxConsoleGetDimensions(screen->Console, &screen->Width, &screen->Height);
    if (!NT_SUCCESS(status)) {
        screen->Width = 80;
        screen->Height = 25;  /* Fallback to standard text mode */
    }

    /* Hide cursor */
    NuxConsoleSetCursorVisible(screen->Console, FALSE);

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

    if (Width == NULL || Height == NULL) {
        return E_POINTER;
    }

    if (!screen->Initialized) {
        return E_FAIL;
    }

    *Width = screen->Width;
    *Height = screen->Height;
    return S_OK;
}

static HRESULT ANXAPI Screen_Clear(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    NTSTATUS status;

    if (!screen->Initialized) {
        return E_FAIL;
    }

    /* Clear console screen */
    status = NuxConsoleClear(screen->Console);
    if (!NT_SUCCESS(status)) {
        return E_FAIL;
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
    NUX_COLOR fg, bg;
    NTSTATUS status;

    if (!screen->Initialized || Text == NULL) {
        return E_POINTER;
    }

    fg = MapColor(Fg);
    bg = MapColor(Bg);

    /* Set console colors */
    NuxConsoleSetColors(screen->Console, fg, bg);

    /* Write text at position */
    status = NuxConsoleWriteAt(screen->Console, X, Y, Text);
    if (!NT_SUCCESS(status)) {
        return E_FAIL;
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
    NUX_COLOR fg, bg;
    NTSTATUS status;

    if (!screen->Initialized) {
        return E_FAIL;
    }

    fg = MapColor(Fg);
    bg = MapColor(Bg);

    /* Use kernel console box drawing */
    status = NuxConsoleDrawBox(screen->Console, X, Y, Width, Height, fg, bg);
    if (!NT_SUCCESS(status)) {
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT ANXAPI Screen_GetKey(ITuiScreen *This, TUI_KEY *Key)
{
    TuiScreen *screen = (TuiScreen *)This;
    NUX_KEY nuxKey;
    NTSTATUS status;

    if (!screen->Initialized || Key == NULL) {
        return E_POINTER;
    }

    /* Read key from kernel keyboard driver */
    status = NuxKeyboardReadKey(&nuxKey, TRUE);  /* Blocking read */
    if (!NT_SUCCESS(status)) {
        return E_FAIL;
    }

    *Key = MapNuxKey(nuxKey);
    return S_OK;
}

static HRESULT ANXAPI Screen_Refresh(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    NTSTATUS status;

    if (!screen->Initialized) {
        return E_FAIL;
    }

    /* Flush framebuffer to screen */
    status = NuxConsoleFlush(screen->Console);
    if (!NT_SUCCESS(status)) {
        return E_FAIL;
    }

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

#endif /* __NUX__ */
