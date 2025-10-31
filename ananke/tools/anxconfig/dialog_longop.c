/*
 * dialog_longop.c - Long Operation Progress Dialog
 *
 * Modal dialog for long-running operations with progress bar,
 * status updates, time tracking, and cancel support.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_STATUS_LENGTH 256
#define DIALOG_WIDTH 60
#define DIALOG_HEIGHT 12

typedef struct {
    ITuiLongOpDialog Interface;
    WIDGET_STATE State;

    CHAR8 Title[128];
    CHAR8 StatusText[MAX_STATUS_LENGTH];
    UINT32 ProgressPercent;  /* 0-100 */
    BOOLEAN Indeterminate;    /* For operations without known duration */
    BOOLEAN Cancelled;

    /* Time tracking */
    time_t StartTime;
    time_t LastUpdateTime;
    UINT32 ElapsedSeconds;
    UINT32 EstimatedSeconds;  /* ETA */

    /* Sub-widgets */
    ITuiProgressBar *ProgressBar;
    ITuiButton *CancelButton;

    /* Callback for cancel */
    HRESULT (*OnCancel)(VOID *UserData);
    VOID *UserData;

    /* Animation for indeterminate progress */
    UINT32 AnimationFrame;
} TuiLongOpDialogImpl;

/* IUnknown methods */
static HRESULT ANXAPI LongOpDialog_QueryInterface(
    ITuiLongOpDialog *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiLongOpDialog)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI LongOpDialog_AddRef(ITuiLongOpDialog *This)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI LongOpDialog_Release(ITuiLongOpDialog *This)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        if (impl->ProgressBar) {
            impl->ProgressBar->Vtbl->Release(impl->ProgressBar);
        }
        if (impl->CancelButton) {
            impl->CancelButton->Vtbl->Release(impl->CancelButton);
        }
        free(impl);
    }

    return count;
}

/* Helper to format time */
static VOID FormatTime(UINT32 Seconds, CHAR8 *Buffer, UINTN BufferSize)
{
    UINT32 hours = Seconds / 3600;
    UINT32 minutes = (Seconds % 3600) / 60;
    UINT32 secs = Seconds % 60;

    if (hours > 0) {
        snprintf(Buffer, BufferSize, "%u:%02u:%02u", hours, minutes, secs);
    } else {
        snprintf(Buffer, BufferSize, "%u:%02u", minutes, secs);
    }
}

/* Render the dialog */
static HRESULT ANXAPI LongOpDialog_Render(
    ITuiLongOpDialog *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    INT32 i, j;
    CHAR8 line[256];
    CHAR8 timeStr[64];

    if (!impl->State.Visible) return S_OK;

    /* Draw shadow (offset by 2,1) */
    for (i = 0; i < DIALOG_HEIGHT; i++) {
        for (j = 0; j < DIALOG_WIDTH; j++) {
            Screen->Vtbl->WriteChar(Screen, X + j + 2, Y + i + 1, ' ', TuiColorBlack, TuiColorBlack);
        }
    }

    /* Draw dialog background */
    for (i = 0; i < DIALOG_HEIGHT; i++) {
        for (j = 0; j < DIALOG_WIDTH; j++) {
            Screen->Vtbl->WriteChar(Screen, X + j, Y + i, ' ', TuiColorBlack, TuiColorWhite);
        }
    }

    /* Draw 3D border */
    /* Top and left - light */
    for (j = 0; j < DIALOG_WIDTH; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, Y, 0x2500, TuiColorBrightWhite, TuiColorWhite);
    }
    for (i = 0; i < DIALOG_HEIGHT; i++) {
        Screen->Vtbl->WriteChar(Screen, X, Y + i, 0x2502, TuiColorBrightWhite, TuiColorWhite);
    }

    /* Bottom and right - dark */
    for (j = 0; j < DIALOG_WIDTH; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, Y + DIALOG_HEIGHT - 1, 0x2500, TuiColorBrightBlack, TuiColorWhite);
    }
    for (i = 0; i < DIALOG_HEIGHT; i++) {
        Screen->Vtbl->WriteChar(Screen, X + DIALOG_WIDTH - 1, Y + i, 0x2502, TuiColorBrightBlack, TuiColorWhite);
    }

    /* Corners */
    Screen->Vtbl->WriteChar(Screen, X, Y, 0x250C, TuiColorBrightWhite, TuiColorWhite);
    Screen->Vtbl->WriteChar(Screen, X + DIALOG_WIDTH - 1, Y, 0x2510, TuiColorBrightBlack, TuiColorWhite);
    Screen->Vtbl->WriteChar(Screen, X, Y + DIALOG_HEIGHT - 1, 0x2514, TuiColorBrightWhite, TuiColorWhite);
    Screen->Vtbl->WriteChar(Screen, X + DIALOG_WIDTH - 1, Y + DIALOG_HEIGHT - 1, 0x2518, TuiColorBrightBlack, TuiColorWhite);

    /* Draw title bar */
    Screen->Vtbl->WriteText(Screen, X + 2, Y + 1, impl->Title, TuiColorWhite, TuiColorBlue);
    for (j = strlen(impl->Title) + 2; j < DIALOG_WIDTH - 2; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, Y + 1, ' ', TuiColorWhite, TuiColorBlue);
    }

    /* Draw status text (centered, word-wrapped if needed) */
    INT32 statusY = Y + 3;
    UINTN statusLen = strlen(impl->StatusText);
    if (statusLen > DIALOG_WIDTH - 4) {
        /* Word wrap */
        CHAR8 wrapped[MAX_STATUS_LENGTH];
        strncpy(wrapped, impl->StatusText, sizeof(wrapped) - 1);
        wrapped[sizeof(wrapped) - 1] = '\0';

        /* Simple word wrap - split at spaces */
        CHAR8 *line1 = wrapped;
        CHAR8 *line2 = NULL;
        UINTN midpoint = (DIALOG_WIDTH - 4) / 2;

        for (UINTN k = midpoint; k < statusLen && k < DIALOG_WIDTH - 4; k++) {
            if (wrapped[k] == ' ') {
                wrapped[k] = '\0';
                line2 = &wrapped[k + 1];
                break;
            }
        }

        INT32 x1 = X + (DIALOG_WIDTH - strlen(line1)) / 2;
        Screen->Vtbl->WriteText(Screen, x1, statusY, line1, TuiColorBlack, TuiColorWhite);

        if (line2) {
            INT32 x2 = X + (DIALOG_WIDTH - strlen(line2)) / 2;
            Screen->Vtbl->WriteText(Screen, x2, statusY + 1, line2, TuiColorBlack, TuiColorWhite);
            statusY += 2;
        } else {
            statusY += 1;
        }
    } else {
        INT32 textX = X + (DIALOG_WIDTH - statusLen) / 2;
        Screen->Vtbl->WriteText(Screen, textX, statusY, impl->StatusText, TuiColorBlack, TuiColorWhite);
        statusY += 1;
    }

    /* Draw progress bar */
    if (impl->ProgressBar) {
        impl->ProgressBar->Vtbl->Render(impl->ProgressBar, Screen, X + 4, statusY + 1, FALSE);
    }

    /* Draw percentage (if determinate) */
    if (!impl->Indeterminate) {
        snprintf(line, sizeof(line), "%u%%", impl->ProgressPercent);
        INT32 percentX = X + (DIALOG_WIDTH - strlen(line)) / 2;
        Screen->Vtbl->WriteText(Screen, percentX, statusY + 2, line, TuiColorBlack, TuiColorWhite);
    } else {
        /* Draw spinner animation */
        const CHAR8 *spinner[] = {"|", "/", "-", "\\"};
        CHAR8 spinText[32];
        snprintf(spinText, sizeof(spinText), "Working %s", spinner[impl->AnimationFrame % 4]);
        INT32 spinX = X + (DIALOG_WIDTH - strlen(spinText)) / 2;
        Screen->Vtbl->WriteText(Screen, spinX, statusY + 2, spinText, TuiColorBlack, TuiColorWhite);
    }

    /* Draw time information */
    FormatTime(impl->ElapsedSeconds, timeStr, sizeof(timeStr));
    snprintf(line, sizeof(line), "Elapsed: %s", timeStr);
    Screen->Vtbl->WriteText(Screen, X + 4, statusY + 4, line, TuiColorBrightBlack, TuiColorWhite);

    if (!impl->Indeterminate && impl->ProgressPercent > 0 && impl->ProgressPercent < 100) {
        FormatTime(impl->EstimatedSeconds, timeStr, sizeof(timeStr));
        snprintf(line, sizeof(line), "Remaining: %s", timeStr);
        Screen->Vtbl->WriteText(Screen, X + DIALOG_WIDTH - strlen(line) - 4, statusY + 4, line, TuiColorBrightBlack, TuiColorWhite);
    }

    /* Draw Cancel button */
    if (impl->CancelButton) {
        impl->CancelButton->Vtbl->Render(impl->CancelButton, Screen,
            X + (DIALOG_WIDTH - 12) / 2, Y + DIALOG_HEIGHT - 3, FALSE);
    }

    return S_OK;
}

/* Handle keyboard input */
static HRESULT ANXAPI LongOpDialog_HandleKey(
    ITuiLongOpDialog *This,
    TUI_KEY Key
)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;

    if (Key == TuiKeyEscape || Key == 'c' || Key == 'C') {
        impl->Cancelled = TRUE;
        if (impl->OnCancel) {
            impl->OnCancel(impl->UserData);
        }
        return S_OK;
    }

    if (Key == TuiKeyEnter && impl->CancelButton) {
        impl->Cancelled = TRUE;
        if (impl->OnCancel) {
            impl->OnCancel(impl->UserData);
        }
        return S_OK;
    }

    return S_OK;
}

/* Set dialog title */
static HRESULT ANXAPI LongOpDialog_SetTitle(
    ITuiLongOpDialog *This,
    CONST CHAR8 *Title
)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    strncpy(impl->Title, Title, sizeof(impl->Title) - 1);
    impl->Title[sizeof(impl->Title) - 1] = '\0';
    return S_OK;
}

/* Update progress */
static HRESULT ANXAPI LongOpDialog_UpdateProgress(
    ITuiLongOpDialog *This,
    UINT32 Percent,
    CONST CHAR8 *StatusText
)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    time_t now = time(NULL);

    impl->ProgressPercent = (Percent > 100) ? 100 : Percent;

    if (StatusText) {
        strncpy(impl->StatusText, StatusText, sizeof(impl->StatusText) - 1);
        impl->StatusText[sizeof(impl->StatusText) - 1] = '\0';
    }

    /* Update elapsed time */
    impl->ElapsedSeconds = (UINT32)difftime(now, impl->StartTime);

    /* Estimate remaining time */
    if (impl->ProgressPercent > 0 && impl->ProgressPercent < 100) {
        UINT32 totalEstimated = (impl->ElapsedSeconds * 100) / impl->ProgressPercent;
        impl->EstimatedSeconds = totalEstimated - impl->ElapsedSeconds;
    }

    /* Update progress bar */
    if (impl->ProgressBar) {
        impl->ProgressBar->Vtbl->SetProgress(impl->ProgressBar, impl->ProgressPercent);
    }

    impl->LastUpdateTime = now;
    impl->AnimationFrame++;

    return S_OK;
}

/* Set indeterminate mode */
static HRESULT ANXAPI LongOpDialog_SetIndeterminate(
    ITuiLongOpDialog *This,
    BOOLEAN Indeterminate
)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    impl->Indeterminate = Indeterminate;

    if (impl->ProgressBar) {
        impl->ProgressBar->Vtbl->SetStyle(impl->ProgressBar,
            Indeterminate ? TuiProgressMarquee : TuiProgressBar);
    }

    return S_OK;
}

/* Check if cancelled */
static BOOLEAN ANXAPI LongOpDialog_IsCancelled(ITuiLongOpDialog *This)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    return impl->Cancelled;
}

/* Set cancel callback */
static HRESULT ANXAPI LongOpDialog_SetCancelCallback(
    ITuiLongOpDialog *This,
    HRESULT (*Callback)(VOID *UserData),
    VOID *UserData
)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    impl->OnCancel = Callback;
    impl->UserData = UserData;
    return S_OK;
}

/* Start operation */
static HRESULT ANXAPI LongOpDialog_Start(ITuiLongOpDialog *This)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    impl->StartTime = time(NULL);
    impl->LastUpdateTime = impl->StartTime;
    impl->ElapsedSeconds = 0;
    impl->EstimatedSeconds = 0;
    impl->ProgressPercent = 0;
    impl->Cancelled = FALSE;
    impl->AnimationFrame = 0;
    impl->State.Visible = TRUE;
    return S_OK;
}

/* Complete operation */
static HRESULT ANXAPI LongOpDialog_Complete(ITuiLongOpDialog *This)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    impl->ProgressPercent = 100;
    if (impl->ProgressBar) {
        impl->ProgressBar->Vtbl->SetProgress(impl->ProgressBar, 100);
    }
    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI LongOpDialog_SetBounds(
    ITuiLongOpDialog *This,
    CONST TUI_RECT *Bounds
)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI LongOpDialog_GetBounds(
    ITuiLongOpDialog *This,
    TUI_RECT *Bounds
)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI LongOpDialog_SetVisible(
    ITuiLongOpDialog *This,
    BOOLEAN Visible
)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI LongOpDialog_IsVisible(ITuiLongOpDialog *This)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI LongOpDialog_SetEnabled(
    ITuiLongOpDialog *This,
    BOOLEAN Enabled
)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI LongOpDialog_IsEnabled(ITuiLongOpDialog *This)
{
    TuiLongOpDialogImpl *impl = (TuiLongOpDialogImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiLongOpDialogVtbl LongOpDialogVtbl = {
    LongOpDialog_QueryInterface,
    LongOpDialog_AddRef,
    LongOpDialog_Release,
    LongOpDialog_Render,
    LongOpDialog_HandleKey,
    LongOpDialog_SetBounds,
    LongOpDialog_GetBounds,
    LongOpDialog_SetVisible,
    LongOpDialog_IsVisible,
    LongOpDialog_SetEnabled,
    LongOpDialog_IsEnabled,
    LongOpDialog_SetTitle,
    LongOpDialog_UpdateProgress,
    LongOpDialog_SetIndeterminate,
    LongOpDialog_IsCancelled,
    LongOpDialog_SetCancelCallback,
    LongOpDialog_Start,
    LongOpDialog_Complete
};

/* Factory function */
HRESULT AnxTuiCreateLongOpDialog(
    CONST CHAR8 *Title,
    ITuiLongOpDialog **OutDialog
)
{
    TuiLongOpDialogImpl *impl;
    HRESULT hr;

    if (!OutDialog) return E_INVALIDARG;

    impl = (TuiLongOpDialogImpl *)malloc(sizeof(TuiLongOpDialogImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiLongOpDialogImpl));
    impl->Interface.Vtbl = &LongOpDialogVtbl;
    InitWidgetState(&impl->State);

    strncpy(impl->Title, Title ? Title : "Progress", sizeof(impl->Title) - 1);
    impl->Title[sizeof(impl->Title) - 1] = '\0';

    strcpy(impl->StatusText, "Initializing...");
    impl->Indeterminate = FALSE;
    impl->Cancelled = FALSE;

    /* Create progress bar */
    hr = AnxTuiCreateProgressBar(TuiProgressBar, &impl->ProgressBar);
    if (FAILED(hr)) {
        free(impl);
        return hr;
    }

    TUI_RECT progressBounds = {4, 5, DIALOG_WIDTH - 8, 1};
    impl->ProgressBar->Vtbl->SetBounds(impl->ProgressBar, &progressBounds);

    /* Create Cancel button */
    hr = AnxTuiCreateButton("Cancel", NULL, NULL, &impl->CancelButton);
    if (FAILED(hr)) {
        impl->ProgressBar->Vtbl->Release(impl->ProgressBar);
        free(impl);
        return hr;
    }

    TUI_RECT buttonBounds = {(DIALOG_WIDTH - 12) / 2, DIALOG_HEIGHT - 3, 12, 1};
    impl->CancelButton->Vtbl->SetBounds(impl->CancelButton, &buttonBounds);

    impl->State.Bounds.Width = DIALOG_WIDTH;
    impl->State.Bounds.Height = DIALOG_HEIGHT;

    *OutDialog = &impl->Interface;
    return S_OK;
}
