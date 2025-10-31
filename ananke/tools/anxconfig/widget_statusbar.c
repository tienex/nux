/*
 * StatusBar Widget Implementation
 *
 * Status bar that can attach to windows or desktop.
 * Supports multiple panels with different alignments.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

#define MAX_STATUS_PANELS 16

typedef struct {
    CHAR8 Text[256];
    UINT32 Width;           /* Fixed width (0 = auto) */
    TUI_TEXT_ALIGNMENT Alignment;
    BOOLEAN Spring;         /* Expand to fill available space */
} StatusPanel;

typedef struct {
    ITuiStatusBar Interface;
    WIDGET_STATE State;
    StatusPanel Panels[MAX_STATUS_PANELS];
    UINT32 PanelCount;
} TuiStatusBarImpl;

/* IUnknown methods */
static HRESULT ANXAPI StatusBar_QueryInterface(
    ITuiStatusBar *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI StatusBar_AddRef(ITuiStatusBar *This)
{
    TuiStatusBarImpl *impl = (TuiStatusBarImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI StatusBar_Release(ITuiStatusBar *This)
{
    TuiStatusBarImpl *impl = (TuiStatusBarImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiStatusBar methods */
static HRESULT ANXAPI StatusBar_AddPanel(
    ITuiStatusBar *This,
    UINT32 Width,
    TUI_TEXT_ALIGNMENT Alignment
)
{
    TuiStatusBarImpl *impl = (TuiStatusBarImpl *)This;

    if (impl->PanelCount >= MAX_STATUS_PANELS) return E_OUTOFMEMORY;

    impl->Panels[impl->PanelCount].Text[0] = '\0';
    impl->Panels[impl->PanelCount].Width = Width;
    impl->Panels[impl->PanelCount].Alignment = Alignment;
    impl->Panels[impl->PanelCount].Spring = FALSE;
    impl->PanelCount++;

    return S_OK;
}

static HRESULT ANXAPI StatusBar_AddSpringPanel(
    ITuiStatusBar *This,
    TUI_TEXT_ALIGNMENT Alignment
)
{
    TuiStatusBarImpl *impl = (TuiStatusBarImpl *)This;

    if (impl->PanelCount >= MAX_STATUS_PANELS) return E_OUTOFMEMORY;

    impl->Panels[impl->PanelCount].Text[0] = '\0';
    impl->Panels[impl->PanelCount].Width = 0;
    impl->Panels[impl->PanelCount].Alignment = Alignment;
    impl->Panels[impl->PanelCount].Spring = TRUE;
    impl->PanelCount++;

    return S_OK;
}

static HRESULT ANXAPI StatusBar_SetPanelText(
    ITuiStatusBar *This,
    INT32 PanelIndex,
    CONST CHAR8 *Text
)
{
    TuiStatusBarImpl *impl = (TuiStatusBarImpl *)This;

    if (Text == NULL) return E_POINTER;
    if (PanelIndex < 0 || (UINT32)PanelIndex >= impl->PanelCount) {
        return E_INVALIDARG;
    }

    strncpy(impl->Panels[PanelIndex].Text, Text,
            sizeof(impl->Panels[0].Text) - 1);
    impl->Panels[PanelIndex].Text[sizeof(impl->Panels[0].Text) - 1] = '\0';

    return S_OK;
}

static HRESULT ANXAPI StatusBar_GetPanelText(
    ITuiStatusBar *This,
    INT32 PanelIndex,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiStatusBarImpl *impl = (TuiStatusBarImpl *)This;

    if (Buffer == NULL) return E_POINTER;
    if (PanelIndex < 0 || (UINT32)PanelIndex >= impl->PanelCount) {
        return E_INVALIDARG;
    }

    strncpy(Buffer, impl->Panels[PanelIndex].Text, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';

    return S_OK;
}

static HRESULT ANXAPI StatusBar_Clear(ITuiStatusBar *This)
{
    TuiStatusBarImpl *impl = (TuiStatusBarImpl *)This;
    impl->PanelCount = 0;
    return S_OK;
}

/* Helper: Compute panel positions and widths */
static VOID ComputePanelLayout(
    TuiStatusBarImpl *impl,
    UINT32 TotalWidth,
    UINT32 *PanelWidths,
    UINT32 *PanelPositions
)
{
    UINT32 i;
    UINT32 usedWidth = 0;
    UINT32 springCount = 0;
    UINT32 currentPos = 0;

    /* First pass: compute fixed widths and count springs */
    for (i = 0; i < impl->PanelCount; i++) {
        if (impl->Panels[i].Spring) {
            springCount++;
        } else if (impl->Panels[i].Width > 0) {
            PanelWidths[i] = impl->Panels[i].Width;
            usedWidth += impl->Panels[i].Width;
        } else {
            /* Auto width based on text length */
            PanelWidths[i] = strlen(impl->Panels[i].Text) + 2;
            usedWidth += PanelWidths[i];
        }
    }

    /* Account for separators */
    if (impl->PanelCount > 1) {
        usedWidth += impl->PanelCount - 1;
    }

    /* Second pass: distribute remaining space to springs */
    if (springCount > 0 && TotalWidth > usedWidth) {
        UINT32 remainingWidth = TotalWidth - usedWidth;
        UINT32 springWidth = remainingWidth / springCount;

        for (i = 0; i < impl->PanelCount; i++) {
            if (impl->Panels[i].Spring) {
                PanelWidths[i] = springWidth;
            }
        }
    }

    /* Third pass: compute positions */
    for (i = 0; i < impl->PanelCount; i++) {
        PanelPositions[i] = currentPos;
        currentPos += PanelWidths[i];
        if (i < impl->PanelCount - 1) {
            currentPos++;  /* Separator */
        }
    }
}

/* Helper: Render text with alignment */
static VOID RenderAlignedText(
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    CONST CHAR8 *Text,
    TUI_TEXT_ALIGNMENT Alignment,
    TUI_COLOR Fg,
    TUI_COLOR Bg
)
{
    CHAR8 display[256];
    UINT32 textLen = strlen(Text);
    INT32 startX = X;

    if (textLen > Width) {
        /* Truncate */
        strncpy(display, Text, Width);
        display[Width] = '\0';
        textLen = Width;
    } else {
        strcpy(display, Text);
    }

    /* Calculate starting position based on alignment */
    if (Alignment == TuiAlignCenter) {
        startX = X + (Width - textLen) / 2;
    } else if (Alignment == TuiAlignRight) {
        startX = X + Width - textLen;
    } else {
        startX = X + 1;  /* Left align with padding */
    }

    /* Clear panel background */
    UINT32 i;
    for (i = 0; i < Width; i++) {
        Screen->Vtbl->WriteText(Screen, X + i, Y, " ", Fg, Bg);
    }

    /* Render text */
    Screen->Vtbl->WriteText(Screen, startX, Y, display, Fg, Bg);
}

static HRESULT ANXAPI StatusBar_Render(
    ITuiStatusBar *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width
)
{
    TuiStatusBarImpl *impl = (TuiStatusBarImpl *)This;
    UINT32 i;
    UINT32 panelWidths[MAX_STATUS_PANELS];
    UINT32 panelPositions[MAX_STATUS_PANELS];
    TUI_COLOR fg, bg;

    if (!impl->State.Visible) return S_OK;

    fg = impl->State.ForegroundColor;
    bg = impl->State.BackgroundColor;

    /* Compute panel layout */
    ComputePanelLayout(impl, Width, panelWidths, panelPositions);

    /* Clear status bar background */
    ClearRect(Screen, X, Y, Width, 1, bg);

    /* Render panels */
    for (i = 0; i < impl->PanelCount; i++) {
        RenderAlignedText(
            Screen,
            X + panelPositions[i],
            Y,
            panelWidths[i],
            impl->Panels[i].Text,
            impl->Panels[i].Alignment,
            fg,
            bg
        );

        /* Draw separator */
        if (i < impl->PanelCount - 1) {
            Screen->Vtbl->WriteText(Screen,
                                    X + panelPositions[i] + panelWidths[i],
                                    Y, "│", fg, bg);
        }
    }

    return S_OK;
}

/* Vtable */
static CONST ITuiStatusBar_Vtbl StatusBarVtbl = {
    StatusBar_QueryInterface,
    StatusBar_AddRef,
    StatusBar_Release,
    StatusBar_AddPanel,
    StatusBar_AddSpringPanel,
    StatusBar_SetPanelText,
    StatusBar_GetPanelText,
    StatusBar_Clear,
    StatusBar_Render
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateStatusBar(OUT ITuiStatusBar **StatusBar)
{
    TuiStatusBarImpl *impl;

    if (StatusBar == NULL) return E_POINTER;

    impl = (TuiStatusBarImpl *)calloc(1, sizeof(TuiStatusBarImpl));
    if (impl == NULL) {
        *StatusBar = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &StatusBarVtbl;
    InitWidgetState(&impl->State);

    impl->PanelCount = 0;

    /* Default colors for status bar */
    impl->State.ForegroundColor = TuiColorBlack;
    impl->State.BackgroundColor = TuiColorWhite;

    *StatusBar = &impl->Interface;
    return S_OK;
}
