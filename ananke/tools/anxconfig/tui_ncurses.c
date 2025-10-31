/*
 * Ncurses TUI Backend
 *
 * Implements the Ananke TUI interfaces using ncurses library.
 * Used on Unix/Linux systems with terminal support.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <ananke/tui.h>
#include <ananke/anxconfig.h>

/* TuiScreen structure */
typedef struct {
    ITuiScreen Interface;
    UINTN RefCount;
    BOOLEAN Initialized;
} TuiScreen;

/* Color mapping */
static int MapColor(TUI_COLOR Color)
{
    switch (Color) {
        case TUI_COLOR_BLACK:   return COLOR_BLACK;
        case TUI_COLOR_BLUE:    return COLOR_BLUE;
        case TUI_COLOR_GREEN:   return COLOR_GREEN;
        case TUI_COLOR_CYAN:    return COLOR_CYAN;
        case TUI_COLOR_RED:     return COLOR_RED;
        case TUI_COLOR_MAGENTA: return COLOR_MAGENTA;
        case TUI_COLOR_YELLOW:  return COLOR_YELLOW;
        case TUI_COLOR_WHITE:   return COLOR_WHITE;
        default:                return COLOR_WHITE;
    }
}

/* Key mapping */
static TUI_KEY MapKey(int ch)
{
    switch (ch) {
        case KEY_UP:    return TUI_KEY_UP;
        case KEY_DOWN:  return TUI_KEY_DOWN;
        case KEY_LEFT:  return TUI_KEY_LEFT;
        case KEY_RIGHT: return TUI_KEY_RIGHT;
        case 10:        /* LF */
        case 13:        /* CR */
        case KEY_ENTER: return TUI_KEY_ENTER;
        case 27:        /* ESC */
            return TUI_KEY_ESC;
        case KEY_F(1):  return TUI_KEY_F1;
        case KEY_F(2):  return TUI_KEY_F2;
        case KEY_F(3):  return TUI_KEY_F3;
        case KEY_F(4):  return TUI_KEY_F4;
        case KEY_F(5):  return TUI_KEY_F5;
        case KEY_F(6):  return TUI_KEY_F6;
        case KEY_F(7):  return TUI_KEY_F7;
        case KEY_F(8):  return TUI_KEY_F8;
        case KEY_F(9):  return TUI_KEY_F9;
        case KEY_F(10): return TUI_KEY_F10;
        case KEY_F(11): return TUI_KEY_F11;
        case KEY_F(12): return TUI_KEY_F12;
        case KEY_BACKSPACE:
        case 127:       /* DEL */
        case 8:         /* BS */
            return TUI_KEY_BACKSPACE;
        case KEY_DC:    return TUI_KEY_DELETE;
        case KEY_IC:    return TUI_KEY_INSERT;
        case KEY_HOME:  return TUI_KEY_HOME;
        case KEY_END:   return TUI_KEY_END;
        case KEY_PPAGE: return TUI_KEY_PAGEUP;
        case KEY_NPAGE: return TUI_KEY_PAGEDOWN;
        case 9:         /* TAB */
            return TUI_KEY_TAB;
        default:
            if (ch >= 32 && ch <= 126) {
                return (TUI_KEY)ch;
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
        if (screen->Initialized) {
            endwin();
        }
        free(screen);
    }

    return refCount;
}

/* ITuiScreen methods */
static HRESULT ANXAPI Screen_Initialize(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;

    if (screen->Initialized) {
        return S_OK;
    }

    /* Initialize ncurses */
    initscr();
    cbreak();           /* Disable line buffering */
    noecho();           /* Don't echo input */
    keypad(stdscr, TRUE); /* Enable function keys */
    curs_set(0);        /* Hide cursor */

    /* Initialize colors if supported */
    if (has_colors()) {
        start_color();

        /* Define color pairs (foreground, background) */
        init_pair(1, COLOR_WHITE, COLOR_BLUE);   /* Menu bar */
        init_pair(2, COLOR_BLACK, COLOR_CYAN);   /* Selected item */
        init_pair(3, COLOR_WHITE, COLOR_BLACK);  /* Normal text */
        init_pair(4, COLOR_YELLOW, COLOR_BLUE);  /* Highlighted text */
    }

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

    *Height = LINES;
    *Width = COLS;
    return S_OK;
}

static HRESULT ANXAPI Screen_Clear(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;

    if (!screen->Initialized) {
        return E_FAIL;
    }

    clear();
    refresh();
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

    if (!screen->Initialized || Text == NULL) {
        return E_POINTER;
    }

    /* Set colors if supported */
    if (has_colors()) {
        int fg = MapColor(Fg);
        int bg = MapColor(Bg);

        /* Use predefined color pairs or default */
        if (fg == COLOR_WHITE && bg == COLOR_BLUE) {
            attron(COLOR_PAIR(1));
        } else if (fg == COLOR_BLACK && bg == COLOR_CYAN) {
            attron(COLOR_PAIR(2));
        } else if (fg == COLOR_YELLOW && bg == COLOR_BLUE) {
            attron(COLOR_PAIR(4));
        } else {
            attron(COLOR_PAIR(3));
        }
    }

    /* Write text at position */
    mvaddstr(Y, X, Text);

    if (has_colors()) {
        attroff(COLOR_PAIR(1) | COLOR_PAIR(2) | COLOR_PAIR(3) | COLOR_PAIR(4));
    }

    refresh();
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
    UINT32 i;

    if (!screen->Initialized) {
        return E_FAIL;
    }

    /* Draw top border */
    mvaddch(Y, X, ACS_ULCORNER);
    for (i = 1; i < Width - 1; i++) {
        mvaddch(Y, X + i, ACS_HLINE);
    }
    mvaddch(Y, X + Width - 1, ACS_URCORNER);

    /* Draw sides */
    for (i = 1; i < Height - 1; i++) {
        mvaddch(Y + i, X, ACS_VLINE);
        mvaddch(Y + i, X + Width - 1, ACS_VLINE);
    }

    /* Draw bottom border */
    mvaddch(Y + Height - 1, X, ACS_LLCORNER);
    for (i = 1; i < Width - 1; i++) {
        mvaddch(Y + Height - 1, X + i, ACS_HLINE);
    }
    mvaddch(Y + Height - 1, X + Width - 1, ACS_LRCORNER);

    refresh();
    return S_OK;
}

static HRESULT ANXAPI Screen_GetKey(ITuiScreen *This, TUI_KEY *Key)
{
    TuiScreen *screen = (TuiScreen *)This;
    int ch;

    if (!screen->Initialized || Key == NULL) {
        return E_POINTER;
    }

    ch = getch();
    *Key = MapKey(ch);
    return S_OK;
}

static HRESULT ANXAPI Screen_Refresh(ITuiScreen *This)
{
    TuiScreen *screen = (TuiScreen *)This;

    if (!screen->Initialized) {
        return E_FAIL;
    }

    refresh();
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
