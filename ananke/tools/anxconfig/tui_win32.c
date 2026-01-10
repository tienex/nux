/*
 * Win32 Console TUI Backend
 *
 * Implements the Ananke TUI interfaces using Windows Console API.
 * Used on Windows NT/2000/XP/Vista/7/8/10/11 systems.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ananke/tui.h>
#include <ananke/anxconfig.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/* TuiScreen structure */
typedef struct {
    ITuiScreen Interface;
    UINTN RefCount;
    BOOLEAN Initialized;
    HANDLE StdOut;
    HANDLE StdIn;
    CONSOLE_SCREEN_BUFFER_INFO OriginalInfo;
    DWORD OriginalMode;
} TuiScreen;

/* Color mapping to Windows console attributes */
static WORD MapColor(TUI_COLOR Fg, TUI_COLOR Bg)
{
    WORD fg = 0, bg = 0;

    switch (Fg) {
        case TUI_COLOR_BLACK:   fg = 0; break;
        case TUI_COLOR_BLUE:    fg = FOREGROUND_BLUE; break;
        case TUI_COLOR_GREEN:   fg = FOREGROUND_GREEN; break;
        case TUI_COLOR_CYAN:    fg = FOREGROUND_BLUE | FOREGROUND_GREEN; break;
        case TUI_COLOR_RED:     fg = FOREGROUND_RED; break;
        case TUI_COLOR_MAGENTA: fg = FOREGROUND_RED | FOREGROUND_BLUE; break;
        case TUI_COLOR_YELLOW:  fg = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case TUI_COLOR_WHITE:   fg = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        default:                fg = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
    }

    switch (Bg) {
        case TUI_COLOR_BLACK:   bg = 0; break;
        case TUI_COLOR_BLUE:    bg = BACKGROUND_BLUE; break;
        case TUI_COLOR_GREEN:   bg = BACKGROUND_GREEN; break;
        case TUI_COLOR_CYAN:    bg = BACKGROUND_BLUE | BACKGROUND_GREEN; break;
        case TUI_COLOR_RED:     bg = BACKGROUND_RED; break;
        case TUI_COLOR_MAGENTA: bg = BACKGROUND_RED | BACKGROUND_BLUE; break;
        case TUI_COLOR_YELLOW:  bg = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY; break;
        case TUI_COLOR_WHITE:   bg = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; break;
        default:                bg = 0; break;
    }

    return fg | bg;
}

/* Key mapping from Windows virtual key codes */
static TUI_KEY MapVirtualKey(WORD vkCode, CHAR ch)
{
    /* ASCII characters */
    if (ch >= 32 && ch <= 126) {
        return (TUI_KEY)ch;
    }

    switch (ch) {
        case 13:  return TUI_KEY_ENTER;
        case 27:  return TUI_KEY_ESC;
        case 8:   return TUI_KEY_BACKSPACE;
        case 9:   return TUI_KEY_TAB;
    }

    /* Virtual key codes */
    switch (vkCode) {
        case VK_UP:       return TUI_KEY_UP;
        case VK_DOWN:     return TUI_KEY_DOWN;
        case VK_LEFT:     return TUI_KEY_LEFT;
        case VK_RIGHT:    return TUI_KEY_RIGHT;
        case VK_F1:       return TUI_KEY_F1;
        case VK_F2:       return TUI_KEY_F2;
        case VK_F3:       return TUI_KEY_F3;
        case VK_F4:       return TUI_KEY_F4;
        case VK_F5:       return TUI_KEY_F5;
        case VK_F6:       return TUI_KEY_F6;
        case VK_F7:       return TUI_KEY_F7;
        case VK_F8:       return TUI_KEY_F8;
        case VK_F9:       return TUI_KEY_F9;
        case VK_F10:      return TUI_KEY_F10;
        case VK_F11:      return TUI_KEY_F11;
        case VK_F12:      return TUI_KEY_F12;
        case VK_DELETE:   return TUI_KEY_DELETE;
        case VK_INSERT:   return TUI_KEY_INSERT;
        case VK_HOME:     return TUI_KEY_HOME;
        case VK_END:      return TUI_KEY_END;
        case VK_PRIOR:    return TUI_KEY_PAGEUP;
        case VK_NEXT:     return TUI_KEY_PAGEDOWN;
        default:          return TUI_KEY_UNKNOWN;
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
            /* Restore original console mode and cursor */
            SetConsoleMode(screen->StdIn, screen->OriginalMode);
            SetConsoleTextAttribute(screen->StdOut, screen->OriginalInfo.wAttributes);

            CONSOLE_CURSOR_INFO cursorInfo;
            cursorInfo.dwSize = 25;
            cursorInfo.bVisible = TRUE;
            SetConsoleCursorInfo(screen->StdOut, &cursorInfo);
        }
        free(screen);
    }

    return refCount;
}

/* ITuiScreen methods */
static HRESULT ANXAPI Screen_Initialize(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    CONSOLE_CURSOR_INFO cursorInfo;
    DWORD mode;

    if (screen->Initialized) {
        return S_OK;
    }

    /* Get console handles */
    screen->StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    screen->StdIn = GetStdHandle(STD_INPUT_HANDLE);

    if (screen->StdOut == INVALID_HANDLE_VALUE ||
        screen->StdIn == INVALID_HANDLE_VALUE) {
        return E_FAIL;
    }

    /* Save original console state */
    GetConsoleScreenBufferInfo(screen->StdOut, &screen->OriginalInfo);
    GetConsoleMode(screen->StdIn, &screen->OriginalMode);

    /* Set console mode for raw input */
    mode = ENABLE_WINDOW_INPUT;
    SetConsoleMode(screen->StdIn, mode);

    /* Hide cursor */
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(screen->StdOut, &cursorInfo);

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
    CONSOLE_SCREEN_BUFFER_INFO info;

    if (Width == NULL || Height == NULL) {
        return E_POINTER;
    }

    if (!screen->Initialized) {
        return E_FAIL;
    }

    GetConsoleScreenBufferInfo(screen->StdOut, &info);
    *Width = info.srWindow.Right - info.srWindow.Left + 1;
    *Height = info.srWindow.Bottom - info.srWindow.Top + 1;

    return S_OK;
}

static HRESULT ANXAPI Screen_Clear(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;
    CONSOLE_SCREEN_BUFFER_INFO info;
    DWORD written;
    COORD topLeft = {0, 0};

    if (!screen->Initialized) {
        return E_FAIL;
    }

    GetConsoleScreenBufferInfo(screen->StdOut, &info);

    /* Fill console with spaces */
    FillConsoleOutputCharacterA(screen->StdOut, ' ',
        info.dwSize.X * info.dwSize.Y, topLeft, &written);

    FillConsoleOutputAttribute(screen->StdOut,
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        info.dwSize.X * info.dwSize.Y, topLeft, &written);

    /* Reset cursor position */
    SetConsoleCursorPosition(screen->StdOut, topLeft);

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
    COORD pos;
    WORD attr;
    DWORD written;

    if (!screen->Initialized || Text == NULL) {
        return E_POINTER;
    }

    pos.X = (SHORT)X;
    pos.Y = (SHORT)Y;
    attr = MapColor(Fg, Bg);

    /* Set cursor position */
    SetConsoleCursorPosition(screen->StdOut, pos);

    /* Set text attributes */
    SetConsoleTextAttribute(screen->StdOut, attr);

    /* Write text */
    WriteConsoleA(screen->StdOut, Text, (DWORD)strlen(Text), &written, NULL);

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
    COORD pos;
    WORD attr;
    DWORD written;
    UINT32 i;
    char line[256];

    if (!screen->Initialized) {
        return E_FAIL;
    }

    attr = MapColor(Fg, Bg);
    SetConsoleTextAttribute(screen->StdOut, attr);

    /* Draw top border */
    pos.X = (SHORT)X;
    pos.Y = (SHORT)Y;
    SetConsoleCursorPosition(screen->StdOut, pos);
    line[0] = '+';
    for (i = 1; i < Width - 1; i++) {
        line[i] = '-';
    }
    line[Width - 1] = '+';
    line[Width] = '\0';
    WriteConsoleA(screen->StdOut, line, Width, &written, NULL);

    /* Draw sides */
    for (i = 1; i < Height - 1; i++) {
        pos.X = (SHORT)X;
        pos.Y = (SHORT)(Y + i);
        SetConsoleCursorPosition(screen->StdOut, pos);
        WriteConsoleA(screen->StdOut, "|", 1, &written, NULL);

        pos.X = (SHORT)(X + Width - 1);
        SetConsoleCursorPosition(screen->StdOut, pos);
        WriteConsoleA(screen->StdOut, "|", 1, &written, NULL);
    }

    /* Draw bottom border */
    pos.X = (SHORT)X;
    pos.Y = (SHORT)(Y + Height - 1);
    SetConsoleCursorPosition(screen->StdOut, pos);
    WriteConsoleA(screen->StdOut, line, Width, &written, NULL);

    return S_OK;
}

static HRESULT ANXAPI Screen_GetKey(ITuiScreen *This, TUI_KEY *Key)
{
    TuiScreen *screen = (TuiScreen *)This;
    INPUT_RECORD inputRecord;
    DWORD eventsRead;

    if (!screen->Initialized || Key == NULL) {
        return E_POINTER;
    }

    /* Read console input */
    while (1) {
        ReadConsoleInput(screen->StdIn, &inputRecord, 1, &eventsRead);

        if (inputRecord.EventType == KEY_EVENT &&
            inputRecord.Event.KeyEvent.bKeyDown) {
            *Key = MapVirtualKey(
                inputRecord.Event.KeyEvent.wVirtualKeyCode,
                inputRecord.Event.KeyEvent.uChar.AsciiChar);
            return S_OK;
        }
    }

    return E_FAIL;
}

static HRESULT ANXAPI Screen_Refresh(ITuiScreen *This)
{
    /* No explicit refresh needed for Windows console */
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

#endif /* _WIN32 */
