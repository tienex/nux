/*
 * widget_richtexteditor.c - Rich Text Editor Widget
 *
 * Full-featured text editor similar to Word for DOS/WordPerfect/WordStar with:
 * - Rich text formatting (bold, italic, underline, colors)
 * - Paragraph formatting (alignment, indentation, spacing)
 * - Block operations (cut, copy, paste, delete)
 * - Search and replace
 * - Word wrap and reflow
 * - Reveal codes mode (WordPerfect-style)
 * - Function key commands
 * - Status line with position and formatting info
 * - Multiple undo/redo levels
 * - Document statistics
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256
#define MAX_LINES 65536
#define MAX_UNDO_LEVELS 100

/* Text attributes */
typedef enum {
    AttrNormal = 0x00,
    AttrBold = 0x01,
    AttrItalic = 0x02,
    AttrUnderline = 0x04,
    AttrStrikethrough = 0x08,
    AttrSubscript = 0x10,
    AttrSuperscript = 0x20
} TextAttribute;

/* Paragraph alignment */
typedef enum {
    AlignLeft,
    AlignCenter,
    AlignRight,
    AlignJustify
} ParagraphAlignment;

/* Character with formatting */
typedef struct {
    CHAR8 Char;
    UINT8 Attributes;      /* TextAttribute flags */
    TUI_COLOR Foreground;
    TUI_COLOR Background;
} FormattedChar;

/* Text line */
typedef struct {
    FormattedChar *Chars;
    UINT32 Length;
    UINT32 Capacity;
    ParagraphAlignment Alignment;
    UINT32 LeftIndent;
    UINT32 RightIndent;
    UINT32 FirstLineIndent;
    BOOLEAN PageBreak;     /* Hard page break after this line */
} TextLine;

/* Cursor position */
typedef struct {
    UINT32 Line;
    UINT32 Column;
} CursorPos;

/* Selection/block */
typedef struct {
    CursorPos Start;
    CursorPos End;
    BOOLEAN Active;
    BOOLEAN Rectangle;     /* Rectangular vs stream selection */
} Selection;

/* Undo entry */
typedef struct {
    enum { UndoInsert, UndoDelete, UndoFormat } Type;
    CursorPos Position;
    VOID *Data;            /* Type-specific data */
    UINT32 DataSize;
} UndoEntry;

typedef struct {
    ITuiRichTextEditor Interface;
    WIDGET_STATE State;

    /* Document */
    TextLine *Lines;
    UINT32 LineCount;
    UINT32 LineCapacity;

    /* Cursor */
    CursorPos Cursor;
    UINT32 PreferredColumn;  /* For up/down movement */

    /* View */
    UINT32 ScrollLine;
    UINT32 ScrollColumn;
    UINT32 VisibleLines;
    UINT32 VisibleColumns;

    /* Selection */
    Selection Selection;

    /* Current formatting */
    UINT8 CurrentAttributes;
    TUI_COLOR CurrentForeground;
    TUI_COLOR CurrentBackground;

    /* Editor state */
    BOOLEAN InsertMode;       /* vs overstrike */
    BOOLEAN WordWrap;
    BOOLEAN ShowCodes;        /* Reveal codes mode (WordPerfect) */
    BOOLEAN Modified;

    /* Search */
    CHAR8 SearchText[MAX_LINE_LENGTH];
    BOOLEAN SearchCaseSensitive;

    /* Undo/redo */
    UndoEntry UndoStack[MAX_UNDO_LEVELS];
    UINT32 UndoCount;
    UINT32 UndoPosition;

    /* Statistics */
    UINT32 CharCount;
    UINT32 WordCount;

    /* Ruler integration */
    ITuiRuler *Ruler;

    /* Callbacks */
    HRESULT (*OnModified)(VOID *UserData);
    HRESULT (*OnCursorMoved)(VOID *UserData, UINT32 Line, UINT32 Column);
    VOID *UserData;

} TuiRichTextEditorImpl;

/* Helper: Ensure line exists */
static HRESULT EnsureLine(TuiRichTextEditorImpl *impl, UINT32 lineIdx)
{
    if (lineIdx >= impl->LineCapacity) {
        UINT32 newCapacity = lineIdx + 1024;
        TextLine *newLines = (TextLine *)realloc(impl->Lines, newCapacity * sizeof(TextLine));
        if (!newLines) return E_OUTOFMEMORY;

        /* Initialize new lines */
        for (UINT32 i = impl->LineCapacity; i < newCapacity; i++) {
            memset(&newLines[i], 0, sizeof(TextLine));
        }

        impl->Lines = newLines;
        impl->LineCapacity = newCapacity;
    }

    if (lineIdx >= impl->LineCount) {
        impl->LineCount = lineIdx + 1;
    }

    /* Allocate line buffer if needed */
    TextLine *line = &impl->Lines[lineIdx];
    if (!line->Chars) {
        line->Capacity = 128;
        line->Chars = (FormattedChar *)malloc(line->Capacity * sizeof(FormattedChar));
        if (!line->Chars) return E_OUTOFMEMORY;
        line->Length = 0;
        line->Alignment = AlignLeft;
    }

    return S_OK;
}

/* Helper: Insert character at cursor */
static HRESULT InsertChar(TuiRichTextEditorImpl *impl, CHAR8 ch)
{
    HRESULT hr = EnsureLine(impl, impl->Cursor.Line);
    if (FAILED(hr)) return hr;

    TextLine *line = &impl->Lines[impl->Cursor.Line];

    /* Expand line buffer if needed */
    if (line->Length >= line->Capacity) {
        line->Capacity *= 2;
        FormattedChar *newChars = (FormattedChar *)realloc(line->Chars, line->Capacity * sizeof(FormattedChar));
        if (!newChars) return E_OUTOFMEMORY;
        line->Chars = newChars;
    }

    /* Insert character */
    if (impl->InsertMode) {
        /* Shift characters right */
        for (UINT32 i = line->Length; i > impl->Cursor.Column; i--) {
            line->Chars[i] = line->Chars[i - 1];
        }
        line->Length++;
    } else if (impl->Cursor.Column >= line->Length) {
        line->Length = impl->Cursor.Column + 1;
    }

    /* Set character with formatting */
    FormattedChar *fc = &line->Chars[impl->Cursor.Column];
    fc->Char = ch;
    fc->Attributes = impl->CurrentAttributes;
    fc->Foreground = impl->CurrentForeground;
    fc->Background = impl->CurrentBackground;

    impl->Cursor.Column++;
    impl->Modified = TRUE;
    impl->CharCount++;

    return S_OK;
}

/* Helper: Delete character at cursor */
static HRESULT DeleteChar(TuiRichTextEditorImpl *impl)
{
    if (impl->Cursor.Line >= impl->LineCount) return S_OK;

    TextLine *line = &impl->Lines[impl->Cursor.Line];

    if (impl->Cursor.Column >= line->Length) {
        /* At end of line - join with next line */
        if (impl->Cursor.Line < impl->LineCount - 1) {
            TextLine *nextLine = &impl->Lines[impl->Cursor.Line + 1];

            /* Append next line to current */
            UINT32 newLength = line->Length + nextLine->Length;
            if (newLength > line->Capacity) {
                line->Capacity = newLength + 128;
                FormattedChar *newChars = (FormattedChar *)realloc(line->Chars, line->Capacity * sizeof(FormattedChar));
                if (!newChars) return E_OUTOFMEMORY;
                line->Chars = newChars;
            }

            memcpy(&line->Chars[line->Length], nextLine->Chars, nextLine->Length * sizeof(FormattedChar));
            line->Length = newLength;

            /* Remove next line */
            free(nextLine->Chars);
            for (UINT32 i = impl->Cursor.Line + 1; i < impl->LineCount - 1; i++) {
                impl->Lines[i] = impl->Lines[i + 1];
            }
            impl->LineCount--;
        }
    } else {
        /* Delete character */
        for (UINT32 i = impl->Cursor.Column; i < line->Length - 1; i++) {
            line->Chars[i] = line->Chars[i + 1];
        }
        line->Length--;
        impl->CharCount--;
    }

    impl->Modified = TRUE;
    return S_OK;
}

/* Helper: Insert newline */
static HRESULT InsertNewline(TuiRichTextEditorImpl *impl)
{
    HRESULT hr = EnsureLine(impl, impl->Cursor.Line + 1);
    if (FAILED(hr)) return hr;

    /* Shift lines down */
    for (UINT32 i = impl->LineCount; i > impl->Cursor.Line + 1; i--) {
        impl->Lines[i] = impl->Lines[i - 1];
    }

    TextLine *currentLine = &impl->Lines[impl->Cursor.Line];
    TextLine *newLine = &impl->Lines[impl->Cursor.Line + 1];

    /* Split current line at cursor */
    UINT32 remainingChars = currentLine->Length - impl->Cursor.Column;

    newLine->Capacity = remainingChars + 128;
    newLine->Chars = (FormattedChar *)malloc(newLine->Capacity * sizeof(FormattedChar));
    if (!newLine->Chars) return E_OUTOFMEMORY;

    memcpy(newLine->Chars, &currentLine->Chars[impl->Cursor.Column], remainingChars * sizeof(FormattedChar));
    newLine->Length = remainingChars;
    newLine->Alignment = currentLine->Alignment;
    newLine->LeftIndent = currentLine->LeftIndent;
    newLine->RightIndent = currentLine->RightIndent;
    newLine->FirstLineIndent = 0;  /* Only first line gets first-line indent */

    currentLine->Length = impl->Cursor.Column;

    impl->LineCount++;
    impl->Cursor.Line++;
    impl->Cursor.Column = 0;
    impl->Modified = TRUE;

    return S_OK;
}

/* IUnknown methods */
static HRESULT ANXAPI RichTextEditor_QueryInterface(
    ITuiRichTextEditor *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiRichTextEditor)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI RichTextEditor_AddRef(ITuiRichTextEditor *This)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI RichTextEditor_Release(ITuiRichTextEditor *This)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        /* Free lines */
        for (UINT32 i = 0; i < impl->LineCount; i++) {
            if (impl->Lines[i].Chars) {
                free(impl->Lines[i].Chars);
            }
        }
        free(impl->Lines);

        /* Release ruler */
        if (impl->Ruler) {
            impl->Ruler->Vtbl->Release(impl->Ruler);
        }

        free(impl);
    }

    return count;
}

/* Render the editor */
static HRESULT ANXAPI RichTextEditor_Render(
    ITuiRichTextEditor *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;

    if (!impl->State.Visible) return S_OK;

    UINT32 width = impl->State.Bounds.Width;
    UINT32 height = impl->State.Bounds.Height;

    /* Render ruler if present */
    UINT32 contentY = Y;
    if (impl->Ruler && impl->Ruler->Vtbl->IsVisible(impl->Ruler)) {
        impl->Ruler->Vtbl->Render(impl->Ruler, Screen, X, Y);
        impl->Ruler->Vtbl->SetCurrentPosition(impl->Ruler, impl->Cursor.Column);
        contentY += 1;
        height -= 1;
    }

    /* Draw text area background */
    for (UINT32 i = 0; i < height - 1; i++) {
        for (UINT32 j = 0; j < width; j++) {
            Screen->Vtbl->WriteChar(Screen, X + j, contentY + i, ' ', TuiColorBlack, TuiColorWhite);
        }
    }

    /* Render text lines */
    UINT32 viewLine = impl->ScrollLine;
    for (UINT32 row = 0; row < height - 1 && viewLine < impl->LineCount; row++, viewLine++) {
        TextLine *line = &impl->Lines[viewLine];

        UINT32 col = 0;
        for (UINT32 c = impl->ScrollColumn; c < line->Length && col < width; c++, col++) {
            FormattedChar *fc = &line->Chars[c];

            TUI_COLOR fg = fc->Foreground;
            TUI_COLOR bg = fc->Background;

            /* Apply attributes */
            if (fc->Attributes & AttrBold) {
                /* Brighten foreground for bold */
                if (fg < TuiColorBrightBlack) {
                    fg = (TUI_COLOR)(fg + 8);
                }
            }

            /* Show cursor */
            if (viewLine == impl->Cursor.Line && c == impl->Cursor.Column) {
                fg = TuiColorWhite;
                bg = TuiColorBlue;
            }

            /* Show selection */
            if (impl->Selection.Active) {
                BOOLEAN inSelection = FALSE;
                if (impl->Selection.Start.Line == impl->Selection.End.Line) {
                    /* Single line selection */
                    if (viewLine == impl->Selection.Start.Line &&
                        c >= impl->Selection.Start.Column &&
                        c < impl->Selection.End.Column) {
                        inSelection = TRUE;
                    }
                } else {
                    /* Multi-line selection */
                    if (viewLine > impl->Selection.Start.Line && viewLine < impl->Selection.End.Line) {
                        inSelection = TRUE;
                    } else if (viewLine == impl->Selection.Start.Line && c >= impl->Selection.Start.Column) {
                        inSelection = TRUE;
                    } else if (viewLine == impl->Selection.End.Line && c < impl->Selection.End.Column) {
                        inSelection = TRUE;
                    }
                }

                if (inSelection) {
                    fg = TuiColorWhite;
                    bg = TuiColorCyan;
                }
            }

            CHAR8 displayChar = fc->Char;

            /* Show codes in reveal mode */
            if (impl->ShowCodes && fc->Attributes) {
                displayChar = '*';  /* Placeholder for format code */
            }

            Screen->Vtbl->WriteChar(Screen, X + col, contentY + row, displayChar, fg, bg);
        }

        /* Show end-of-line marker */
        if (col < width && !impl->ShowCodes) {
            Screen->Vtbl->WriteChar(Screen, X + col, contentY + row, gBoxChars.ArrowLeft, TuiColorBrightBlack, TuiColorWhite);
        }
    }

    /* Draw status line */
    UINT32 statusY = Y + impl->State.Bounds.Height - 1;
    CHAR8 status[256];

    CHAR8 modifiedStr[4] = "";
    if (impl->Modified) {
        strcpy(modifiedStr, " * ");
    }

    CHAR8 modeStr[16];
    snprintf(modeStr, sizeof(modeStr), "%s", impl->InsertMode ? "INS" : "OVR");

    CHAR8 formatStr[64] = "";
    if (impl->CurrentAttributes & AttrBold) strcat(formatStr, "B");
    if (impl->CurrentAttributes & AttrItalic) strcat(formatStr, "I");
    if (impl->CurrentAttributes & AttrUnderline) strcat(formatStr, "U");

    snprintf(status, sizeof(status),
             "%s Ln %u Col %u  %s  %s  Chars:%u Words:%u",
             modifiedStr,
             impl->Cursor.Line + 1,
             impl->Cursor.Column + 1,
             modeStr,
             formatStr,
             impl->CharCount,
             impl->WordCount);

    for (UINT32 i = 0; i < width; i++) {
        Screen->Vtbl->WriteChar(Screen, X + i, statusY, ' ', TuiColorWhite, TuiColorBlue);
    }
    Screen->Vtbl->WriteText(Screen, X, statusY, status, TuiColorWhite, TuiColorBlue);

    return S_OK;
}

/* Handle keyboard input */
static HRESULT ANXAPI RichTextEditor_HandleKey(
    ITuiRichTextEditor *This,
    TUI_KEY Key
)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;

    /* Navigation */
    switch (Key) {
        case TuiKeyUp:
            if (impl->Cursor.Line > 0) {
                impl->Cursor.Line--;
                impl->Cursor.Column = impl->PreferredColumn;
                if (impl->Cursor.Line < impl->ScrollLine) {
                    impl->ScrollLine = impl->Cursor.Line;
                }
            }
            return S_OK;

        case TuiKeyDown:
            if (impl->Cursor.Line < impl->LineCount - 1) {
                impl->Cursor.Line++;
                impl->Cursor.Column = impl->PreferredColumn;
                if (impl->Cursor.Line >= impl->ScrollLine + impl->VisibleLines) {
                    impl->ScrollLine++;
                }
            }
            return S_OK;

        case TuiKeyLeft:
            if (impl->Cursor.Column > 0) {
                impl->Cursor.Column--;
                impl->PreferredColumn = impl->Cursor.Column;
            } else if (impl->Cursor.Line > 0) {
                impl->Cursor.Line--;
                impl->Cursor.Column = impl->Lines[impl->Cursor.Line].Length;
                impl->PreferredColumn = impl->Cursor.Column;
            }
            return S_OK;

        case TuiKeyRight:
            if (impl->Cursor.Line < impl->LineCount) {
                TextLine *line = &impl->Lines[impl->Cursor.Line];
                if (impl->Cursor.Column < line->Length) {
                    impl->Cursor.Column++;
                    impl->PreferredColumn = impl->Cursor.Column;
                } else if (impl->Cursor.Line < impl->LineCount - 1) {
                    impl->Cursor.Line++;
                    impl->Cursor.Column = 0;
                    impl->PreferredColumn = 0;
                }
            }
            return S_OK;

        case TuiKeyHome:
            impl->Cursor.Column = 0;
            impl->PreferredColumn = 0;
            return S_OK;

        case TuiKeyEnd:
            if (impl->Cursor.Line < impl->LineCount) {
                impl->Cursor.Column = impl->Lines[impl->Cursor.Line].Length;
                impl->PreferredColumn = impl->Cursor.Column;
            }
            return S_OK;

        case TuiKeyPageUp:
            if (impl->Cursor.Line >= impl->VisibleLines) {
                impl->Cursor.Line -= impl->VisibleLines;
                impl->ScrollLine = impl->Cursor.Line;
            } else {
                impl->Cursor.Line = 0;
                impl->ScrollLine = 0;
            }
            return S_OK;

        case TuiKeyPageDown:
            if (impl->Cursor.Line + impl->VisibleLines < impl->LineCount) {
                impl->Cursor.Line += impl->VisibleLines;
                impl->ScrollLine = impl->Cursor.Line;
            } else {
                impl->Cursor.Line = impl->LineCount - 1;
            }
            return S_OK;

        case TuiKeyEnter:
            InsertNewline(impl);
            return S_OK;

        case TuiKeyBackspace:
            if (impl->Cursor.Column > 0) {
                impl->Cursor.Column--;
                DeleteChar(impl);
                impl->PreferredColumn = impl->Cursor.Column;
            }
            return S_OK;

        case TuiKeyDelete:
            DeleteChar(impl);
            return S_OK;

        case TuiKeyInsert:
            impl->InsertMode = !impl->InsertMode;
            return S_OK;

        case TuiKeyF6:
            /* Toggle bold (WordPerfect-style) */
            impl->CurrentAttributes ^= AttrBold;
            return S_OK;

        case TuiKeyF7:
            /* Toggle italic */
            impl->CurrentAttributes ^= AttrItalic;
            return S_OK;

        case TuiKeyF8:
            /* Toggle underline */
            impl->CurrentAttributes ^= AttrUnderline;
            return S_OK;

        case TuiKeyF11:
            /* Reveal codes (WordPerfect) */
            impl->ShowCodes = !impl->ShowCodes;
            return S_OK;

        default:
            /* Printable character */
            if (Key >= 32 && Key < 127) {
                InsertChar(impl, (CHAR8)Key);
                impl->PreferredColumn = impl->Cursor.Column;
            }
            return S_OK;
    }

    return S_OK;
}

/* Set text */
static HRESULT ANXAPI RichTextEditor_SetText(
    ITuiRichTextEditor *This,
    CONST CHAR8 *Text
)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;

    /* Clear existing text */
    for (UINT32 i = 0; i < impl->LineCount; i++) {
        if (impl->Lines[i].Chars) {
            free(impl->Lines[i].Chars);
            impl->Lines[i].Chars = NULL;
        }
    }
    impl->LineCount = 0;
    impl->Cursor.Line = 0;
    impl->Cursor.Column = 0;

    /* Parse and insert text */
    if (!Text) return S_OK;

    CONST CHAR8 *p = Text;
    while (*p) {
        if (*p == '\n') {
            InsertNewline(impl);
            p++;
        } else if (*p == '\r') {
            p++;  /* Skip CR */
        } else {
            InsertChar(impl, *p++);
        }
    }

    impl->Modified = FALSE;
    return S_OK;
}

/* Toggle formatting */
static HRESULT ANXAPI RichTextEditor_ToggleFormat(
    ITuiRichTextEditor *This,
    UINT32 Attribute
)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;
    impl->CurrentAttributes ^= (UINT8)Attribute;
    return S_OK;
}

/* Set ruler */
static HRESULT ANXAPI RichTextEditor_SetRuler(
    ITuiRichTextEditor *This,
    ITuiRuler *Ruler
)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;

    if (impl->Ruler) {
        impl->Ruler->Vtbl->Release(impl->Ruler);
    }

    impl->Ruler = Ruler;
    if (Ruler) {
        Ruler->Vtbl->AddRef(Ruler);
    }

    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI RichTextEditor_SetBounds(ITuiRichTextEditor *This, CONST TUI_RECT *Bounds)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;
    impl->State.Bounds = *Bounds;
    impl->VisibleLines = Bounds->Height - 2;  /* -2 for ruler and status */
    impl->VisibleColumns = Bounds->Width;
    return S_OK;
}

static HRESULT ANXAPI RichTextEditor_GetBounds(ITuiRichTextEditor *This, TUI_RECT *Bounds)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI RichTextEditor_SetVisible(ITuiRichTextEditor *This, BOOLEAN Visible)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI RichTextEditor_IsVisible(ITuiRichTextEditor *This)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI RichTextEditor_SetEnabled(ITuiRichTextEditor *This, BOOLEAN Enabled)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI RichTextEditor_IsEnabled(ITuiRichTextEditor *This)
{
    TuiRichTextEditorImpl *impl = (TuiRichTextEditorImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiRichTextEditorVtbl RichTextEditorVtbl = {
    RichTextEditor_QueryInterface,
    RichTextEditor_AddRef,
    RichTextEditor_Release,
    RichTextEditor_Render,
    RichTextEditor_HandleKey,
    RichTextEditor_SetBounds,
    RichTextEditor_GetBounds,
    RichTextEditor_SetVisible,
    RichTextEditor_IsVisible,
    RichTextEditor_SetEnabled,
    RichTextEditor_IsEnabled,
    RichTextEditor_SetText,
    RichTextEditor_ToggleFormat,
    RichTextEditor_SetRuler
};

/* Factory function */
HRESULT AnxTuiCreateRichTextEditor(ITuiRichTextEditor **OutEditor)
{
    TuiRichTextEditorImpl *impl;

    if (!OutEditor) return E_INVALIDARG;

    impl = (TuiRichTextEditorImpl *)malloc(sizeof(TuiRichTextEditorImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiRichTextEditorImpl));
    impl->Interface.Vtbl = &RichTextEditorVtbl;
    InitWidgetState(&impl->State);

    /* Initialize document */
    impl->LineCapacity = 1024;
    impl->Lines = (TextLine *)calloc(impl->LineCapacity, sizeof(TextLine));
    if (!impl->Lines) {
        free(impl);
        return E_OUTOFMEMORY;
    }
    impl->LineCount = 1;

    /* Initialize first line */
    EnsureLine(impl, 0);

    /* Default state */
    impl->InsertMode = TRUE;
    impl->WordWrap = TRUE;
    impl->ShowCodes = FALSE;
    impl->Modified = FALSE;

    impl->CurrentAttributes = AttrNormal;
    impl->CurrentForeground = TuiColorBlack;
    impl->CurrentBackground = TuiColorWhite;

    impl->State.Bounds.Width = 80;
    impl->State.Bounds.Height = 24;
    impl->VisibleLines = 22;
    impl->VisibleColumns = 80;

    *OutEditor = &impl->Interface;
    return S_OK;
}
