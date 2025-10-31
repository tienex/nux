/*
 * ProgressBar Widget Implementation
 *
 * Visual indicator of progress for long-running operations.
 * Supports determinate (with known progress) and indeterminate
 * (activity indicator) modes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

typedef enum {
    ProgressStyleBlocks,       /* ████████░░░░ */
    ProgressStyleBar,          /* [========>  ] */
    ProgressStyleDots,         /* ●●●●●○○○○○○○ */
    ProgressStyleNumbers       /* 45% [======>] */
} ProgressStyle;

typedef struct {
    ITuiProgressBar Interface;
    WIDGET_STATE State;
    CHAR8 Label[128];
    INT32 MinValue;
    INT32 MaxValue;
    INT32 CurrentValue;
    UINT32 Width;
    ProgressStyle Style;
    BOOLEAN ShowPercentage;
    BOOLEAN Indeterminate;     /* Activity indicator mode */
    UINT32 IndeterminatePos;   /* For animation */
} TuiProgressBarImpl;

/* IUnknown methods */
static HRESULT ANXAPI ProgressBar_QueryInterface(
    ITuiProgressBar *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI ProgressBar_AddRef(ITuiProgressBar *This)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ProgressBar_Release(ITuiProgressBar *This)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiProgressBar methods */
static HRESULT ANXAPI ProgressBar_SetLabel(
    ITuiProgressBar *This,
    CONST CHAR8 *Label
)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;
    if (Label == NULL) return E_POINTER;

    strncpy(impl->Label, Label, sizeof(impl->Label) - 1);
    impl->Label[sizeof(impl->Label) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetRange(
    ITuiProgressBar *This,
    INT32 MinValue,
    INT32 MaxValue
)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;

    if (MinValue >= MaxValue) return E_INVALIDARG;

    impl->MinValue = MinValue;
    impl->MaxValue = MaxValue;

    /* Clamp current value */
    if (impl->CurrentValue < MinValue) impl->CurrentValue = MinValue;
    if (impl->CurrentValue > MaxValue) impl->CurrentValue = MaxValue;

    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetValue(
    ITuiProgressBar *This,
    INT32 Value
)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;

    if (Value < impl->MinValue) Value = impl->MinValue;
    if (Value > impl->MaxValue) Value = impl->MaxValue;

    impl->CurrentValue = Value;

    return S_OK;
}

static HRESULT ANXAPI ProgressBar_GetValue(
    ITuiProgressBar *This,
    INT32 *Value
)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;
    if (Value == NULL) return E_POINTER;
    *Value = impl->CurrentValue;
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetWidth(
    ITuiProgressBar *This,
    UINT32 Width
)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;
    impl->Width = Width;
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetStyle(
    ITuiProgressBar *This,
    UINT32 Style
)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;
    impl->Style = (ProgressStyle)Style;
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetShowPercentage(
    ITuiProgressBar *This,
    BOOLEAN ShowPercentage
)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;
    impl->ShowPercentage = ShowPercentage;
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_SetIndeterminate(
    ITuiProgressBar *This,
    BOOLEAN Indeterminate
)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;
    impl->Indeterminate = Indeterminate;
    if (Indeterminate) {
        impl->IndeterminatePos = 0;
    }
    return S_OK;
}

static HRESULT ANXAPI ProgressBar_Increment(
    ITuiProgressBar *This,
    INT32 Delta
)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;
    return ProgressBar_SetValue(This, impl->CurrentValue + Delta);
}

static HRESULT ANXAPI ProgressBar_Render(
    ITuiProgressBar *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiProgressBarImpl *impl = (TuiProgressBarImpl *)This;
    CHAR8 display[256];
    UINT32 i;
    INT32 percentage;
    INT32 filledWidth;
    TUI_COLOR fg, bg;

    if (!impl->State.Visible) return S_OK;

    fg = impl->State.ForegroundColor;
    bg = impl->State.BackgroundColor;

    /* Render label if present */
    if (strlen(impl->Label) > 0) {
        Screen->Vtbl->WriteText(Screen, X, Y, impl->Label, fg, bg);
        Y++;  /* Move to next line */
    }

    if (impl->Indeterminate) {
        /* Indeterminate mode - activity indicator */
        switch (impl->Style) {
            case ProgressStyleBlocks:
            case ProgressStyleBar:
                /* Animated moving block */
                display[0] = '[';
                for (i = 0; i < impl->Width; i++) {
                    if (i >= impl->IndeterminatePos &&
                        i < impl->IndeterminatePos + 5) {
                        display[i + 1] = '=';
                    } else {
                        display[i + 1] = ' ';
                    }
                }
                display[impl->Width + 1] = ']';
                display[impl->Width + 2] = '\0';
                Screen->Vtbl->WriteText(Screen, X, Y, display, fg, bg);

                /* Animate */
                impl->IndeterminatePos++;
                if (impl->IndeterminatePos >= impl->Width) {
                    impl->IndeterminatePos = 0;
                }
                break;

            case ProgressStyleDots:
                /* Spinning dots */
                CONST CHAR8 *spinner[] = { "⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏" };
                snprintf(display, sizeof(display), "%s Working...",
                         spinner[impl->IndeterminatePos % 10]);
                Screen->Vtbl->WriteText(Screen, X, Y, display, fg, bg);
                impl->IndeterminatePos++;
                break;

            default:
                break;
        }

    } else {
        /* Determinate mode - show actual progress */
        INT32 range = impl->MaxValue - impl->MinValue;
        if (range <= 0) range = 1;

        percentage = ((impl->CurrentValue - impl->MinValue) * 100) / range;
        filledWidth = ((impl->CurrentValue - impl->MinValue) * impl->Width) / range;

        switch (impl->Style) {
            case ProgressStyleBlocks:
                /* ████████░░░░ 45% */
                for (i = 0; i < impl->Width; i++) {
                    if ((INT32)i < filledWidth) {
                        display[i] = '█';
                    } else {
                        display[i] = '░';
                    }
                }
                display[impl->Width] = '\0';
                Screen->Vtbl->WriteText(Screen, X, Y, display,
                                        TuiColorGreen, bg);

                if (impl->ShowPercentage) {
                    snprintf(display, sizeof(display), " %d%%", percentage);
                    Screen->Vtbl->WriteText(Screen, X + impl->Width + 1, Y,
                                            display, fg, bg);
                }
                break;

            case ProgressStyleBar:
                /* [========>  ] 45% */
                display[0] = '[';
                for (i = 0; i < impl->Width; i++) {
                    if ((INT32)i < filledWidth - 1) {
                        display[i + 1] = '=';
                    } else if ((INT32)i == filledWidth - 1) {
                        display[i + 1] = '>';
                    } else {
                        display[i + 1] = ' ';
                    }
                }
                display[impl->Width + 1] = ']';
                display[impl->Width + 2] = '\0';
                Screen->Vtbl->WriteText(Screen, X, Y, display, fg, bg);

                if (impl->ShowPercentage) {
                    snprintf(display, sizeof(display), " %d%%", percentage);
                    Screen->Vtbl->WriteText(Screen, X + impl->Width + 3, Y,
                                            display, fg, bg);
                }
                break;

            case ProgressStyleDots:
                /* ●●●●●○○○○○○○ 45% */
                for (i = 0; i < impl->Width; i++) {
                    if ((INT32)i < filledWidth) {
                        display[i] = '●';
                    } else {
                        display[i] = '○';
                    }
                }
                display[impl->Width] = '\0';
                Screen->Vtbl->WriteText(Screen, X, Y, display,
                                        TuiColorGreen, bg);

                if (impl->ShowPercentage) {
                    snprintf(display, sizeof(display), " %d%%", percentage);
                    Screen->Vtbl->WriteText(Screen, X + impl->Width + 1, Y,
                                            display, fg, bg);
                }
                break;

            case ProgressStyleNumbers:
                /* 45% [======>] */
                snprintf(display, sizeof(display), "%3d%% ", percentage);
                Screen->Vtbl->WriteText(Screen, X, Y, display, fg, bg);

                display[0] = '[';
                for (i = 0; i < impl->Width; i++) {
                    if ((INT32)i < filledWidth - 1) {
                        display[i + 1] = '=';
                    } else if ((INT32)i == filledWidth - 1) {
                        display[i + 1] = '>';
                    } else {
                        display[i + 1] = ' ';
                    }
                }
                display[impl->Width + 1] = ']';
                display[impl->Width + 2] = '\0';
                Screen->Vtbl->WriteText(Screen, X + 5, Y, display, fg, bg);
                break;
        }
    }

    return S_OK;
}

/* Vtable */
static CONST ITuiProgressBar_Vtbl ProgressBarVtbl = {
    ProgressBar_QueryInterface,
    ProgressBar_AddRef,
    ProgressBar_Release,
    ProgressBar_SetLabel,
    ProgressBar_SetRange,
    ProgressBar_SetValue,
    ProgressBar_GetValue,
    ProgressBar_SetWidth,
    ProgressBar_SetStyle,
    ProgressBar_SetShowPercentage,
    ProgressBar_SetIndeterminate,
    ProgressBar_Increment,
    ProgressBar_Render
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateProgressBar(
    IN  UINT32 Width,
    OUT ITuiProgressBar **ProgressBar
)
{
    TuiProgressBarImpl *impl;

    if (ProgressBar == NULL) return E_POINTER;

    impl = (TuiProgressBarImpl *)calloc(1, sizeof(TuiProgressBarImpl));
    if (impl == NULL) {
        *ProgressBar = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &ProgressBarVtbl;
    InitWidgetState(&impl->State);

    impl->Label[0] = '\0';
    impl->MinValue = 0;
    impl->MaxValue = 100;
    impl->CurrentValue = 0;
    impl->Width = Width > 0 ? Width : 40;
    impl->Style = ProgressStyleBar;
    impl->ShowPercentage = TRUE;
    impl->Indeterminate = FALSE;
    impl->IndeterminatePos = 0;

    *ProgressBar = &impl->Interface;
    return S_OK;
}
