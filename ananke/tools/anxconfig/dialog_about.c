/*
 * About Dialog Implementation
 *
 * Standard about box showing application information,
 * version, copyright, and credits.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

#define MAX_LINES 32
#define MAX_LINE_LENGTH 128

typedef struct {
    ITuiAboutDialog Interface;
    WIDGET_STATE State;
    CHAR8 Title[128];
    CHAR8 AppName[128];
    CHAR8 Version[64];
    CHAR8 Copyright[256];
    CHAR8 Description[512];
    CHAR8 Credits[MAX_LINES][MAX_LINE_LENGTH];
    UINT32 CreditCount;
    CHAR8 Website[256];
    CHAR8 License[128];
    ITuiButton *OkButton;
    UINT32 Width;
    UINT32 Height;
} TuiAboutDialogImpl;

/* IUnknown methods */
static HRESULT ANXAPI AboutDialog_QueryInterface(
    ITuiAboutDialog *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI AboutDialog_AddRef(ITuiAboutDialog *This)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI AboutDialog_Release(ITuiAboutDialog *This)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->OkButton) impl->OkButton->Vtbl->Release(impl->OkButton);
        free(impl);
    }
    return refCount;
}

/* ITuiAboutDialog methods */
static HRESULT ANXAPI AboutDialog_SetAppName(
    ITuiAboutDialog *This,
    CONST CHAR8 *AppName
)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;
    if (AppName == NULL) return E_POINTER;

    strncpy(impl->AppName, AppName, sizeof(impl->AppName) - 1);
    impl->AppName[sizeof(impl->AppName) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI AboutDialog_SetVersion(
    ITuiAboutDialog *This,
    CONST CHAR8 *Version
)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;
    if (Version == NULL) return E_POINTER;

    strncpy(impl->Version, Version, sizeof(impl->Version) - 1);
    impl->Version[sizeof(impl->Version) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI AboutDialog_SetCopyright(
    ITuiAboutDialog *This,
    CONST CHAR8 *Copyright
)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;
    if (Copyright == NULL) return E_POINTER;

    strncpy(impl->Copyright, Copyright, sizeof(impl->Copyright) - 1);
    impl->Copyright[sizeof(impl->Copyright) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI AboutDialog_SetDescription(
    ITuiAboutDialog *This,
    CONST CHAR8 *Description
)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;
    if (Description == NULL) return E_POINTER;

    strncpy(impl->Description, Description, sizeof(impl->Description) - 1);
    impl->Description[sizeof(impl->Description) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI AboutDialog_SetWebsite(
    ITuiAboutDialog *This,
    CONST CHAR8 *Website
)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;
    if (Website == NULL) return E_POINTER;

    strncpy(impl->Website, Website, sizeof(impl->Website) - 1);
    impl->Website[sizeof(impl->Website) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI AboutDialog_SetLicense(
    ITuiAboutDialog *This,
    CONST CHAR8 *License
)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;
    if (License == NULL) return E_POINTER;

    strncpy(impl->License, License, sizeof(impl->License) - 1);
    impl->License[sizeof(impl->License) - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI AboutDialog_AddCredit(
    ITuiAboutDialog *This,
    CONST CHAR8 *Credit
)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;

    if (Credit == NULL) return E_POINTER;
    if (impl->CreditCount >= MAX_LINES) return E_OUTOFMEMORY;

    strncpy(impl->Credits[impl->CreditCount], Credit, MAX_LINE_LENGTH - 1);
    impl->Credits[impl->CreditCount][MAX_LINE_LENGTH - 1] = '\0';
    impl->CreditCount++;

    return S_OK;
}

static HRESULT ANXAPI AboutDialog_Show(
    ITuiAboutDialog *This,
    ITuiScreen *Screen
)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;
    UINT32 screenWidth, screenHeight;

    Screen->Vtbl->GetDimensions(Screen, &screenWidth, &screenHeight);

    /* Center dialog */
    INT32 x = (screenWidth - impl->Width) / 2;
    INT32 y = (screenHeight - impl->Height) / 2;

    impl->State.Visible = TRUE;

    /* Render */
    AboutDialog_Render(This, Screen, x, y);
    Screen->Vtbl->Refresh(Screen);

    /* Enter modal loop (simplified) */
    return S_OK;
}

static HRESULT ANXAPI AboutDialog_Render(
    ITuiAboutDialog *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;
    UINT32 i;
    INT32 currentY = Y + 2;
    CHAR8 display[256];

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

    /* Draw application name (large/bold) */
    if (strlen(impl->AppName) > 0) {
        Screen->Vtbl->WriteText(Screen, X + (impl->Width - strlen(impl->AppName)) / 2,
                                currentY, impl->AppName,
                                TuiColorBlue, TuiColorWhite);
        currentY += 2;
    }

    /* Draw version */
    if (strlen(impl->Version) > 0) {
        snprintf(display, sizeof(display), "Version %s", impl->Version);
        Screen->Vtbl->WriteText(Screen, X + (impl->Width - strlen(display)) / 2,
                                currentY, display,
                                TuiColorBlack, TuiColorWhite);
        currentY += 2;
    }

    /* Draw separator line */
    for (i = 2; i < impl->Width - 2; i++) {
        Screen->Vtbl->WriteText(Screen, X + i, currentY, "─",
                                TuiColorBlack, TuiColorWhite);
    }
    currentY += 2;

    /* Draw description */
    if (strlen(impl->Description) > 0) {
        /* Simple word wrap */
        UINT32 maxWidth = impl->Width - 6;
        CONST CHAR8 *p = impl->Description;
        CHAR8 line[256];
        UINT32 linePos = 0;

        while (*p != '\0' && currentY < Y + impl->Height - 6) {
            if (*p == ' ' && linePos > maxWidth) {
                line[linePos] = '\0';
                Screen->Vtbl->WriteText(Screen, X + 3, currentY, line,
                                        TuiColorBlack, TuiColorWhite);
                currentY++;
                linePos = 0;
                p++;
                continue;
            }
            line[linePos++] = *p++;
        }
        if (linePos > 0) {
            line[linePos] = '\0';
            Screen->Vtbl->WriteText(Screen, X + 3, currentY, line,
                                    TuiColorBlack, TuiColorWhite);
            currentY++;
        }
        currentY++;
    }

    /* Draw copyright */
    if (strlen(impl->Copyright) > 0) {
        Screen->Vtbl->WriteText(Screen, X + 3, currentY, impl->Copyright,
                                TuiColorBlack, TuiColorWhite);
        currentY++;
    }

    /* Draw license */
    if (strlen(impl->License) > 0) {
        snprintf(display, sizeof(display), "License: %s", impl->License);
        Screen->Vtbl->WriteText(Screen, X + 3, currentY, display,
                                TuiColorBlack, TuiColorWhite);
        currentY++;
    }

    /* Draw website */
    if (strlen(impl->Website) > 0) {
        Screen->Vtbl->WriteText(Screen, X + 3, currentY, impl->Website,
                                TuiColorBlue, TuiColorWhite);
        currentY++;
    }

    /* Draw credits */
    if (impl->CreditCount > 0 && currentY < Y + impl->Height - 4) {
        currentY++;
        Screen->Vtbl->WriteText(Screen, X + 3, currentY, "Credits:",
                                TuiColorBlack, TuiColorWhite);
        currentY++;

        for (i = 0; i < impl->CreditCount && currentY < Y + impl->Height - 3; i++) {
            Screen->Vtbl->WriteText(Screen, X + 5, currentY, impl->Credits[i],
                                    TuiColorBlack, TuiColorWhite);
            currentY++;
        }
    }

    /* Draw OK button */
    if (impl->OkButton) {
        impl->OkButton->Vtbl->Render(impl->OkButton, Screen,
                                      X + (impl->Width - 8) / 2,
                                      Y + impl->Height - 2, FALSE);
    }

    return S_OK;
}

static HRESULT ANXAPI AboutDialog_HandleKey(
    ITuiAboutDialog *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiAboutDialogImpl *impl = (TuiAboutDialogImpl *)This;

    if (!impl->State.Visible) {
        *Handled = FALSE;
        return S_OK;
    }

    if (Key == TuiKeyEnter || Key == TuiKeyEsc || Key == ' ') {
        /* Close dialog */
        impl->State.Visible = FALSE;
        *Handled = TRUE;
        return S_OK;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiAboutDialog_Vtbl AboutDialogVtbl = {
    AboutDialog_QueryInterface,
    AboutDialog_AddRef,
    AboutDialog_Release,
    AboutDialog_SetAppName,
    AboutDialog_SetVersion,
    AboutDialog_SetCopyright,
    AboutDialog_SetDescription,
    AboutDialog_SetWebsite,
    AboutDialog_SetLicense,
    AboutDialog_AddCredit,
    AboutDialog_Show,
    AboutDialog_Render,
    AboutDialog_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateAboutDialog(OUT ITuiAboutDialog **AboutDialog)
{
    TuiAboutDialogImpl *impl;
    HRESULT hr;

    if (AboutDialog == NULL) return E_POINTER;

    impl = (TuiAboutDialogImpl *)calloc(1, sizeof(TuiAboutDialogImpl));
    if (impl == NULL) {
        *AboutDialog = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &AboutDialogVtbl;
    InitWidgetState(&impl->State);

    strcpy(impl->Title, "About");
    impl->AppName[0] = '\0';
    impl->Version[0] = '\0';
    impl->Copyright[0] = '\0';
    impl->Description[0] = '\0';
    impl->Website[0] = '\0';
    impl->License[0] = '\0';
    impl->CreditCount = 0;
    impl->Width = 60;
    impl->Height = 20;

    /* Create OK button */
    hr = AnxTuiCreateButton(&impl->OkButton);
    if (FAILED(hr)) {
        free(impl);
        *AboutDialog = NULL;
        return hr;
    }
    impl->OkButton->Vtbl->SetLabel(impl->OkButton, "  OK  ");

    *AboutDialog = &impl->Interface;
    return S_OK;
}

/* Helper function: Show simple about dialog */
HRESULT ANXAPI AnxTuiShowAboutDialog(
    ITuiScreen *Screen,
    CONST CHAR8 *AppName,
    CONST CHAR8 *Version,
    CONST CHAR8 *Copyright
)
{
    ITuiAboutDialog *dialog;
    HRESULT hr;

    hr = AnxTuiCreateAboutDialog(&dialog);
    if (FAILED(hr)) return hr;

    if (AppName) dialog->Vtbl->SetAppName(dialog, AppName);
    if (Version) dialog->Vtbl->SetVersion(dialog, Version);
    if (Copyright) dialog->Vtbl->SetCopyright(dialog, Copyright);

    hr = dialog->Vtbl->Show(dialog, Screen);
    dialog->Vtbl->Release(dialog);

    return hr;
}
