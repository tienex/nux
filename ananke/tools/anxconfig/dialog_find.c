/*
 * Find and Find/Replace Dialog Implementation
 *
 * Common dialog for finding text and optionally replacing it.
 * Supports case-sensitive search, whole word matching, and regex.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "widgets_common.h"

typedef struct {
    ITuiFindDialog Interface;
    WIDGET_STATE State;
    CHAR8 Title[128];
    CHAR8 FindText[256];
    CHAR8 ReplaceText[256];
    BOOLEAN CaseSensitive;
    BOOLEAN WholeWord;
    BOOLEAN UseRegex;
    BOOLEAN ReplaceMode;
    BOOLEAN SearchBackward;
    BOOLEAN WrapAround;
    ITuiInput *FindInput;
    ITuiInput *ReplaceInput;
    ITuiCheckbox *CaseCheckbox;
    ITuiCheckbox *WholeWordCheckbox;
    ITuiCheckbox *RegexCheckbox;
    ITuiCheckbox *BackwardCheckbox;
    ITuiCheckbox *WrapCheckbox;
    ITuiButton *FindNextButton;
    ITuiButton *ReplaceButton;
    ITuiButton *ReplaceAllButton;
    ITuiButton *CloseButton;
    UINT32 Width;
    UINT32 Height;
} TuiFindDialogImpl;

/* IUnknown methods */
static HRESULT ANXAPI FindDialog_QueryInterface(
    ITuiFindDialog *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI FindDialog_AddRef(ITuiFindDialog *This)
{
    TuiFindDialogImpl *impl = (TuiFindDialogImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI FindDialog_Release(ITuiFindDialog *This)
{
    TuiFindDialogImpl *impl = (TuiFindDialogImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->FindInput) impl->FindInput->Vtbl->Release(impl->FindInput);
        if (impl->ReplaceInput) impl->ReplaceInput->Vtbl->Release(impl->ReplaceInput);
        if (impl->CaseCheckbox) impl->CaseCheckbox->Vtbl->Release(impl->CaseCheckbox);
        if (impl->WholeWordCheckbox) impl->WholeWordCheckbox->Vtbl->Release(impl->WholeWordCheckbox);
        if (impl->RegexCheckbox) impl->RegexCheckbox->Vtbl->Release(impl->RegexCheckbox);
        if (impl->BackwardCheckbox) impl->BackwardCheckbox->Vtbl->Release(impl->BackwardCheckbox);
        if (impl->WrapCheckbox) impl->WrapCheckbox->Vtbl->Release(impl->WrapCheckbox);
        if (impl->FindNextButton) impl->FindNextButton->Vtbl->Release(impl->FindNextButton);
        if (impl->ReplaceButton) impl->ReplaceButton->Vtbl->Release(impl->ReplaceButton);
        if (impl->ReplaceAllButton) impl->ReplaceAllButton->Vtbl->Release(impl->ReplaceAllButton);
        if (impl->CloseButton) impl->CloseButton->Vtbl->Release(impl->CloseButton);
        free(impl);
    }
    return refCount;
}

/* ITuiFindDialog methods */
static HRESULT ANXAPI FindDialog_SetReplaceMode(
    ITuiFindDialog *This,
    BOOLEAN ReplaceMode
)
{
    TuiFindDialogImpl *impl = (TuiFindDialogImpl *)This;
    impl->ReplaceMode = ReplaceMode;

    if (ReplaceMode) {
        strcpy(impl->Title, "Find and Replace");
        impl->Height = 20;
    } else {
        strcpy(impl->Title, "Find");
        impl->Height = 16;
    }

    return S_OK;
}

static HRESULT ANXAPI FindDialog_SetFindText(
    ITuiFindDialog *This,
    CONST CHAR8 *Text
)
{
    TuiFindDialogImpl *impl = (TuiFindDialogImpl *)This;
    if (Text == NULL) return E_POINTER;

    strncpy(impl->FindText, Text, sizeof(impl->FindText) - 1);
    impl->FindText[sizeof(impl->FindText) - 1] = '\0';

    if (impl->FindInput) {
        impl->FindInput->Vtbl->SetValue(impl->FindInput, Text);
    }

    return S_OK;
}

static HRESULT ANXAPI FindDialog_GetFindText(
    ITuiFindDialog *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiFindDialogImpl *impl = (TuiFindDialogImpl *)This;
    if (Buffer == NULL) return E_POINTER;

    /* Get from input if available */
    if (impl->FindInput) {
        impl->FindInput->Vtbl->GetValue(impl->FindInput, impl->FindText,
                                         sizeof(impl->FindText));
    }

    strncpy(Buffer, impl->FindText, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI FindDialog_GetReplaceText(
    ITuiFindDialog *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiFindDialogImpl *impl = (TuiFindDialogImpl *)This;
    if (Buffer == NULL) return E_POINTER;

    /* Get from input if available */
    if (impl->ReplaceInput) {
        impl->ReplaceInput->Vtbl->GetValue(impl->ReplaceInput, impl->ReplaceText,
                                            sizeof(impl->ReplaceText));
    }

    strncpy(Buffer, impl->ReplaceText, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI FindDialog_GetOptions(
    ITuiFindDialog *This,
    BOOLEAN *CaseSensitive,
    BOOLEAN *WholeWord,
    BOOLEAN *UseRegex,
    BOOLEAN *SearchBackward,
    BOOLEAN *WrapAround
)
{
    TuiFindDialogImpl *impl = (TuiFindDialogImpl *)This;

    /* Update from checkboxes */
    if (impl->CaseCheckbox) {
        impl->CaseSensitive = impl->CaseCheckbox->Vtbl->GetChecked(impl->CaseCheckbox);
    }
    if (impl->WholeWordCheckbox) {
        impl->WholeWord = impl->WholeWordCheckbox->Vtbl->GetChecked(impl->WholeWordCheckbox);
    }
    if (impl->RegexCheckbox) {
        impl->UseRegex = impl->RegexCheckbox->Vtbl->GetChecked(impl->RegexCheckbox);
    }
    if (impl->BackwardCheckbox) {
        impl->SearchBackward = impl->BackwardCheckbox->Vtbl->GetChecked(impl->BackwardCheckbox);
    }
    if (impl->WrapCheckbox) {
        impl->WrapAround = impl->WrapCheckbox->Vtbl->GetChecked(impl->WrapCheckbox);
    }

    if (CaseSensitive) *CaseSensitive = impl->CaseSensitive;
    if (WholeWord) *WholeWord = impl->WholeWord;
    if (UseRegex) *UseRegex = impl->UseRegex;
    if (SearchBackward) *SearchBackward = impl->SearchBackward;
    if (WrapAround) *WrapAround = impl->WrapAround;

    return S_OK;
}

static HRESULT ANXAPI FindDialog_Show(
    ITuiFindDialog *This,
    ITuiScreen *Screen
)
{
    TuiFindDialogImpl *impl = (TuiFindDialogImpl *)This;
    UINT32 screenWidth, screenHeight;

    impl->State.Visible = TRUE;

    Screen->Vtbl->GetDimensions(Screen, &screenWidth, &screenHeight);

    /* Center dialog */
    INT32 x = (screenWidth - impl->Width) / 2;
    INT32 y = (screenHeight - impl->Height) / 2;

    /* Render */
    FindDialog_Render(This, Screen, x, y);
    Screen->Vtbl->Refresh(Screen);

    return S_OK;
}

static HRESULT ANXAPI FindDialog_Render(
    ITuiFindDialog *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiFindDialogImpl *impl = (TuiFindDialogImpl *)This;
    UINT32 i;
    INT32 currentY;

    if (!impl->State.Visible) return S_OK;

    /* Draw shadow */
    for (i = 0; i < impl->Height + 1; i++) {
        ClearRect(Screen, X + 2, Y + i + 1, impl->Width, 1, TuiColorBlack);
    }

    /* Draw dialog box */
    DrawBoxSingle(Screen, X, Y, impl->Width, impl->Height,
                  TuiColorBlack, TuiColorWhite);

    /* Draw title bar */
    CHAR8 titleBar[128];
    snprintf(titleBar, sizeof(titleBar), " %s ", impl->Title);
    Screen->Vtbl->WriteText(Screen, X + 2, Y, titleBar,
                            TuiColorWhite, TuiColorBlue);

    currentY = Y + 2;

    /* Find text label */
    Screen->Vtbl->WriteText(Screen, X + 2, currentY, "Find what:",
                            TuiColorBlack, TuiColorWhite);
    currentY++;

    /* Find input */
    if (impl->FindInput) {
        impl->FindInput->Vtbl->Render(impl->FindInput, Screen,
                                       X + 4, currentY, FALSE);
    }
    currentY += 2;

    /* Replace text (if in replace mode) */
    if (impl->ReplaceMode) {
        Screen->Vtbl->WriteText(Screen, X + 2, currentY, "Replace with:",
                                TuiColorBlack, TuiColorWhite);
        currentY++;

        if (impl->ReplaceInput) {
            impl->ReplaceInput->Vtbl->Render(impl->ReplaceInput, Screen,
                                              X + 4, currentY, FALSE);
        }
        currentY += 2;
    }

    /* Options */
    Screen->Vtbl->WriteText(Screen, X + 2, currentY, "Options:",
                            TuiColorBlack, TuiColorWhite);
    currentY++;

    if (impl->CaseCheckbox) {
        impl->CaseCheckbox->Vtbl->Render(impl->CaseCheckbox, Screen,
                                          X + 4, currentY++, FALSE);
    }
    if (impl->WholeWordCheckbox) {
        impl->WholeWordCheckbox->Vtbl->Render(impl->WholeWordCheckbox, Screen,
                                                X + 4, currentY++, FALSE);
    }
    if (impl->RegexCheckbox) {
        impl->RegexCheckbox->Vtbl->Render(impl->RegexCheckbox, Screen,
                                            X + 4, currentY++, FALSE);
    }
    if (impl->BackwardCheckbox) {
        impl->BackwardCheckbox->Vtbl->Render(impl->BackwardCheckbox, Screen,
                                               X + 4, currentY++, FALSE);
    }
    if (impl->WrapCheckbox) {
        impl->WrapCheckbox->Vtbl->Render(impl->WrapCheckbox, Screen,
                                           X + 4, currentY++, FALSE);
    }

    /* Buttons */
    currentY = Y + impl->Height - 2;
    INT32 buttonX = X + 2;

    if (impl->FindNextButton) {
        impl->FindNextButton->Vtbl->Render(impl->FindNextButton, Screen,
                                            buttonX, currentY, FALSE);
        buttonX += 14;
    }

    if (impl->ReplaceMode) {
        if (impl->ReplaceButton) {
            impl->ReplaceButton->Vtbl->Render(impl->ReplaceButton, Screen,
                                               buttonX, currentY, FALSE);
            buttonX += 12;
        }

        if (impl->ReplaceAllButton) {
            impl->ReplaceAllButton->Vtbl->Render(impl->ReplaceAllButton, Screen,
                                                   buttonX, currentY, FALSE);
            buttonX += 14;
        }
    }

    if (impl->CloseButton) {
        impl->CloseButton->Vtbl->Render(impl->CloseButton, Screen,
                                          X + impl->Width - 10, currentY, FALSE);
    }

    return S_OK;
}

static HRESULT ANXAPI FindDialog_HandleKey(
    ITuiFindDialog *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiFindDialogImpl *impl = (TuiFindDialogImpl *)This;

    if (!impl->State.Visible) {
        *Handled = FALSE;
        return S_OK;
    }

    if (Key == TuiKeyEsc) {
        impl->State.Visible = FALSE;
        *Handled = TRUE;
        return S_OK;
    }

    /* Route to active widget */
    /* TODO: Implement proper focus management */

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiFindDialog_Vtbl FindDialogVtbl = {
    FindDialog_QueryInterface,
    FindDialog_AddRef,
    FindDialog_Release,
    FindDialog_SetReplaceMode,
    FindDialog_SetFindText,
    FindDialog_GetFindText,
    FindDialog_GetReplaceText,
    FindDialog_GetOptions,
    FindDialog_Show,
    FindDialog_Render,
    FindDialog_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateFindDialog(
    IN  BOOLEAN ReplaceMode,
    OUT ITuiFindDialog **FindDialog
)
{
    TuiFindDialogImpl *impl;
    HRESULT hr;

    if (FindDialog == NULL) return E_POINTER;

    impl = (TuiFindDialogImpl *)calloc(1, sizeof(TuiFindDialogImpl));
    if (impl == NULL) {
        *FindDialog = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &FindDialogVtbl;
    InitWidgetState(&impl->State);

    impl->ReplaceMode = ReplaceMode;
    impl->FindText[0] = '\0';
    impl->ReplaceText[0] = '\0';
    impl->CaseSensitive = FALSE;
    impl->WholeWord = FALSE;
    impl->UseRegex = FALSE;
    impl->SearchBackward = FALSE;
    impl->WrapAround = TRUE;
    impl->Width = 60;

    /* Create widgets */
    hr = AnxTuiCreateInput("", &impl->FindInput);
    if (FAILED(hr)) goto cleanup;
    impl->FindInput->Vtbl->SetWidth(impl->FindInput, 50);

    if (ReplaceMode) {
        hr = AnxTuiCreateInput("", &impl->ReplaceInput);
        if (FAILED(hr)) goto cleanup;
        impl->ReplaceInput->Vtbl->SetWidth(impl->ReplaceInput, 50);
    }

    hr = AnxTuiCreateCheckbox(&impl->CaseCheckbox);
    if (FAILED(hr)) goto cleanup;
    impl->CaseCheckbox->Vtbl->SetLabel(impl->CaseCheckbox, "Case sensitive");

    hr = AnxTuiCreateCheckbox(&impl->WholeWordCheckbox);
    if (FAILED(hr)) goto cleanup;
    impl->WholeWordCheckbox->Vtbl->SetLabel(impl->WholeWordCheckbox, "Whole word");

    hr = AnxTuiCreateCheckbox(&impl->RegexCheckbox);
    if (FAILED(hr)) goto cleanup;
    impl->RegexCheckbox->Vtbl->SetLabel(impl->RegexCheckbox, "Regular expression");

    hr = AnxTuiCreateCheckbox(&impl->BackwardCheckbox);
    if (FAILED(hr)) goto cleanup;
    impl->BackwardCheckbox->Vtbl->SetLabel(impl->BackwardCheckbox, "Search backward");

    hr = AnxTuiCreateCheckbox(&impl->WrapCheckbox);
    if (FAILED(hr)) goto cleanup;
    impl->WrapCheckbox->Vtbl->SetLabel(impl->WrapCheckbox, "Wrap around");
    impl->WrapCheckbox->Vtbl->SetChecked(impl->WrapCheckbox, TRUE);

    hr = AnxTuiCreateButton(&impl->FindNextButton);
    if (FAILED(hr)) goto cleanup;
    impl->FindNextButton->Vtbl->SetLabel(impl->FindNextButton, " Find Next ");

    if (ReplaceMode) {
        hr = AnxTuiCreateButton(&impl->ReplaceButton);
        if (FAILED(hr)) goto cleanup;
        impl->ReplaceButton->Vtbl->SetLabel(impl->ReplaceButton, " Replace ");

        hr = AnxTuiCreateButton(&impl->ReplaceAllButton);
        if (FAILED(hr)) goto cleanup;
        impl->ReplaceAllButton->Vtbl->SetLabel(impl->ReplaceAllButton, "Replace All");
    }

    hr = AnxTuiCreateButton(&impl->CloseButton);
    if (FAILED(hr)) goto cleanup;
    impl->CloseButton->Vtbl->SetLabel(impl->CloseButton, " Close ");

    FindDialog_SetReplaceMode(&impl->Interface, ReplaceMode);

    *FindDialog = &impl->Interface;
    return S_OK;

cleanup:
    FindDialog_Release(&impl->Interface);
    *FindDialog = NULL;
    return hr;
}
