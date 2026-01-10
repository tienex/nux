/*
 * Print Dialog Implementation
 *
 * Common dialog for printer selection and print job configuration.
 * Supports page range, copies, quality settings, and printer options.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

#define MAX_PRINTERS 32
#define MAX_PRINTER_NAME 128

typedef enum {
    PrintRangeAll,
    PrintRangeSelection,
    PrintRangePages,
    PrintRangeCurrentPage
} PrintRange;

typedef enum {
    PrintQualityDraft,
    PrintQualityNormal,
    PrintQualityHigh,
    PrintQualityBest
} PrintQuality;

typedef struct {
    CHAR8 Name[MAX_PRINTER_NAME];
    CHAR8 Driver[64];
    CHAR8 Port[32];
    BOOLEAN IsDefault;
    BOOLEAN IsAvailable;
} PrinterInfo;

typedef struct {
    ITuiPrintDialog Interface;
    WIDGET_STATE State;
    CHAR8 Title[128];
    PrinterInfo Printers[MAX_PRINTERS];
    UINT32 PrinterCount;
    INT32 SelectedPrinter;
    PrintRange Range;
    UINT32 PageFrom;
    UINT32 PageTo;
    UINT32 Copies;
    BOOLEAN Collate;
    PrintQuality Quality;
    BOOLEAN PrintToFile;
    CHAR8 OutputFile[512];
    BOOLEAN DialogResult;
    ITuiListBox *PrinterList;
    ITuiComboBox *RangeCombo;
    ITuiSpinner *PageFromSpinner;
    ITuiSpinner *PageToSpinner;
    ITuiSpinner *CopiesSpinner;
    ITuiCheckbox *CollateCheckbox;
    ITuiCheckbox *PrintToFileCheckbox;
    ITuiComboBox *QualityCombo;
    ITuiButton *PrintButton;
    ITuiButton *CancelButton;
    ITuiButton *PropertiesButton;
    UINT32 Width;
    UINT32 Height;
} TuiPrintDialogImpl;

/* IUnknown methods */
static HRESULT ANXAPI PrintDialog_QueryInterface(
    ITuiPrintDialog *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI PrintDialog_AddRef(ITuiPrintDialog *This)
{
    TuiPrintDialogImpl *impl = (TuiPrintDialogImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI PrintDialog_Release(ITuiPrintDialog *This)
{
    TuiPrintDialogImpl *impl = (TuiPrintDialogImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->PrinterList) impl->PrinterList->Vtbl->Release(impl->PrinterList);
        if (impl->RangeCombo) impl->RangeCombo->Vtbl->Release(impl->RangeCombo);
        if (impl->PageFromSpinner) impl->PageFromSpinner->Vtbl->Release(impl->PageFromSpinner);
        if (impl->PageToSpinner) impl->PageToSpinner->Vtbl->Release(impl->PageToSpinner);
        if (impl->CopiesSpinner) impl->CopiesSpinner->Vtbl->Release(impl->CopiesSpinner);
        if (impl->CollateCheckbox) impl->CollateCheckbox->Vtbl->Release(impl->CollateCheckbox);
        if (impl->PrintToFileCheckbox) impl->PrintToFileCheckbox->Vtbl->Release(impl->PrintToFileCheckbox);
        if (impl->QualityCombo) impl->QualityCombo->Vtbl->Release(impl->QualityCombo);
        if (impl->PrintButton) impl->PrintButton->Vtbl->Release(impl->PrintButton);
        if (impl->CancelButton) impl->CancelButton->Vtbl->Release(impl->CancelButton);
        if (impl->PropertiesButton) impl->PropertiesButton->Vtbl->Release(impl->PropertiesButton);
        free(impl);
    }
    return refCount;
}

/* Helper: Enumerate printers */
static HRESULT EnumeratePrinters(TuiPrintDialogImpl *impl)
{
    /* Platform-specific printer enumeration would go here */
    /* For now, add mock printers */

    impl->PrinterCount = 0;

    /* Default printer */
    strcpy(impl->Printers[0].Name, "Default Printer");
    strcpy(impl->Printers[0].Driver, "Generic");
    strcpy(impl->Printers[0].Port, "LPT1:");
    impl->Printers[0].IsDefault = TRUE;
    impl->Printers[0].IsAvailable = TRUE;
    impl->PrinterCount++;

    /* PDF printer */
    strcpy(impl->Printers[1].Name, "Microsoft Print to PDF");
    strcpy(impl->Printers[1].Driver, "PDF");
    strcpy(impl->Printers[1].Port, "FILE:");
    impl->Printers[1].IsDefault = FALSE;
    impl->Printers[1].IsAvailable = TRUE;
    impl->PrinterCount++;

    /* PostScript printer */
    strcpy(impl->Printers[2].Name, "PostScript Printer");
    strcpy(impl->Printers[2].Driver, "PostScript");
    strcpy(impl->Printers[2].Port, "LPT2:");
    impl->Printers[2].IsDefault = FALSE;
    impl->Printers[2].IsAvailable = TRUE;
    impl->PrinterCount++;

    /* Update list box */
    if (impl->PrinterList) {
        UINT32 i;
        CHAR8 display[256];

        impl->PrinterList->Vtbl->Clear(impl->PrinterList);

        for (i = 0; i < impl->PrinterCount; i++) {
            snprintf(display, sizeof(display), "%s%s (%s on %s)",
                     impl->Printers[i].IsDefault ? "* " : "  ",
                     impl->Printers[i].Name,
                     impl->Printers[i].Driver,
                     impl->Printers[i].Port);
            impl->PrinterList->Vtbl->AddItem(impl->PrinterList, display, NULL);
        }

        /* Select default printer */
        for (i = 0; i < impl->PrinterCount; i++) {
            if (impl->Printers[i].IsDefault) {
                impl->PrinterList->Vtbl->SetSelectedIndex(impl->PrinterList, i);
                impl->SelectedPrinter = i;
                break;
            }
        }
    }

    return S_OK;
}

/* ITuiPrintDialog methods */
static HRESULT ANXAPI PrintDialog_SetPageRange(
    ITuiPrintDialog *This,
    UINT32 From,
    UINT32 To
)
{
    TuiPrintDialogImpl *impl = (TuiPrintDialogImpl *)This;
    impl->PageFrom = From;
    impl->PageTo = To;

    if (impl->PageFromSpinner) {
        impl->PageFromSpinner->Vtbl->SetValue(impl->PageFromSpinner, From);
    }
    if (impl->PageToSpinner) {
        impl->PageToSpinner->Vtbl->SetValue(impl->PageToSpinner, To);
    }

    return S_OK;
}

static HRESULT ANXAPI PrintDialog_Show(
    ITuiPrintDialog *This,
    ITuiScreen *Screen
)
{
    TuiPrintDialogImpl *impl = (TuiPrintDialogImpl *)This;
    UINT32 screenWidth, screenHeight;

    impl->DialogResult = FALSE;
    impl->State.Visible = TRUE;

    /* Enumerate printers */
    EnumeratePrinters(impl);

    Screen->Vtbl->GetDimensions(Screen, &screenWidth, &screenHeight);

    /* Center dialog */
    INT32 x = (screenWidth - impl->Width) / 2;
    INT32 y = (screenHeight - impl->Height) / 2;

    /* Render */
    PrintDialog_Render(This, Screen, x, y);
    Screen->Vtbl->Refresh(Screen);

    return S_OK;
}

static HRESULT ANXAPI PrintDialog_GetResult(
    ITuiPrintDialog *This,
    BOOLEAN *Accepted
)
{
    TuiPrintDialogImpl *impl = (TuiPrintDialogImpl *)This;
    if (Accepted == NULL) return E_POINTER;
    *Accepted = impl->DialogResult;
    return S_OK;
}

static HRESULT ANXAPI PrintDialog_GetSettings(
    ITuiPrintDialog *This,
    INT32 *PrinterIndex,
    UINT32 *PrintRange,
    UINT32 *PageFrom,
    UINT32 *PageTo,
    UINT32 *Copies
)
{
    TuiPrintDialogImpl *impl = (TuiPrintDialogImpl *)This;

    if (PrinterIndex) *PrinterIndex = impl->SelectedPrinter;
    if (PrintRange) *PrintRange = impl->Range;
    if (PageFrom) *PageFrom = impl->PageFrom;
    if (PageTo) *PageTo = impl->PageTo;
    if (Copies) *Copies = impl->Copies;

    return S_OK;
}

static HRESULT ANXAPI PrintDialog_GetPrinterName(
    ITuiPrintDialog *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiPrintDialogImpl *impl = (TuiPrintDialogImpl *)This;

    if (Buffer == NULL) return E_POINTER;
    if (impl->SelectedPrinter < 0 || (UINT32)impl->SelectedPrinter >= impl->PrinterCount) {
        Buffer[0] = '\0';
        return E_FAIL;
    }

    strncpy(Buffer, impl->Printers[impl->SelectedPrinter].Name, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI PrintDialog_Render(
    ITuiPrintDialog *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiPrintDialogImpl *impl = (TuiPrintDialogImpl *)This;
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

    /* Printer selection */
    Screen->Vtbl->WriteText(Screen, X + 2, currentY, "Printer:",
                            TuiColorBlack, TuiColorWhite);
    currentY++;

    if (impl->PrinterList) {
        impl->PrinterList->Vtbl->Render(impl->PrinterList, Screen,
                                         X + 2, currentY, impl->Width - 4, 6);
    }
    currentY += 7;

    /* Properties button */
    if (impl->PropertiesButton) {
        impl->PropertiesButton->Vtbl->Render(impl->PropertiesButton, Screen,
                                              X + impl->Width - 16, currentY, FALSE);
    }
    currentY += 2;

    /* Print range */
    Screen->Vtbl->WriteText(Screen, X + 2, currentY, "Print range:",
                            TuiColorBlack, TuiColorWhite);
    currentY++;

    if (impl->RangeCombo) {
        impl->RangeCombo->Vtbl->Render(impl->RangeCombo, Screen,
                                        X + 4, currentY, 30);
    }
    currentY += 2;

    /* Page range (if applicable) */
    if (impl->Range == PrintRangePages) {
        Screen->Vtbl->WriteText(Screen, X + 4, currentY, "From:",
                                TuiColorBlack, TuiColorWhite);
        if (impl->PageFromSpinner) {
            impl->PageFromSpinner->Vtbl->Render(impl->PageFromSpinner, Screen,
                                                 X + 10, currentY);
        }

        Screen->Vtbl->WriteText(Screen, X + 26, currentY, "To:",
                                TuiColorBlack, TuiColorWhite);
        if (impl->PageToSpinner) {
            impl->PageToSpinner->Vtbl->Render(impl->PageToSpinner, Screen,
                                               X + 30, currentY);
        }
        currentY++;
    }
    currentY++;

    /* Copies */
    Screen->Vtbl->WriteText(Screen, X + 2, currentY, "Copies:",
                            TuiColorBlack, TuiColorWhite);
    if (impl->CopiesSpinner) {
        impl->CopiesSpinner->Vtbl->Render(impl->CopiesSpinner, Screen,
                                           X + 10, currentY);
    }
    currentY++;

    /* Collate */
    if (impl->CollateCheckbox) {
        impl->CollateCheckbox->Vtbl->Render(impl->CollateCheckbox, Screen,
                                             X + 4, currentY++, FALSE);
    }
    currentY++;

    /* Quality */
    Screen->Vtbl->WriteText(Screen, X + 2, currentY, "Quality:",
                            TuiColorBlack, TuiColorWhite);
    currentY++;
    if (impl->QualityCombo) {
        impl->QualityCombo->Vtbl->Render(impl->QualityCombo, Screen,
                                          X + 4, currentY, 30);
    }
    currentY += 2;

    /* Print to file */
    if (impl->PrintToFileCheckbox) {
        impl->PrintToFileCheckbox->Vtbl->Render(impl->PrintToFileCheckbox, Screen,
                                                 X + 2, currentY++, FALSE);
    }

    /* Buttons */
    if (impl->PrintButton) {
        impl->PrintButton->Vtbl->Render(impl->PrintButton, Screen,
                                         X + impl->Width - 26, Y + impl->Height - 2, FALSE);
    }

    if (impl->CancelButton) {
        impl->CancelButton->Vtbl->Render(impl->CancelButton, Screen,
                                          X + impl->Width - 14, Y + impl->Height - 2, FALSE);
    }

    return S_OK;
}

static HRESULT ANXAPI PrintDialog_HandleKey(
    ITuiPrintDialog *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiPrintDialogImpl *impl = (TuiPrintDialogImpl *)This;

    if (!impl->State.Visible) {
        *Handled = FALSE;
        return S_OK;
    }

    if (Key == TuiKeyEsc) {
        impl->DialogResult = FALSE;
        impl->State.Visible = FALSE;
        *Handled = TRUE;
        return S_OK;
    }

    if (Key == TuiKeyEnter) {
        /* Accept and print */
        impl->DialogResult = TRUE;
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
static CONST ITuiPrintDialog_Vtbl PrintDialogVtbl = {
    PrintDialog_QueryInterface,
    PrintDialog_AddRef,
    PrintDialog_Release,
    PrintDialog_SetPageRange,
    PrintDialog_Show,
    PrintDialog_GetResult,
    PrintDialog_GetSettings,
    PrintDialog_GetPrinterName,
    PrintDialog_Render,
    PrintDialog_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreatePrintDialog(OUT ITuiPrintDialog **PrintDialog)
{
    TuiPrintDialogImpl *impl;
    HRESULT hr;

    if (PrintDialog == NULL) return E_POINTER;

    impl = (TuiPrintDialogImpl *)calloc(1, sizeof(TuiPrintDialogImpl));
    if (impl == NULL) {
        *PrintDialog = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &PrintDialogVtbl;
    InitWidgetState(&impl->State);

    strcpy(impl->Title, "Print");
    impl->PrinterCount = 0;
    impl->SelectedPrinter = 0;
    impl->Range = PrintRangeAll;
    impl->PageFrom = 1;
    impl->PageTo = 1;
    impl->Copies = 1;
    impl->Collate = FALSE;
    impl->Quality = PrintQualityNormal;
    impl->PrintToFile = FALSE;
    impl->OutputFile[0] = '\0';
    impl->DialogResult = FALSE;
    impl->Width = 70;
    impl->Height = 28;

    /* Create widgets */
    hr = AnxTuiCreateListBox(6, &impl->PrinterList);
    if (FAILED(hr)) goto cleanup;

    hr = AnxTuiCreateComboBox(FALSE, &impl->RangeCombo);
    if (FAILED(hr)) goto cleanup;
    impl->RangeCombo->Vtbl->AddItem(impl->RangeCombo, "All pages", NULL);
    impl->RangeCombo->Vtbl->AddItem(impl->RangeCombo, "Selection", NULL);
    impl->RangeCombo->Vtbl->AddItem(impl->RangeCombo, "Pages", NULL);
    impl->RangeCombo->Vtbl->AddItem(impl->RangeCombo, "Current page", NULL);
    impl->RangeCombo->Vtbl->SetSelectedIndex(impl->RangeCombo, 0);

    hr = AnxTuiCreateSpinner("", 1, 9999, &impl->PageFromSpinner);
    if (FAILED(hr)) goto cleanup;

    hr = AnxTuiCreateSpinner("", 1, 9999, &impl->PageToSpinner);
    if (FAILED(hr)) goto cleanup;

    hr = AnxTuiCreateSpinner("", 1, 99, &impl->CopiesSpinner);
    if (FAILED(hr)) goto cleanup;

    hr = AnxTuiCreateCheckbox(&impl->CollateCheckbox);
    if (FAILED(hr)) goto cleanup;
    impl->CollateCheckbox->Vtbl->SetLabel(impl->CollateCheckbox, "Collate");

    hr = AnxTuiCreateCheckbox(&impl->PrintToFileCheckbox);
    if (FAILED(hr)) goto cleanup;
    impl->PrintToFileCheckbox->Vtbl->SetLabel(impl->PrintToFileCheckbox, "Print to file");

    hr = AnxTuiCreateComboBox(FALSE, &impl->QualityCombo);
    if (FAILED(hr)) goto cleanup;
    impl->QualityCombo->Vtbl->AddItem(impl->QualityCombo, "Draft", NULL);
    impl->QualityCombo->Vtbl->AddItem(impl->QualityCombo, "Normal", NULL);
    impl->QualityCombo->Vtbl->AddItem(impl->QualityCombo, "High", NULL);
    impl->QualityCombo->Vtbl->AddItem(impl->QualityCombo, "Best", NULL);
    impl->QualityCombo->Vtbl->SetSelectedIndex(impl->QualityCombo, 1);

    hr = AnxTuiCreateButton(&impl->PrintButton);
    if (FAILED(hr)) goto cleanup;
    impl->PrintButton->Vtbl->SetLabel(impl->PrintButton, " Print ");

    hr = AnxTuiCreateButton(&impl->CancelButton);
    if (FAILED(hr)) goto cleanup;
    impl->CancelButton->Vtbl->SetLabel(impl->CancelButton, " Cancel ");

    hr = AnxTuiCreateButton(&impl->PropertiesButton);
    if (FAILED(hr)) goto cleanup;
    impl->PropertiesButton->Vtbl->SetLabel(impl->PropertiesButton, "Properties");

    *PrintDialog = &impl->Interface;
    return S_OK;

cleanup:
    PrintDialog_Release(&impl->Interface);
    *PrintDialog = NULL;
    return hr;
}
