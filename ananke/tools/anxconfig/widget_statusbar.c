/*
 * StatusBar Widget Implementation
 *
 * Status bar that can attach to windows or desktop.
 * Supports multiple panels with different alignments.
 * Uses new event dispatching architecture with ITuiWidget, ITuiDrawListener.
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
    ITuiWidget WidgetInterface;
    ITuiDrawListener DrawListener;

    WIDGET_STATE State;
    StatusPanel Panels[MAX_STATUS_PANELS];
    UINT32 PanelCount;
    ITuiResponder *NextResponder;
    ITuiSurface *Surface;
} TuiStatusBarImpl;

/* Helper macros for interface conversions */
#define STATUSBAR_FROM_WIDGET(w) ((TuiStatusBarImpl*)((UINT8*)(w) - offsetof(TuiStatusBarImpl, WidgetInterface)))
#define STATUSBAR_FROM_DRAW(d) ((TuiStatusBarImpl*)((UINT8*)(d) - offsetof(TuiStatusBarImpl, DrawListener)))

/*=============================================================================
 * ITuiWidget Implementation
 *===========================================================================*/

static HRESULT ANXAPI StatusBarWidget_QueryInterface(ITuiWidget *This, REFIID riid, VOID **ppvObject)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    if (!ppvObject) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ITuiWidget)) {
        *ppvObject = &impl->WidgetInterface;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiDrawListener)) {
        *ppvObject = &impl->DrawListener;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiSerializable)) {
        *ppvObject = &impl->WidgetInterface;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiResponder)) {
        *ppvObject = &impl->WidgetInterface;
        impl->State.RefCount++;
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI StatusBarWidget_AddRef(ITuiWidget *This)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI StatusBarWidget_Release(ITuiWidget *This)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
        if (impl->NextResponder) impl->NextResponder->Vtbl->Release(impl->NextResponder);
        free(impl);
    }
    return refCount;
}

static HRESULT ANXAPI StatusBarWidget_SetBounds(ITuiWidget *This, CONST TUI_RECT *Bounds)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_GetBounds(ITuiWidget *This, TUI_RECT *Bounds)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    if (!Bounds) return E_INVALIDARG;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_SetVisible(ITuiWidget *This, BOOLEAN Visible)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_GetVisible(ITuiWidget *This, BOOLEAN *Visible)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    if (!Visible) return E_INVALIDARG;
    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_SetEnabled(ITuiWidget *This, BOOLEAN Enabled)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_GetEnabled(ITuiWidget *This, BOOLEAN *Enabled)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    if (!Enabled) return E_INVALIDARG;
    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_SetParent(ITuiWidget *This, ITuiWidget *Parent)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    if (impl->NextResponder) {
        impl->NextResponder->Vtbl->Release(impl->NextResponder);
        impl->NextResponder = NULL;
    }
    if (Parent) {
        ITuiResponder *parentResponder = NULL;
        HRESULT hr = Parent->Vtbl->QueryInterface((ITuiWidget *)Parent, &IID_ITuiResponder,
                                                   (VOID **)&parentResponder);
        if (SUCCEEDED(hr)) impl->NextResponder = parentResponder;
    }
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_GetParent(ITuiWidget *This, ITuiWidget **Parent)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    if (!Parent) return E_INVALIDARG;
    if (impl->NextResponder) {
        return impl->NextResponder->Vtbl->QueryInterface(impl->NextResponder,
                                                         &IID_ITuiWidget,
                                                         (VOID **)Parent);
    }
    *Parent = NULL;
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_Invalidate(ITuiWidget *This, CONST TUI_RECT *Rect)
{
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_GetNextResponder(ITuiWidget *This, ITuiResponder **NextResponder)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    if (!NextResponder) return E_INVALIDARG;
    *NextResponder = impl->NextResponder;
    if (impl->NextResponder) impl->NextResponder->Vtbl->AddRef(impl->NextResponder);
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_BecomeFirstResponder(ITuiWidget *This)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    impl->State.Focused = TRUE;
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_ResignFirstResponder(ITuiWidget *This)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    impl->State.Focused = FALSE;
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_SerializeToYaml(ITuiWidget *This, CHAR8 **OutYaml, UINTN *OutLength)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(This);
    CHAR8 *yaml = (CHAR8 *)malloc(4096);
    if (!yaml) return E_OUTOFMEMORY;

    snprintf(yaml, 4096,
        "type: StatusBar\nbounds:\n  x: %d\n  y: %d\n  width: %d\n  height: %d\n"
        "visible: %s\nenabled: %s\npanel_count: %u\n",
        impl->State.Bounds.X, impl->State.Bounds.Y, impl->State.Bounds.Width, impl->State.Bounds.Height,
        impl->State.Visible ? "true" : "false", impl->State.Enabled ? "true" : "false",
        impl->PanelCount);

    *OutYaml = yaml;
    *OutLength = strlen(yaml);
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_DeserializeFromYaml(ITuiWidget *This, CONST CHAR8 *Yaml, UINTN Length)
{
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_GetTypeName(ITuiWidget *This, CONST CHAR8 **OutTypeName)
{
    *OutTypeName = "StatusBar";
    return S_OK;
}

static HRESULT ANXAPI StatusBarWidget_Clone(ITuiWidget *This, ITuiSerializable **OutClone)
{
    ITuiWidget *newStatusBar = NULL;
    HRESULT hr = AnxTuiCreateStatusBar(&newStatusBar);
    if (FAILED(hr)) return hr;

    *OutClone = (ITuiSerializable *)newStatusBar;
    return S_OK;
}

static ITuiWidget_Vtbl StatusBarWidgetVtbl = {
    StatusBarWidget_QueryInterface, StatusBarWidget_AddRef, StatusBarWidget_Release,
    StatusBarWidget_SetBounds, StatusBarWidget_GetBounds, StatusBarWidget_SetVisible, StatusBarWidget_GetVisible,
    StatusBarWidget_SetEnabled, StatusBarWidget_GetEnabled, StatusBarWidget_SetParent, StatusBarWidget_GetParent,
    StatusBarWidget_Invalidate, StatusBarWidget_GetNextResponder, StatusBarWidget_BecomeFirstResponder,
    StatusBarWidget_ResignFirstResponder, StatusBarWidget_SerializeToYaml, StatusBarWidget_DeserializeFromYaml,
    StatusBarWidget_GetTypeName, StatusBarWidget_Clone
};

/*=============================================================================
 * ITuiDrawListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI StatusBarDraw_QueryInterface(ITuiDrawListener *This, REFIID riid, VOID **ppvObject)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_DRAW(This);
    return StatusBarWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI StatusBarDraw_AddRef(ITuiDrawListener *This)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI StatusBarDraw_Release(ITuiDrawListener *This)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_DRAW(This);
    return StatusBarWidget_Release(&impl->WidgetInterface);
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
    ITuiSurface *Surface,
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
    UINT32 i;

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
    for (i = 0; i < Width; i++) {
        Surface->Vtbl->WriteText(Surface, X + i, Y, " ", Fg, Bg);
    }

    /* Render text */
    Surface->Vtbl->WriteText(Surface, startX, Y, display, Fg, Bg);
}

static HRESULT ANXAPI StatusBarDraw_OnDraw(ITuiDrawListener *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_DRAW(This);
    UINT32 i;
    UINT32 panelWidths[MAX_STATUS_PANELS];
    UINT32 panelPositions[MAX_STATUS_PANELS];
    TUI_COLOR fg, bg;
    UINT32 width;

    if (!impl->State.Visible) return S_OK;

    fg = impl->State.ForegroundColor;
    bg = impl->State.BackgroundColor;
    width = impl->State.Bounds.Width;

    /* Compute panel layout */
    ComputePanelLayout(impl, width, panelWidths, panelPositions);

    /* Render panels */
    for (i = 0; i < impl->PanelCount; i++) {
        RenderAlignedText(
            Surface,
            panelPositions[i],
            0,
            panelWidths[i],
            impl->Panels[i].Text,
            impl->Panels[i].Alignment,
            fg,
            bg
        );

        /* Draw separator */
        if (i < impl->PanelCount - 1) {
            Surface->Vtbl->WriteText(Surface,
                                    panelPositions[i] + panelWidths[i],
                                    0, "│", fg, bg);
        }
    }

    return S_OK;
}

static HRESULT ANXAPI StatusBarDraw_OnGetPreferredSize(ITuiDrawListener *This, UINT32 *Width, UINT32 *Height)
{
    if (Width) *Width = 80;  /* Default width */
    if (Height) *Height = 1;  /* Status bar is always 1 line tall */
    return S_OK;
}

static ITuiDrawListener_Vtbl StatusBarDrawVtbl = {
    StatusBarDraw_QueryInterface, StatusBarDraw_AddRef, StatusBarDraw_Release,
    StatusBarDraw_OnDraw, StatusBarDraw_OnGetPreferredSize
};

/*=============================================================================
 * Factory Function
 *===========================================================================*/

HRESULT ANXAPI AnxTuiCreateStatusBar(OUT ITuiWidget **Widget)
{
    TuiStatusBarImpl *impl;

    if (Widget == NULL) return E_POINTER;

    impl = (TuiStatusBarImpl *)calloc(1, sizeof(TuiStatusBarImpl));
    if (impl == NULL) {
        *Widget = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize vtables */
    impl->WidgetInterface.Vtbl = &StatusBarWidgetVtbl;
    impl->DrawListener.Vtbl = &StatusBarDrawVtbl;

    /* Initialize widget state */
    InitWidgetState(&impl->State);

    /* Default colors for status bar */
    impl->State.ForegroundColor = TuiColorBlack;
    impl->State.BackgroundColor = TuiColorWhite;

    impl->PanelCount = 0;
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *Widget = &impl->WidgetInterface;
    return S_OK;
}

/* Convenience functions for statusbar-specific operations */
HRESULT ANXAPI AnxTuiStatusBarAddPanel(ITuiWidget *Widget, UINT32 Width, TUI_TEXT_ALIGNMENT Alignment)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(Widget);

    if (impl->PanelCount >= MAX_STATUS_PANELS) return E_OUTOFMEMORY;

    impl->Panels[impl->PanelCount].Text[0] = '\0';
    impl->Panels[impl->PanelCount].Width = Width;
    impl->Panels[impl->PanelCount].Alignment = Alignment;
    impl->Panels[impl->PanelCount].Spring = FALSE;
    impl->PanelCount++;

    return S_OK;
}

HRESULT ANXAPI AnxTuiStatusBarAddSpringPanel(ITuiWidget *Widget, TUI_TEXT_ALIGNMENT Alignment)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(Widget);

    if (impl->PanelCount >= MAX_STATUS_PANELS) return E_OUTOFMEMORY;

    impl->Panels[impl->PanelCount].Text[0] = '\0';
    impl->Panels[impl->PanelCount].Width = 0;
    impl->Panels[impl->PanelCount].Alignment = Alignment;
    impl->Panels[impl->PanelCount].Spring = TRUE;
    impl->PanelCount++;

    return S_OK;
}

HRESULT ANXAPI AnxTuiStatusBarSetPanelText(ITuiWidget *Widget, INT32 PanelIndex, CONST CHAR8 *Text)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(Widget);

    if (Text == NULL) return E_POINTER;
    if (PanelIndex < 0 || (UINT32)PanelIndex >= impl->PanelCount) {
        return E_INVALIDARG;
    }

    strncpy(impl->Panels[PanelIndex].Text, Text,
            sizeof(impl->Panels[0].Text) - 1);
    impl->Panels[PanelIndex].Text[sizeof(impl->Panels[0].Text) - 1] = '\0';

    return S_OK;
}

HRESULT ANXAPI AnxTuiStatusBarGetPanelText(ITuiWidget *Widget, INT32 PanelIndex, CHAR8 *Buffer, UINTN BufferSize)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(Widget);

    if (Buffer == NULL) return E_POINTER;
    if (PanelIndex < 0 || (UINT32)PanelIndex >= impl->PanelCount) {
        return E_INVALIDARG;
    }

    strncpy(Buffer, impl->Panels[PanelIndex].Text, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';

    return S_OK;
}

HRESULT ANXAPI AnxTuiStatusBarClear(ITuiWidget *Widget)
{
    TuiStatusBarImpl *impl = STATUSBAR_FROM_WIDGET(Widget);
    impl->PanelCount = 0;
    return S_OK;
}
