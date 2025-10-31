/*
 * widget_terminal.c - Terminal Emulator Widget
 *
 * Terminal widget with customizable renderer and parser for
 * ANSI/VT100 escape sequences and text processing.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_TERMINAL_COLS 132
#define MAX_TERMINAL_ROWS 60
#define MAX_SCROLLBACK 1000
#define ESC_BUFFER_SIZE 128

/* Terminal cell structure */
typedef struct {
    UINT32 Codepoint;      /* Unicode codepoint */
    TUI_COLOR Foreground;
    TUI_COLOR Background;
    UINT8 Attributes;      /* Bold, underline, etc. */
} TerminalCell;

/* Terminal emulation mode */
typedef enum {
    TerminalModeVT100,
    TerminalModeVT102,
    TerminalModeVT220,
    TerminalModeXTerm,
    TerminalModeLinux
} TerminalMode;

/* Parser state */
typedef enum {
    ParserStateNormal,
    ParserStateEscape,
    ParserStateCSI,          /* Control Sequence Introducer */
    ParserStateOSC,          /* Operating System Command */
    ParserStateDCS           /* Device Control String */
} ParserState;

/* Custom renderer callback */
typedef HRESULT (*TerminalRenderCallback)(
    VOID *UserData,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    CONST TerminalCell *Cell
);

/* Custom parser callback */
typedef HRESULT (*TerminalParserCallback)(
    VOID *UserData,
    CONST CHAR8 *Sequence,
    UINTN Length
);

typedef struct {
    ITuiTerminal Interface;
    WIDGET_STATE State;

    /* Terminal dimensions */
    UINT32 Cols;
    UINT32 Rows;

    /* Display buffer */
    TerminalCell **Buffer;      /* [Rows][Cols] */
    TerminalCell **Scrollback;  /* [ScrollbackLines][Cols] */
    UINT32 ScrollbackLines;
    UINT32 ScrollbackMax;

    /* Cursor state */
    INT32 CursorX;
    INT32 CursorY;
    BOOLEAN CursorVisible;

    /* Current text attributes */
    TUI_COLOR CurrentForeground;
    TUI_COLOR CurrentBackground;
    UINT8 CurrentAttributes;

    /* Parser state */
    ParserState ParseState;
    CHAR8 EscapeBuffer[ESC_BUFFER_SIZE];
    UINTN EscapeBufferLen;

    /* Terminal mode */
    TerminalMode Mode;

    /* Scrolling */
    INT32 ViewportTop;     /* Top line of viewport in scrollback */

    /* Custom callbacks */
    TerminalRenderCallback CustomRenderer;
    VOID *RendererUserData;
    TerminalParserCallback CustomParser;
    VOID *ParserUserData;

    /* Input callback */
    HRESULT (*OnInput)(VOID *UserData, CONST CHAR8 *Input, UINTN Length);
    VOID *InputUserData;

} TuiTerminalImpl;

/* Helper: Allocate buffer */
static TerminalCell **AllocateBuffer(UINT32 Rows, UINT32 Cols)
{
    TerminalCell **buffer = (TerminalCell **)malloc(Rows * sizeof(TerminalCell *));
    if (!buffer) return NULL;

    for (UINT32 i = 0; i < Rows; i++) {
        buffer[i] = (TerminalCell *)calloc(Cols, sizeof(TerminalCell));
        if (!buffer[i]) {
            /* Cleanup on failure */
            for (UINT32 j = 0; j < i; j++) {
                free(buffer[j]);
            }
            free(buffer);
            return NULL;
        }

        /* Initialize cells */
        for (UINT32 j = 0; j < Cols; j++) {
            buffer[i][j].Codepoint = ' ';
            buffer[i][j].Foreground = TuiColorWhite;
            buffer[i][j].Background = TuiColorBlack;
            buffer[i][j].Attributes = 0;
        }
    }

    return buffer;
}

/* Helper: Free buffer */
static VOID FreeBuffer(TerminalCell **Buffer, UINT32 Rows)
{
    if (!Buffer) return;

    for (UINT32 i = 0; i < Rows; i++) {
        if (Buffer[i]) {
            free(Buffer[i]);
        }
    }
    free(Buffer);
}

/* Helper: Clear screen */
static VOID ClearScreen(TuiTerminalImpl *impl)
{
    for (UINT32 y = 0; y < impl->Rows; y++) {
        for (UINT32 x = 0; x < impl->Cols; x++) {
            impl->Buffer[y][x].Codepoint = ' ';
            impl->Buffer[y][x].Foreground = impl->CurrentForeground;
            impl->Buffer[y][x].Background = impl->CurrentBackground;
            impl->Buffer[y][x].Attributes = 0;
        }
    }
    impl->CursorX = 0;
    impl->CursorY = 0;
}

/* Helper: Scroll up one line */
static VOID ScrollUp(TuiTerminalImpl *impl)
{
    /* Move top line to scrollback if there's room */
    if (impl->ScrollbackLines < impl->ScrollbackMax) {
        /* Allocate new scrollback line */
        TerminalCell *newLine = (TerminalCell *)malloc(impl->Cols * sizeof(TerminalCell));
        if (newLine) {
            memcpy(newLine, impl->Buffer[0], impl->Cols * sizeof(TerminalCell));
            impl->Scrollback[impl->ScrollbackLines++] = newLine;
        }
    }

    /* Shift all lines up */
    TerminalCell *firstLine = impl->Buffer[0];
    for (UINT32 y = 0; y < impl->Rows - 1; y++) {
        impl->Buffer[y] = impl->Buffer[y + 1];
    }

    /* Clear last line */
    impl->Buffer[impl->Rows - 1] = firstLine;
    for (UINT32 x = 0; x < impl->Cols; x++) {
        firstLine[x].Codepoint = ' ';
        firstLine[x].Foreground = impl->CurrentForeground;
        firstLine[x].Background = impl->CurrentBackground;
        firstLine[x].Attributes = 0;
    }
}

/* Helper: Put character at cursor position */
static VOID PutChar(TuiTerminalImpl *impl, UINT32 Codepoint)
{
    if (impl->CursorY >= impl->Rows) {
        impl->CursorY = impl->Rows - 1;
        ScrollUp(impl);
    }

    if (impl->CursorX >= impl->Cols) {
        impl->CursorX = 0;
        impl->CursorY++;
        if (impl->CursorY >= impl->Rows) {
            impl->CursorY = impl->Rows - 1;
            ScrollUp(impl);
        }
    }

    TerminalCell *cell = &impl->Buffer[impl->CursorY][impl->CursorX];
    cell->Codepoint = Codepoint;
    cell->Foreground = impl->CurrentForeground;
    cell->Background = impl->CurrentBackground;
    cell->Attributes = impl->CurrentAttributes;

    impl->CursorX++;
}

/* Helper: Process CSI sequence (ANSI escape codes) */
static VOID ProcessCSI(TuiTerminalImpl *impl, CONST CHAR8 *Sequence)
{
    /* Parse CSI parameters */
    INT32 params[16];
    INT32 paramCount = 0;
    CONST CHAR8 *p = Sequence;

    /* Skip CSI prefix */
    if (*p == '[') p++;

    /* Parse numeric parameters */
    while (*p && paramCount < 16) {
        if (isdigit(*p)) {
            params[paramCount++] = atoi(p);
            while (isdigit(*p)) p++;
        }
        if (*p == ';') p++;
        if (!isdigit(*p)) break;
    }

    /* Get command character */
    CHAR8 cmd = *p;

    switch (cmd) {
        case 'H': /* Cursor Position */
        case 'f': /* Horizontal and Vertical Position */
            impl->CursorY = (paramCount > 0 && params[0] > 0) ? params[0] - 1 : 0;
            impl->CursorX = (paramCount > 1 && params[1] > 0) ? params[1] - 1 : 0;
            if (impl->CursorY >= impl->Rows) impl->CursorY = impl->Rows - 1;
            if (impl->CursorX >= impl->Cols) impl->CursorX = impl->Cols - 1;
            break;

        case 'A': /* Cursor Up */
            impl->CursorY -= (paramCount > 0 && params[0] > 0) ? params[0] : 1;
            if (impl->CursorY < 0) impl->CursorY = 0;
            break;

        case 'B': /* Cursor Down */
            impl->CursorY += (paramCount > 0 && params[0] > 0) ? params[0] : 1;
            if (impl->CursorY >= impl->Rows) impl->CursorY = impl->Rows - 1;
            break;

        case 'C': /* Cursor Forward */
            impl->CursorX += (paramCount > 0 && params[0] > 0) ? params[0] : 1;
            if (impl->CursorX >= impl->Cols) impl->CursorX = impl->Cols - 1;
            break;

        case 'D': /* Cursor Backward */
            impl->CursorX -= (paramCount > 0 && params[0] > 0) ? params[0] : 1;
            if (impl->CursorX < 0) impl->CursorX = 0;
            break;

        case 'J': /* Erase Display */
            if (paramCount == 0 || params[0] == 0) {
                /* Clear from cursor to end of screen */
                for (UINT32 x = impl->CursorX; x < impl->Cols; x++) {
                    impl->Buffer[impl->CursorY][x].Codepoint = ' ';
                }
                for (UINT32 y = impl->CursorY + 1; y < impl->Rows; y++) {
                    for (UINT32 x = 0; x < impl->Cols; x++) {
                        impl->Buffer[y][x].Codepoint = ' ';
                    }
                }
            } else if (params[0] == 2) {
                /* Clear entire screen */
                ClearScreen(impl);
            }
            break;

        case 'K': /* Erase Line */
            if (paramCount == 0 || params[0] == 0) {
                /* Clear from cursor to end of line */
                for (UINT32 x = impl->CursorX; x < impl->Cols; x++) {
                    impl->Buffer[impl->CursorY][x].Codepoint = ' ';
                }
            } else if (params[0] == 2) {
                /* Clear entire line */
                for (UINT32 x = 0; x < impl->Cols; x++) {
                    impl->Buffer[impl->CursorY][x].Codepoint = ' ';
                }
            }
            break;

        case 'm': /* Select Graphic Rendition (SGR) */
            for (INT32 i = 0; i < paramCount; i++) {
                INT32 param = params[i];
                if (param == 0) {
                    /* Reset all attributes */
                    impl->CurrentForeground = TuiColorWhite;
                    impl->CurrentBackground = TuiColorBlack;
                    impl->CurrentAttributes = 0;
                } else if (param == 1) {
                    impl->CurrentAttributes |= TuiAttrBold;
                } else if (param == 4) {
                    impl->CurrentAttributes |= TuiAttrUnderline;
                } else if (param == 7) {
                    impl->CurrentAttributes |= TuiAttrReverse;
                } else if (param >= 30 && param <= 37) {
                    /* Foreground colors */
                    impl->CurrentForeground = (TUI_COLOR)(param - 30);
                } else if (param >= 40 && param <= 47) {
                    /* Background colors */
                    impl->CurrentBackground = (TUI_COLOR)(param - 40);
                } else if (param >= 90 && param <= 97) {
                    /* Bright foreground colors */
                    impl->CurrentForeground = (TUI_COLOR)(param - 90 + 8);
                } else if (param >= 100 && param <= 107) {
                    /* Bright background colors */
                    impl->CurrentBackground = (TUI_COLOR)(param - 100 + 8);
                }
            }
            break;
    }
}

/* Helper: Parse input text */
static VOID ParseInput(TuiTerminalImpl *impl, CONST CHAR8 *Text, UINTN Length)
{
    for (UINTN i = 0; i < Length; i++) {
        CHAR8 ch = Text[i];

        switch (impl->ParseState) {
            case ParserStateNormal:
                if (ch == '\x1B') {  /* ESC */
                    impl->ParseState = ParserStateEscape;
                    impl->EscapeBufferLen = 0;
                } else if (ch == '\n') {
                    impl->CursorX = 0;
                    impl->CursorY++;
                    if (impl->CursorY >= impl->Rows) {
                        impl->CursorY = impl->Rows - 1;
                        ScrollUp(impl);
                    }
                } else if (ch == '\r') {
                    impl->CursorX = 0;
                } else if (ch == '\t') {
                    impl->CursorX = (impl->CursorX + 8) & ~7;  /* Tab to next 8-column boundary */
                } else if (ch == '\b') {
                    if (impl->CursorX > 0) impl->CursorX--;
                } else if (ch >= 32) {
                    PutChar(impl, ch);
                }
                break;

            case ParserStateEscape:
                if (ch == '[') {
                    impl->ParseState = ParserStateCSI;
                    impl->EscapeBuffer[impl->EscapeBufferLen++] = ch;
                } else if (ch == ']') {
                    impl->ParseState = ParserStateOSC;
                    impl->EscapeBuffer[impl->EscapeBufferLen++] = ch;
                } else {
                    /* Unknown escape sequence, ignore */
                    impl->ParseState = ParserStateNormal;
                }
                break;

            case ParserStateCSI:
                impl->EscapeBuffer[impl->EscapeBufferLen++] = ch;
                if (isalpha(ch) || impl->EscapeBufferLen >= ESC_BUFFER_SIZE - 1) {
                    /* End of CSI sequence */
                    impl->EscapeBuffer[impl->EscapeBufferLen] = '\0';

                    /* Call custom parser if available */
                    if (impl->CustomParser) {
                        impl->CustomParser(impl->ParserUserData, impl->EscapeBuffer, impl->EscapeBufferLen);
                    } else {
                        /* Default processing */
                        ProcessCSI(impl, impl->EscapeBuffer);
                    }

                    impl->ParseState = ParserStateNormal;
                    impl->EscapeBufferLen = 0;
                }
                break;

            case ParserStateOSC:
                /* OSC sequences end with BEL or ST */
                if (ch == '\x07' || ch == '\x9C') {
                    impl->ParseState = ParserStateNormal;
                    impl->EscapeBufferLen = 0;
                } else if (impl->EscapeBufferLen < ESC_BUFFER_SIZE - 1) {
                    impl->EscapeBuffer[impl->EscapeBufferLen++] = ch;
                }
                break;

            case ParserStateDCS:
                /* Not implemented yet */
                impl->ParseState = ParserStateNormal;
                break;
        }
    }
}

/* IUnknown methods */
static HRESULT ANXAPI Terminal_QueryInterface(
    ITuiTerminal *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiTerminal)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Terminal_AddRef(ITuiTerminal *This)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Terminal_Release(ITuiTerminal *This)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        FreeBuffer(impl->Buffer, impl->Rows);
        FreeBuffer(impl->Scrollback, impl->ScrollbackLines);
        free(impl);
    }

    return count;
}

/* Render the terminal */
static HRESULT ANXAPI Terminal_Render(
    ITuiTerminal *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    BOOLEAN Focused
)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;

    if (!impl->State.Visible) return S_OK;

    /* Render each cell */
    for (UINT32 row = 0; row < impl->Rows && row < impl->State.Bounds.Height; row++) {
        for (UINT32 col = 0; col < impl->Cols && col < impl->State.Bounds.Width; col++) {
            TerminalCell *cell = &impl->Buffer[row][col];

            if (impl->CustomRenderer) {
                /* Use custom renderer */
                impl->CustomRenderer(impl->RendererUserData, Screen, X + col, Y + row, cell);
            } else {
                /* Default rendering */
                TUI_COLOR fg = cell->Foreground;
                TUI_COLOR bg = cell->Background;

                /* Apply reverse attribute */
                if (cell->Attributes & TuiAttrReverse) {
                    TUI_COLOR temp = fg;
                    fg = bg;
                    bg = temp;
                }

                Screen->Vtbl->WriteChar(Screen, X + col, Y + row, cell->Codepoint, fg, bg);
            }
        }
    }

    /* Render cursor if visible and focused */
    if (Focused && impl->CursorVisible &&
        impl->CursorX >= 0 && impl->CursorX < impl->Cols &&
        impl->CursorY >= 0 && impl->CursorY < impl->Rows) {
        TerminalCell *cell = &impl->Buffer[impl->CursorY][impl->CursorX];
        Screen->Vtbl->WriteChar(Screen, X + impl->CursorX, Y + impl->CursorY,
                               cell->Codepoint, cell->Background, cell->Foreground);
    }

    return S_OK;
}

/* Handle keyboard input */
static HRESULT ANXAPI Terminal_HandleKey(
    ITuiTerminal *This,
    TUI_KEY Key
)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    CHAR8 buffer[16];
    UINTN len = 0;

    /* Convert key to input string */
    if (Key >= 32 && Key < 127) {
        buffer[len++] = (CHAR8)Key;
    } else {
        switch (Key) {
            case TuiKeyEnter:
                buffer[len++] = '\r';
                break;
            case TuiKeyBackspace:
                buffer[len++] = '\b';
                break;
            case TuiKeyTab:
                buffer[len++] = '\t';
                break;
            case TuiKeyUp:
                buffer[len++] = '\x1B';
                buffer[len++] = '[';
                buffer[len++] = 'A';
                break;
            case TuiKeyDown:
                buffer[len++] = '\x1B';
                buffer[len++] = '[';
                buffer[len++] = 'B';
                break;
            case TuiKeyRight:
                buffer[len++] = '\x1B';
                buffer[len++] = '[';
                buffer[len++] = 'C';
                break;
            case TuiKeyLeft:
                buffer[len++] = '\x1B';
                buffer[len++] = '[';
                buffer[len++] = 'D';
                break;
        }
    }

    /* Send to input callback if available */
    if (len > 0 && impl->OnInput) {
        impl->OnInput(impl->InputUserData, buffer, len);
    }

    return S_OK;
}

/* Write text to terminal */
static HRESULT ANXAPI Terminal_WriteText(
    ITuiTerminal *This,
    CONST CHAR8 *Text,
    UINTN Length
)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    if (!Text) return E_INVALIDARG;

    if (Length == 0) {
        Length = strlen(Text);
    }

    ParseInput(impl, Text, Length);
    return S_OK;
}

/* Clear terminal */
static HRESULT ANXAPI Terminal_Clear(ITuiTerminal *This)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    ClearScreen(impl);
    return S_OK;
}

/* Set terminal size */
static HRESULT ANXAPI Terminal_SetSize(
    ITuiTerminal *This,
    UINT32 Cols,
    UINT32 Rows
)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;

    if (Cols == 0 || Rows == 0 || Cols > MAX_TERMINAL_COLS || Rows > MAX_TERMINAL_ROWS) {
        return E_INVALIDARG;
    }

    /* Free old buffer */
    FreeBuffer(impl->Buffer, impl->Rows);

    /* Allocate new buffer */
    impl->Cols = Cols;
    impl->Rows = Rows;
    impl->Buffer = AllocateBuffer(Rows, Cols);
    if (!impl->Buffer) {
        return E_OUTOFMEMORY;
    }

    impl->CursorX = 0;
    impl->CursorY = 0;

    return S_OK;
}

/* Set custom renderer */
static HRESULT ANXAPI Terminal_SetRenderer(
    ITuiTerminal *This,
    TerminalRenderCallback Renderer,
    VOID *UserData
)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    impl->CustomRenderer = Renderer;
    impl->RendererUserData = UserData;
    return S_OK;
}

/* Set custom parser */
static HRESULT ANXAPI Terminal_SetParser(
    ITuiTerminal *This,
    TerminalParserCallback Parser,
    VOID *UserData
)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    impl->CustomParser = Parser;
    impl->ParserUserData = UserData;
    return S_OK;
}

/* Set input callback */
static HRESULT ANXAPI Terminal_SetInputCallback(
    ITuiTerminal *This,
    HRESULT (*Callback)(VOID *UserData, CONST CHAR8 *Input, UINTN Length),
    VOID *UserData
)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    impl->OnInput = Callback;
    impl->InputUserData = UserData;
    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI Terminal_SetBounds(
    ITuiTerminal *This,
    CONST TUI_RECT *Bounds
)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI Terminal_GetBounds(
    ITuiTerminal *This,
    TUI_RECT *Bounds
)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI Terminal_SetVisible(
    ITuiTerminal *This,
    BOOLEAN Visible
)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI Terminal_IsVisible(ITuiTerminal *This)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI Terminal_SetEnabled(
    ITuiTerminal *This,
    BOOLEAN Enabled
)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI Terminal_IsEnabled(ITuiTerminal *This)
{
    TuiTerminalImpl *impl = (TuiTerminalImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiTerminalVtbl TerminalVtbl = {
    Terminal_QueryInterface,
    Terminal_AddRef,
    Terminal_Release,
    Terminal_Render,
    Terminal_HandleKey,
    Terminal_SetBounds,
    Terminal_GetBounds,
    Terminal_SetVisible,
    Terminal_IsVisible,
    Terminal_SetEnabled,
    Terminal_IsEnabled,
    Terminal_WriteText,
    Terminal_Clear,
    Terminal_SetSize,
    Terminal_SetRenderer,
    Terminal_SetParser,
    Terminal_SetInputCallback
};

/* Factory function */
HRESULT AnxTuiCreateTerminal(
    UINT32 Cols,
    UINT32 Rows,
    ITuiTerminal **OutTerminal
)
{
    TuiTerminalImpl *impl;

    if (!OutTerminal) return E_INVALIDARG;
    if (Cols == 0 || Rows == 0 || Cols > MAX_TERMINAL_COLS || Rows > MAX_TERMINAL_ROWS) {
        return E_INVALIDARG;
    }

    impl = (TuiTerminalImpl *)malloc(sizeof(TuiTerminalImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiTerminalImpl));
    impl->Interface.Vtbl = &TerminalVtbl;
    InitWidgetState(&impl->State);

    impl->Cols = Cols;
    impl->Rows = Rows;
    impl->CursorVisible = TRUE;
    impl->CurrentForeground = TuiColorWhite;
    impl->CurrentBackground = TuiColorBlack;
    impl->ParseState = ParserStateNormal;
    impl->Mode = TerminalModeXTerm;
    impl->ScrollbackMax = MAX_SCROLLBACK;

    /* Allocate display buffer */
    impl->Buffer = AllocateBuffer(Rows, Cols);
    if (!impl->Buffer) {
        free(impl);
        return E_OUTOFMEMORY;
    }

    /* Allocate scrollback buffer */
    impl->Scrollback = (TerminalCell **)calloc(MAX_SCROLLBACK, sizeof(TerminalCell *));
    if (!impl->Scrollback) {
        FreeBuffer(impl->Buffer, Rows);
        free(impl);
        return E_OUTOFMEMORY;
    }

    impl->State.Bounds.Width = Cols;
    impl->State.Bounds.Height = Rows;

    *OutTerminal = &impl->Interface;
    return S_OK;
}
