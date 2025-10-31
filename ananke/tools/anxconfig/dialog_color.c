/*
 * Color Picker Dialog Implementation
 *
 * Dialog for selecting colors from available palette.
 * Shows all 16 basic colors plus allows RGB/HSV adjustment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

#define COLOR_PALETTE_SIZE 16

typedef struct {
    ITuiColorDialog Interface;
    WIDGET_STATE State;
    CHAR8 Title[128];
    TUI_COLOR SelectedColor;
    TUI_COLOR PreviewColor;
    INT32 SelectedIndex;
    BOOLEAN DialogResult;
    ITuiButton *OkButton;
    ITuiButton *CancelButton;
    UINT32 Width;
    UINT32 Height;
} TuiColorDialogImpl;

/* IUnknown methods */
static HRESULT ANXAPI ColorDialog_QueryInterface(
    ITuiColorDialog *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI ColorDialog_AddRef(ITuiColorDialog *This)
{
    TuiColorDialogImpl *impl = (TuiColorDialogImpl *)This;
    return ++impl->State.RefCount;
}

static HRESULT ANXAPI ColorDialog_Release(ITuiColorDialog *This)
{
    TuiColorDialogImpl *impl = (TuiColorDialogImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->OkButton) impl->OkButton->Vtbl->Release(impl->OkButton);
        if (impl->CancelButton) impl->CancelButton->Vtbl->Release(impl->CancelButton);
        free(impl);
    }
    return refCount;
}

/* Helper: Get color name */
static CONST CHAR8 *GetColorName(TUI_COLOR color)
{
    switch (color) {
        case TuiColorBlack:        return "Black";
        case TuiColorBlue:         return "Blue";
        case TuiColorGreen:        return "Green";
        case TuiColorCyan:         return "Cyan";
        case TuiColorRed:          return "Red";
        case TuiColorMagenta:      return "Magenta";
        case TuiColorYellow:       return "Yellow";
        case TuiColorWhite:        return "White";
        case TuiColorBrightBlack:  return "Bright Black";
        case TuiColorBrightBlue:   return "Bright Blue";
        case TuiColorBrightGreen:  return "Bright Green";
        case TuiColorBrightCyan:   return "Bright Cyan";
        case TuiColorBrightRed:    return "Bright Red";
        case TuiColorBrightMagenta:return "Bright Magenta";
        case TuiColorBrightYellow: return "Bright Yellow";
        case TuiColorBrightWhite:  return "Bright White";
        default:                   return "Unknown";
    }
}

/* ITuiColorDialog methods */
static HRESULT ANXAPI ColorDialog_SetInitialColor(
    ITuiColorDialog *This,
    TUI_COLOR Color
)
{
    TuiColorDialogImpl *impl = (TuiColorDialogImpl *)This;
    impl->SelectedColor = Color;
    impl->PreviewColor = Color;
    impl->SelectedIndex = Color;
    return S_OK;
}

static HRESULT ANXAPI ColorDialog_Show(
    ITuiColorDialog *This,
    ITuiScreen *Screen
)
{
    TuiColorDialogImpl *impl = (TuiColorDialogImpl *)This;
    UINT32 screenWidth, screenHeight;

    impl->DialogResult = FALSE;
    impl->State.Visible = TRUE;

    Screen->Vtbl->GetDimensions(Screen, &screenWidth, &screenHeight);

    /* Center dialog */
    INT32 x = (screenWidth - impl->Width) / 2;
    INT32 y = (screenHeight - impl->Height) / 2;

    /* Render */
    ColorDialog_Render(This, Screen, x, y);
    Screen->Vtbl->Refresh(Screen);

    /* Enter modal loop (simplified) */
    return S_OK;
}

static HRESULT ANXAPI ColorDialog_GetResult(
    ITuiColorDialog *This,
    BOOLEAN *Accepted
)
{
    TuiColorDialogImpl *impl = (TuiColorDialogImpl *)This;
    if (Accepted == NULL) return E_POINTER;
    *Accepted = impl->DialogResult;
    return S_OK;
}

static HRESULT ANXAPI ColorDialog_GetSelectedColor(
    ITuiColorDialog *This,
    TUI_COLOR *Color
)
{
    TuiColorDialogImpl *impl = (TuiColorDialogImpl *)This;
    if (Color == NULL) return E_POINTER;
    *Color = impl->SelectedColor;
    return S_OK;
}

static HRESULT ANXAPI ColorDialog_Render(
    ITuiColorDialog *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiColorDialogImpl *impl = (TuiColorDialogImpl *)This;
    UINT32 i, j;
    CHAR8 display[256];
    TUI_COLOR colors[COLOR_PALETTE_SIZE] = {
        TuiColorBlack, TuiColorBlue, TuiColorGreen, TuiColorCyan,
        TuiColorRed, TuiColorMagenta, TuiColorYellow, TuiColorWhite,
        TuiColorBrightBlack, TuiColorBrightBlue, TuiColorBrightGreen, TuiColorBrightCyan,
        TuiColorBrightRed, TuiColorBrightMagenta, TuiColorBrightYellow, TuiColorBrightWhite
    };

    if (!impl->State.Visible) return S_OK;

    /* Draw shadow */
    for (i = 0; i < impl->Height + 1; i++) {
        ClearRect(Screen, X + 2, Y + i + 1, impl->Width, 1, TuiColorBlack);
    }

    /* Draw dialog box */
    DrawBoxSingle(Screen, X, Y, impl->Width, impl->Height,
                  TuiColorBlack, TuiColorWhite);

    /* Draw title bar */
    snprintf(display, sizeof(display), " %s ", impl->Title);
    Screen->Vtbl->WriteText(Screen, X + 2, Y, display,
                            TuiColorWhite, TuiColorBlue);

    /* Draw color palette (4x4 grid) */
    INT32 paletteY = Y + 2;
    INT32 colorIndex = 0;

    Screen->Vtbl->WriteText(Screen, X + 2, paletteY, "Select a color:",
                            TuiColorBlack, TuiColorWhite);
    paletteY += 2;

    for (i = 0; i < 4; i++) {
        INT32 paletteX = X + 4;
        for (j = 0; j < 4; j++) {
            TUI_COLOR color = colors[colorIndex];

            /* Draw color block */
            CHAR8 colorBlock[8] = "      ";

            if (colorIndex == impl->SelectedIndex) {
                /* Selected color has border */
                snprintf(display, sizeof(display), "[%s]", colorBlock);
                Screen->Vtbl->WriteText(Screen, paletteX - 1, paletteY, display,
                                        TuiColorBlack, color);
            } else {
                /* Normal color block */
                snprintf(display, sizeof(display), " %s ", colorBlock);
                Screen->Vtbl->WriteText(Screen, paletteX, paletteY, display,
                                        TuiColorBlack, color);
            }

            paletteX += 9;
            colorIndex++;
        }
        paletteY += 2;
    }

    /* Draw preview section */
    paletteY += 1;
    Screen->Vtbl->WriteText(Screen, X + 2, paletteY, "Preview:",
                            TuiColorBlack, TuiColorWhite);
    paletteY++;

    /* Preview block */
    ClearRect(Screen, X + 4, paletteY, 20, 3, impl->PreviewColor);
    DrawBoxSingle(Screen, X + 4, paletteY, 20, 3,
                  TuiColorBlack, impl->PreviewColor);

    /* Color name */
    snprintf(display, sizeof(display), "Color: %s", GetColorName(impl->PreviewColor));
    Screen->Vtbl->WriteText(Screen, X + 2, paletteY + 4, display,
                            TuiColorBlack, TuiColorWhite);

    /* Draw buttons */
    if (impl->OkButton) {
        impl->OkButton->Vtbl->Render(impl->OkButton, Screen,
                                      X + impl->Width - 24, Y + impl->Height - 2, FALSE);
    }

    if (impl->CancelButton) {
        impl->CancelButton->Vtbl->Render(impl->CancelButton, Screen,
                                          X + impl->Width - 12, Y + impl->Height - 2, FALSE);
    }

    return S_OK;
}

static HRESULT ANXAPI ColorDialog_HandleKey(
    ITuiColorDialog *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiColorDialogImpl *impl = (TuiColorDialogImpl *)This;

    if (!impl->State.Visible) {
        *Handled = FALSE;
        return S_OK;
    }

    switch (Key) {
        case TuiKeyLeft:
            if (impl->SelectedIndex % 4 > 0) {
                impl->SelectedIndex--;
                impl->PreviewColor = (TUI_COLOR)impl->SelectedIndex;
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyRight:
            if (impl->SelectedIndex % 4 < 3 && impl->SelectedIndex < COLOR_PALETTE_SIZE - 1) {
                impl->SelectedIndex++;
                impl->PreviewColor = (TUI_COLOR)impl->SelectedIndex;
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyUp:
            if (impl->SelectedIndex >= 4) {
                impl->SelectedIndex -= 4;
                impl->PreviewColor = (TUI_COLOR)impl->SelectedIndex;
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyDown:
            if (impl->SelectedIndex < COLOR_PALETTE_SIZE - 4) {
                impl->SelectedIndex += 4;
                impl->PreviewColor = (TUI_COLOR)impl->SelectedIndex;
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnter:
            /* Accept selection */
            impl->SelectedColor = impl->PreviewColor;
            impl->DialogResult = TRUE;
            impl->State.Visible = FALSE;
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEsc:
            /* Cancel */
            impl->DialogResult = FALSE;
            impl->State.Visible = FALSE;
            *Handled = TRUE;
            return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiColorDialog_Vtbl ColorDialogVtbl = {
    ColorDialog_QueryInterface,
    ColorDialog_AddRef,
    ColorDialog_Release,
    ColorDialog_SetInitialColor,
    ColorDialog_Show,
    ColorDialog_GetResult,
    ColorDialog_GetSelectedColor,
    ColorDialog_Render,
    ColorDialog_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateColorDialog(OUT ITuiColorDialog **ColorDialog)
{
    TuiColorDialogImpl *impl;
    HRESULT hr;

    if (ColorDialog == NULL) return E_POINTER;

    impl = (TuiColorDialogImpl *)calloc(1, sizeof(TuiColorDialogImpl));
    if (impl == NULL) {
        *ColorDialog = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &ColorDialogVtbl;
    InitWidgetState(&impl->State);

    strcpy(impl->Title, "Select Color");
    impl->SelectedColor = TuiColorWhite;
    impl->PreviewColor = TuiColorWhite;
    impl->SelectedIndex = TuiColorWhite;
    impl->DialogResult = FALSE;
    impl->Width = 42;
    impl->Height = 22;

    /* Create buttons */
    hr = AnxTuiCreateButton(&impl->OkButton);
    if (FAILED(hr)) {
        free(impl);
        *ColorDialog = NULL;
        return hr;
    }
    impl->OkButton->Vtbl->SetLabel(impl->OkButton, "  OK  ");

    hr = AnxTuiCreateButton(&impl->CancelButton);
    if (FAILED(hr)) {
        impl->OkButton->Vtbl->Release(impl->OkButton);
        free(impl);
        *ColorDialog = NULL;
        return hr;
    }
    impl->CancelButton->Vtbl->SetLabel(impl->CancelButton, "Cancel");

    *ColorDialog = &impl->Interface;
    return S_OK;
}

/* Helper function: Show simple color picker */
HRESULT ANXAPI AnxTuiShowColorDialog(
    ITuiScreen *Screen,
    TUI_COLOR InitialColor,
    TUI_COLOR *SelectedColor
)
{
    ITuiColorDialog *dialog;
    HRESULT hr;
    BOOLEAN accepted;

    hr = AnxTuiCreateColorDialog(&dialog);
    if (FAILED(hr)) return hr;

    dialog->Vtbl->SetInitialColor(dialog, InitialColor);

    hr = dialog->Vtbl->Show(dialog, Screen);
    if (FAILED(hr)) {
        dialog->Vtbl->Release(dialog);
        return hr;
    }

    hr = dialog->Vtbl->GetResult(dialog, &accepted);
    if (SUCCEEDED(hr) && accepted && SelectedColor != NULL) {
        dialog->Vtbl->GetSelectedColor(dialog, SelectedColor);
    }

    dialog->Vtbl->Release(dialog);
    return hr;
}
