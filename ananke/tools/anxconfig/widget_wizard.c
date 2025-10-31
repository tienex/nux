/*
 * widget_wizard.c - Wizard Dialog Widget
 *
 * Multi-step workflow dialog with Back/Next/Finish/Cancel buttons,
 * progress indicator, and page validation.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_WIZARD_PAGES 32
#define WIZARD_WIDTH 80
#define WIZARD_HEIGHT 30

/* Wizard page */
typedef struct {
    CHAR8 Title[128];
    CHAR8 Description[256];
    VOID *Widget;                /* Page content widget */
    HRESULT (*OnEnter)(VOID *PageWidget, VOID *UserData);
    HRESULT (*OnLeave)(VOID *PageWidget, VOID *UserData, BOOLEAN *AllowLeave);
    HRESULT (*OnValidate)(VOID *PageWidget, VOID *UserData, BOOLEAN *IsValid);
    VOID *UserData;
} WizardPage;

typedef struct {
    ITuiWizard Interface;
    WIDGET_STATE State;

    /* Pages */
    WizardPage Pages[MAX_WIZARD_PAGES];
    UINT32 PageCount;
    INT32 CurrentPage;

    /* Dialog state */
    BOOLEAN Cancelled;
    BOOLEAN Finished;

    /* Buttons */
    ITuiButton *BackButton;
    ITuiButton *NextButton;
    ITuiButton *FinishButton;
    ITuiButton *CancelButton;

    /* Callbacks */
    HRESULT (*OnFinish)(VOID *UserData);
    HRESULT (*OnCancel)(VOID *UserData);
    VOID *UserData;

} TuiWizardImpl;

/* IUnknown methods */
static HRESULT ANXAPI Wizard_QueryInterface(
    ITuiWizard *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiWizard)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI Wizard_AddRef(ITuiWizard *This)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Wizard_Release(ITuiWizard *This)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        if (impl->BackButton) impl->BackButton->Vtbl->Release(impl->BackButton);
        if (impl->NextButton) impl->NextButton->Vtbl->Release(impl->NextButton);
        if (impl->FinishButton) impl->FinishButton->Vtbl->Release(impl->FinishButton);
        if (impl->CancelButton) impl->CancelButton->Vtbl->Release(impl->CancelButton);
        free(impl);
    }

    return count;
}

/* Render the wizard */
static HRESULT ANXAPI Wizard_Render(
    ITuiWizard *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;

    if (!impl->State.Visible) return S_OK;
    if (impl->CurrentPage < 0 || impl->CurrentPage >= (INT32)impl->PageCount) return S_OK;

    WizardPage *page = &impl->Pages[impl->CurrentPage];

    /* Draw shadow */
    for (UINT32 i = 0; i < WIZARD_HEIGHT; i++) {
        for (UINT32 j = 0; j < WIZARD_WIDTH; j++) {
            Screen->Vtbl->WriteChar(Screen, X + j + 2, Y + i + 1, ' ', TuiColorBlack, TuiColorBlack);
        }
    }

    /* Draw dialog background */
    for (UINT32 i = 0; i < WIZARD_HEIGHT; i++) {
        for (UINT32 j = 0; j < WIZARD_WIDTH; j++) {
            Screen->Vtbl->WriteChar(Screen, X + j, Y + i, ' ', TuiColorBlack, TuiColorWhite);
        }
    }

    /* Draw double-line border */
    DrawBoxDouble(Screen, X, Y, WIZARD_WIDTH, WIZARD_HEIGHT, TuiColorBlack, TuiColorWhite);

    /* Draw title bar */
    CHAR8 titleBar[128];
    snprintf(titleBar, sizeof(titleBar), " Wizard - Step %d of %d ",
             impl->CurrentPage + 1, impl->PageCount);
    UINTN titleLen = strlen(titleBar);
    for (UINT32 j = 1; j < WIZARD_WIDTH - 1; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, Y + 1, ' ', TuiColorWhite, TuiColorBlue);
    }
    Screen->Vtbl->WriteText(Screen, X + 2, Y + 1, titleBar, TuiColorWhite, TuiColorBlue);

    /* Draw separator */
    Screen->Vtbl->WriteChar(Screen, X, Y + 2, gBoxChars.SingleTeeLeft, TuiColorBlack, TuiColorWhite);
    for (UINT32 j = 1; j < WIZARD_WIDTH - 1; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, Y + 2, gBoxChars.SingleHorizontal, TuiColorBlack, TuiColorWhite);
    }
    Screen->Vtbl->WriteChar(Screen, X + WIZARD_WIDTH - 1, Y + 2, gBoxChars.SingleTeeRight, TuiColorBlack, TuiColorWhite);

    /* Draw page title and description */
    Screen->Vtbl->WriteText(Screen, X + 3, Y + 4, page->Title, TuiColorBlack, TuiColorWhite);

    /* Word-wrap description */
    CHAR8 desc[256];
    strncpy(desc, page->Description, sizeof(desc) - 1);
    desc[sizeof(desc) - 1] = '\0';

    CHAR8 *line = desc;
    UINT32 descY = Y + 6;
    while (*line && descY < Y + WIZARD_HEIGHT - 8) {
        CHAR8 displayLine[WIZARD_WIDTH - 6];
        UINTN lineLen = 0;
        CHAR8 *wordStart = line;

        while (*line && lineLen < WIZARD_WIDTH - 8) {
            if (*line == ' ' || *line == '\n') {
                if (*line == '\n') {
                    line++;
                    break;
                }
                line++;
                wordStart = line;
            } else {
                displayLine[lineLen++] = *line++;
            }
        }

        displayLine[lineLen] = '\0';
        Screen->Vtbl->WriteText(Screen, X + 3, descY++, displayLine, TuiColorBrightBlack, TuiColorWhite);

        if (*line == '\0') break;
    }

    /* Draw progress indicator */
    UINT32 progressY = Y + WIZARD_HEIGHT - 6;
    Screen->Vtbl->WriteText(Screen, X + 3, progressY, "Progress:", TuiColorBlack, TuiColorWhite);

    UINT32 progressBarWidth = WIZARD_WIDTH - 14;
    UINT32 progressFilled = (progressBarWidth * (impl->CurrentPage + 1)) / impl->PageCount;

    CHAR8 progressBar[128] = {0};
    progressBar[0] = '[';
    for (UINT32 i = 0; i < progressBarWidth; i++) {
        progressBar[i + 1] = (i < progressFilled) ? gBoxChars.Block : gBoxChars.ShadeLight;
    }
    progressBar[progressBarWidth + 1] = ']';
    progressBar[progressBarWidth + 2] = '\0';

    Screen->Vtbl->WriteText(Screen, X + 13, progressY, progressBar, TuiColorBlack, TuiColorWhite);

    /* Draw separator before buttons */
    Screen->Vtbl->WriteChar(Screen, X, Y + WIZARD_HEIGHT - 4, gBoxChars.SingleTeeLeft, TuiColorBlack, TuiColorWhite);
    for (UINT32 j = 1; j < WIZARD_WIDTH - 1; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, Y + WIZARD_HEIGHT - 4, gBoxChars.SingleHorizontal, TuiColorBlack, TuiColorWhite);
    }
    Screen->Vtbl->WriteChar(Screen, X + WIZARD_WIDTH - 1, Y + WIZARD_HEIGHT - 4, gBoxChars.SingleTeeRight, TuiColorBlack, TuiColorWhite);

    /* Draw buttons */
    INT32 buttonY = Y + WIZARD_HEIGHT - 3;

    /* Back button (disabled on first page) */
    if (impl->BackButton) {
        impl->BackButton->Vtbl->SetEnabled(impl->BackButton, impl->CurrentPage > 0);
        impl->BackButton->Vtbl->Render(impl->BackButton, Screen, X + 3, buttonY, FALSE);
    }

    /* Next button (hidden on last page) */
    if (impl->NextButton && impl->CurrentPage < (INT32)impl->PageCount - 1) {
        impl->NextButton->Vtbl->Render(impl->NextButton, Screen, X + 15, buttonY, FALSE);
    }

    /* Finish button (only on last page) */
    if (impl->FinishButton && impl->CurrentPage == (INT32)impl->PageCount - 1) {
        impl->FinishButton->Vtbl->Render(impl->FinishButton, Screen, X + 15, buttonY, FALSE);
    }

    /* Cancel button */
    if (impl->CancelButton) {
        impl->CancelButton->Vtbl->Render(impl->CancelButton, Screen, X + WIZARD_WIDTH - 14, buttonY, FALSE);
    }

    return S_OK;
}

/* Handle keyboard input */
static HRESULT ANXAPI Wizard_HandleKey(
    ITuiWizard *This,
    TUI_KEY Key
)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;

    if (Key == TuiKeyEscape) {
        impl->Cancelled = TRUE;
        if (impl->OnCancel) {
            impl->OnCancel(impl->UserData);
        }
        impl->State.Visible = FALSE;
        return S_OK;
    }

    /* Handle button shortcuts */
    if (Key == 'b' || Key == 'B') {
        /* Back */
        if (impl->CurrentPage > 0) {
            This->Vtbl->GoBack(This);
        }
        return S_OK;
    }

    if (Key == 'n' || Key == 'N' || Key == TuiKeyEnter) {
        /* Next */
        if (impl->CurrentPage < (INT32)impl->PageCount - 1) {
            This->Vtbl->GoNext(This);
        } else {
            /* Finish on last page */
            This->Vtbl->Finish(This);
        }
        return S_OK;
    }

    if (Key == 'f' || Key == 'F') {
        /* Finish */
        if (impl->CurrentPage == (INT32)impl->PageCount - 1) {
            This->Vtbl->Finish(This);
        }
        return S_OK;
    }

    return S_OK;
}

/* Add a page to the wizard */
static HRESULT ANXAPI Wizard_AddPage(
    ITuiWizard *This,
    CONST CHAR8 *Title,
    CONST CHAR8 *Description,
    VOID *PageWidget,
    HRESULT (*OnEnter)(VOID *PageWidget, VOID *UserData),
    HRESULT (*OnLeave)(VOID *PageWidget, VOID *UserData, BOOLEAN *AllowLeave),
    HRESULT (*OnValidate)(VOID *PageWidget, VOID *UserData, BOOLEAN *IsValid),
    VOID *UserData
)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;

    if (impl->PageCount >= MAX_WIZARD_PAGES) {
        return E_OUTOFMEMORY;
    }

    WizardPage *page = &impl->Pages[impl->PageCount++];

    strncpy(page->Title, Title ? Title : "", sizeof(page->Title) - 1);
    page->Title[sizeof(page->Title) - 1] = '\0';

    strncpy(page->Description, Description ? Description : "", sizeof(page->Description) - 1);
    page->Description[sizeof(page->Description) - 1] = '\0';

    page->Widget = PageWidget;
    page->OnEnter = OnEnter;
    page->OnLeave = OnLeave;
    page->OnValidate = OnValidate;
    page->UserData = UserData;

    return S_OK;
}

/* Go to next page */
static HRESULT ANXAPI Wizard_GoNext(ITuiWizard *This)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;

    if (impl->CurrentPage >= (INT32)impl->PageCount - 1) {
        return E_FAIL;  /* Already on last page */
    }

    WizardPage *currentPage = &impl->Pages[impl->CurrentPage];

    /* Validate current page */
    if (currentPage->OnValidate) {
        BOOLEAN isValid = FALSE;
        currentPage->OnValidate(currentPage->Widget, currentPage->UserData, &isValid);
        if (!isValid) {
            return E_FAIL;  /* Validation failed */
        }
    }

    /* OnLeave callback */
    if (currentPage->OnLeave) {
        BOOLEAN allowLeave = TRUE;
        currentPage->OnLeave(currentPage->Widget, currentPage->UserData, &allowLeave);
        if (!allowLeave) {
            return E_FAIL;
        }
    }

    /* Move to next page */
    impl->CurrentPage++;

    /* OnEnter callback */
    WizardPage *nextPage = &impl->Pages[impl->CurrentPage];
    if (nextPage->OnEnter) {
        nextPage->OnEnter(nextPage->Widget, nextPage->UserData);
    }

    return S_OK;
}

/* Go to previous page */
static HRESULT ANXAPI Wizard_GoBack(ITuiWizard *This)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;

    if (impl->CurrentPage <= 0) {
        return E_FAIL;  /* Already on first page */
    }

    WizardPage *currentPage = &impl->Pages[impl->CurrentPage];

    /* OnLeave callback */
    if (currentPage->OnLeave) {
        BOOLEAN allowLeave = TRUE;
        currentPage->OnLeave(currentPage->Widget, currentPage->UserData, &allowLeave);
        if (!allowLeave) {
            return E_FAIL;
        }
    }

    /* Move to previous page */
    impl->CurrentPage--;

    /* OnEnter callback */
    WizardPage *prevPage = &impl->Pages[impl->CurrentPage];
    if (prevPage->OnEnter) {
        prevPage->OnEnter(prevPage->Widget, prevPage->UserData);
    }

    return S_OK;
}

/* Finish the wizard */
static HRESULT ANXAPI Wizard_Finish(ITuiWizard *This)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;

    /* Validate last page */
    if (impl->CurrentPage >= 0 && impl->CurrentPage < (INT32)impl->PageCount) {
        WizardPage *currentPage = &impl->Pages[impl->CurrentPage];

        if (currentPage->OnValidate) {
            BOOLEAN isValid = FALSE;
            currentPage->OnValidate(currentPage->Widget, currentPage->UserData, &isValid);
            if (!isValid) {
                return E_FAIL;
            }
        }
    }

    impl->Finished = TRUE;

    if (impl->OnFinish) {
        impl->OnFinish(impl->UserData);
    }

    impl->State.Visible = FALSE;
    return S_OK;
}

/* Reset wizard to first page */
static HRESULT ANXAPI Wizard_Reset(ITuiWizard *This)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;

    impl->CurrentPage = 0;
    impl->Cancelled = FALSE;
    impl->Finished = FALSE;

    if (impl->PageCount > 0) {
        WizardPage *firstPage = &impl->Pages[0];
        if (firstPage->OnEnter) {
            firstPage->OnEnter(firstPage->Widget, firstPage->UserData);
        }
    }

    return S_OK;
}

/* Set finish callback */
static HRESULT ANXAPI Wizard_SetFinishCallback(
    ITuiWizard *This,
    HRESULT (*Callback)(VOID *UserData),
    VOID *UserData
)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;
    impl->OnFinish = Callback;
    impl->UserData = UserData;
    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI Wizard_SetBounds(ITuiWizard *This, CONST TUI_RECT *Bounds)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI Wizard_GetBounds(ITuiWizard *This, TUI_RECT *Bounds)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI Wizard_SetVisible(ITuiWizard *This, BOOLEAN Visible)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI Wizard_IsVisible(ITuiWizard *This)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI Wizard_SetEnabled(ITuiWizard *This, BOOLEAN Enabled)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI Wizard_IsEnabled(ITuiWizard *This)
{
    TuiWizardImpl *impl = (TuiWizardImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiWizardVtbl WizardVtbl = {
    Wizard_QueryInterface,
    Wizard_AddRef,
    Wizard_Release,
    Wizard_Render,
    Wizard_HandleKey,
    Wizard_SetBounds,
    Wizard_GetBounds,
    Wizard_SetVisible,
    Wizard_IsVisible,
    Wizard_SetEnabled,
    Wizard_IsEnabled,
    Wizard_AddPage,
    Wizard_GoNext,
    Wizard_GoBack,
    Wizard_Finish,
    Wizard_Reset,
    Wizard_SetFinishCallback
};

/* Factory function */
HRESULT AnxTuiCreateWizard(ITuiWizard **OutWizard)
{
    TuiWizardImpl *impl;
    HRESULT hr;

    if (!OutWizard) return E_INVALIDARG;

    impl = (TuiWizardImpl *)malloc(sizeof(TuiWizardImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiWizardImpl));
    impl->Interface.Vtbl = &WizardVtbl;
    InitWidgetState(&impl->State);

    impl->CurrentPage = 0;
    impl->Cancelled = FALSE;
    impl->Finished = FALSE;

    /* Create buttons */
    hr = AnxTuiCreateButton("< Back", NULL, NULL, &impl->BackButton);
    if (FAILED(hr)) {
        free(impl);
        return hr;
    }

    hr = AnxTuiCreateButton("Next >", NULL, NULL, &impl->NextButton);
    if (FAILED(hr)) {
        impl->BackButton->Vtbl->Release(impl->BackButton);
        free(impl);
        return hr;
    }

    hr = AnxTuiCreateButton("Finish", NULL, NULL, &impl->FinishButton);
    if (FAILED(hr)) {
        impl->BackButton->Vtbl->Release(impl->BackButton);
        impl->NextButton->Vtbl->Release(impl->NextButton);
        free(impl);
        return hr;
    }

    hr = AnxTuiCreateButton("Cancel", NULL, NULL, &impl->CancelButton);
    if (FAILED(hr)) {
        impl->BackButton->Vtbl->Release(impl->BackButton);
        impl->NextButton->Vtbl->Release(impl->NextButton);
        impl->FinishButton->Vtbl->Release(impl->FinishButton);
        free(impl);
        return hr;
    }

    impl->State.Bounds.Width = WIZARD_WIDTH;
    impl->State.Bounds.Height = WIZARD_HEIGHT;

    *OutWizard = &impl->Interface;
    return S_OK;
}
