/*
 * widget_propertysheet.c - Property Sheet Widget
 *
 * Tabbed dialog with multiple property pages, OK/Cancel/Apply buttons,
 * validation, and modified state tracking.
 */

#include <ananke/tui.h>
#include "widgets_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PROPERTY_PAGES 16
#define PROPERTY_SHEET_WIDTH 80
#define PROPERTY_SHEET_HEIGHT 30

/* Property page */
typedef struct {
    CHAR8 Title[64];
    CHAR8 Description[256];
    VOID *Widget;  /* Page content widget */

    /* Callbacks */
    HRESULT (*OnActivate)(VOID *PageWidget, VOID *UserData);
    HRESULT (*OnDeactivate)(VOID *PageWidget, VOID *UserData);
    HRESULT (*OnApply)(VOID *PageWidget, VOID *UserData, BOOLEAN *Applied);
    HRESULT (*OnValidate)(VOID *PageWidget, VOID *UserData, BOOLEAN *IsValid);
    HRESULT (*OnReset)(VOID *PageWidget, VOID *UserData);
    VOID *UserData;

    BOOLEAN Modified;  /* Page has unsaved changes */

} PropertyPage;

typedef struct {
    ITuiPropertySheet Interface;
    WIDGET_STATE State;

    /* Pages */
    PropertyPage Pages[MAX_PROPERTY_PAGES];
    UINT32 PageCount;
    INT32 CurrentPage;

    /* Dialog state */
    BOOLEAN Cancelled;
    BOOLEAN Applied;

    /* Buttons */
    ITuiButton *OKButton;
    ITuiButton *CancelButton;
    ITuiButton *ApplyButton;

    /* Callbacks */
    HRESULT (*OnOK)(VOID *UserData);
    HRESULT (*OnCancel)(VOID *UserData);
    VOID *UserData;

    /* Modified state */
    BOOLEAN AnyPageModified;

} TuiPropertySheetImpl;

/* IUnknown methods */
static HRESULT ANXAPI PropertySheet_QueryInterface(
    ITuiPropertySheet *This,
    REFIID Riid,
    VOID **PpvObject
)
{
    if (IsEqualGUID(Riid, &IID_IUnknown) || IsEqualGUID(Riid, &IID_ITuiPropertySheet)) {
        *PpvObject = This;
        This->Vtbl->AddRef(This);
        return S_OK;
    }
    *PpvObject = NULL;
    return E_NOINTERFACE;
}

static UINTN ANXAPI PropertySheet_AddRef(ITuiPropertySheet *This)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI PropertySheet_Release(ITuiPropertySheet *This)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;
    UINTN count = --impl->State.RefCount;

    if (count == 0) {
        if (impl->OKButton) impl->OKButton->Vtbl->Release(impl->OKButton);
        if (impl->CancelButton) impl->CancelButton->Vtbl->Release(impl->CancelButton);
        if (impl->ApplyButton) impl->ApplyButton->Vtbl->Release(impl->ApplyButton);
        free(impl);
    }

    return count;
}

/* Update modified state */
static VOID UpdateModifiedState(TuiPropertySheetImpl *impl)
{
    impl->AnyPageModified = FALSE;
    for (UINT32 i = 0; i < impl->PageCount; i++) {
        if (impl->Pages[i].Modified) {
            impl->AnyPageModified = TRUE;
            break;
        }
    }

    /* Enable/disable Apply button based on modified state */
    if (impl->ApplyButton) {
        impl->ApplyButton->Vtbl->SetEnabled(impl->ApplyButton, impl->AnyPageModified);
    }
}

/* Render the property sheet */
static HRESULT ANXAPI PropertySheet_Render(
    ITuiPropertySheet *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;

    if (!impl->State.Visible) return S_OK;
    if (impl->PageCount == 0) return S_OK;

    /* Draw shadow */
    for (UINT32 i = 0; i < PROPERTY_SHEET_HEIGHT; i++) {
        for (UINT32 j = 0; j < PROPERTY_SHEET_WIDTH; j++) {
            Screen->Vtbl->WriteChar(Screen, X + j + 2, Y + i + 1, ' ', TuiColorBlack, TuiColorBlack);
        }
    }

    /* Draw dialog background */
    for (UINT32 i = 0; i < PROPERTY_SHEET_HEIGHT; i++) {
        for (UINT32 j = 0; j < PROPERTY_SHEET_WIDTH; j++) {
            Screen->Vtbl->WriteChar(Screen, X + j, Y + i, ' ', TuiColorBlack, TuiColorWhite);
        }
    }

    /* Draw double-line border */
    DrawBoxDouble(Screen, X, Y, PROPERTY_SHEET_WIDTH, PROPERTY_SHEET_HEIGHT,
                 TuiColorBlack, TuiColorWhite);

    /* Draw title bar */
    CHAR8 titleBar[128];
    snprintf(titleBar, sizeof(titleBar), " Properties ");
    for (UINT32 j = 1; j < PROPERTY_SHEET_WIDTH - 1; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, Y + 1, ' ', TuiColorWhite, TuiColorBlue);
    }
    Screen->Vtbl->WriteText(Screen, X + 2, Y + 1, titleBar, TuiColorWhite, TuiColorBlue);

    /* Draw separator after title */
    Screen->Vtbl->WriteChar(Screen, X, Y + 2, gBoxChars.SingleTeeLeft, TuiColorBlack, TuiColorWhite);
    for (UINT32 j = 1; j < PROPERTY_SHEET_WIDTH - 1; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, Y + 2, gBoxChars.SingleHorizontal, TuiColorBlack, TuiColorWhite);
    }
    Screen->Vtbl->WriteChar(Screen, X + PROPERTY_SHEET_WIDTH - 1, Y + 2, gBoxChars.SingleTeeRight, TuiColorBlack, TuiColorWhite);

    /* Draw tab strip */
    UINT32 tabY = Y + 3;
    UINT32 tabX = X + 2;

    for (UINT32 i = 0; i < impl->PageCount; i++) {
        PropertyPage *page = &impl->Pages[i];
        CHAR8 tabText[68];

        /* Add asterisk if modified */
        if (page->Modified) {
            snprintf(tabText, sizeof(tabText), " %s * ", page->Title);
        } else {
            snprintf(tabText, sizeof(tabText), " %s ", page->Title);
        }

        UINTN tabWidth = strlen(tabText);
        TUI_COLOR fg = (i == impl->CurrentPage) ? TuiColorBlack : TuiColorBrightBlack;
        TUI_COLOR bg = (i == impl->CurrentPage) ? TuiColorCyan : TuiColorWhite;

        /* Draw tab */
        for (UINT32 j = 0; j < tabWidth; j++) {
            Screen->Vtbl->WriteChar(Screen, tabX + j, tabY, tabText[j], fg, bg);
        }

        tabX += tabWidth + 1;
    }

    /* Draw separator after tabs */
    Screen->Vtbl->WriteChar(Screen, X, tabY + 1, gBoxChars.SingleTeeLeft, TuiColorBlack, TuiColorWhite);
    for (UINT32 j = 1; j < PROPERTY_SHEET_WIDTH - 1; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, tabY + 1, gBoxChars.SingleHorizontal, TuiColorBlack, TuiColorWhite);
    }
    Screen->Vtbl->WriteChar(Screen, X + PROPERTY_SHEET_WIDTH - 1, tabY + 1, gBoxChars.SingleTeeRight, TuiColorBlack, TuiColorWhite);

    /* Draw current page description */
    if (impl->CurrentPage >= 0 && impl->CurrentPage < (INT32)impl->PageCount) {
        PropertyPage *currentPage = &impl->Pages[impl->CurrentPage];

        if (currentPage->Description[0]) {
            Screen->Vtbl->WriteText(Screen, X + 3, tabY + 3,
                                   currentPage->Description,
                                   TuiColorBrightBlack, TuiColorWhite);
        }

        /* Render page widget */
        if (currentPage->Widget) {
            TUI_RECT pageRect = {
                X + 3,
                tabY + 5,
                PROPERTY_SHEET_WIDTH - 6,
                PROPERTY_SHEET_HEIGHT - 12
            };

            ITuiButton *widget = (ITuiButton *)currentPage->Widget;
            if (widget->Vtbl->SetBounds) {
                widget->Vtbl->SetBounds(widget, &pageRect);
            }
            if (widget->Vtbl->Render) {
                widget->Vtbl->Render(widget, Screen, pageRect.X, pageRect.Y, FALSE);
            }
        }
    }

    /* Draw separator before buttons */
    UINT32 buttonSepY = Y + PROPERTY_SHEET_HEIGHT - 4;
    Screen->Vtbl->WriteChar(Screen, X, buttonSepY, gBoxChars.SingleTeeLeft, TuiColorBlack, TuiColorWhite);
    for (UINT32 j = 1; j < PROPERTY_SHEET_WIDTH - 1; j++) {
        Screen->Vtbl->WriteChar(Screen, X + j, buttonSepY, gBoxChars.SingleHorizontal, TuiColorBlack, TuiColorWhite);
    }
    Screen->Vtbl->WriteChar(Screen, X + PROPERTY_SHEET_WIDTH - 1, buttonSepY, gBoxChars.SingleTeeRight, TuiColorBlack, TuiColorWhite);

    /* Draw buttons */
    INT32 buttonY = Y + PROPERTY_SHEET_HEIGHT - 3;

    if (impl->OKButton) {
        impl->OKButton->Vtbl->Render(impl->OKButton, Screen, X + 3, buttonY, FALSE);
    }

    if (impl->CancelButton) {
        impl->CancelButton->Vtbl->Render(impl->CancelButton, Screen, X + 15, buttonY, FALSE);
    }

    if (impl->ApplyButton) {
        impl->ApplyButton->Vtbl->Render(impl->ApplyButton, Screen, X + 30, buttonY, FALSE);
    }

    return S_OK;
}

/* Handle keyboard input */
static HRESULT ANXAPI PropertySheet_HandleKey(
    ITuiPropertySheet *This,
    TUI_KEY Key
)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;

    if (Key == TuiKeyEscape) {
        impl->Cancelled = TRUE;
        if (impl->OnCancel) {
            impl->OnCancel(impl->UserData);
        }
        impl->State.Visible = FALSE;
        return S_OK;
    }

    /* Tab navigation */
    if (Key == TuiKeyTab || Key == TuiKeyRight) {
        if (impl->CurrentPage < (INT32)impl->PageCount - 1) {
            This->Vtbl->SetActivePage(This, impl->CurrentPage + 1);
        }
        return S_OK;
    }

    if (Key == TuiKeyLeft) {
        if (impl->CurrentPage > 0) {
            This->Vtbl->SetActivePage(This, impl->CurrentPage - 1);
        }
        return S_OK;
    }

    /* Button shortcuts */
    if (Key == 'o' || Key == 'O' || Key == TuiKeyEnter) {
        This->Vtbl->OK(This);
        return S_OK;
    }

    if (Key == 'c' || Key == 'C') {
        This->Vtbl->Cancel(This);
        return S_OK;
    }

    if (Key == 'a' || Key == 'A') {
        if (impl->AnyPageModified) {
            This->Vtbl->Apply(This);
        }
        return S_OK;
    }

    return S_OK;
}

/* Add a property page */
static HRESULT ANXAPI PropertySheet_AddPage(
    ITuiPropertySheet *This,
    CONST CHAR8 *Title,
    CONST CHAR8 *Description,
    VOID *PageWidget,
    HRESULT (*OnActivate)(VOID *PageWidget, VOID *UserData),
    HRESULT (*OnDeactivate)(VOID *PageWidget, VOID *UserData),
    HRESULT (*OnApply)(VOID *PageWidget, VOID *UserData, BOOLEAN *Applied),
    HRESULT (*OnValidate)(VOID *PageWidget, VOID *UserData, BOOLEAN *IsValid),
    HRESULT (*OnReset)(VOID *PageWidget, VOID *UserData),
    VOID *UserData
)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;

    if (impl->PageCount >= MAX_PROPERTY_PAGES) {
        return E_OUTOFMEMORY;
    }

    PropertyPage *page = &impl->Pages[impl->PageCount++];

    strncpy(page->Title, Title ? Title : "", sizeof(page->Title) - 1);
    page->Title[sizeof(page->Title) - 1] = '\0';

    strncpy(page->Description, Description ? Description : "", sizeof(page->Description) - 1);
    page->Description[sizeof(page->Description) - 1] = '\0';

    page->Widget = PageWidget;
    page->OnActivate = OnActivate;
    page->OnDeactivate = OnDeactivate;
    page->OnApply = OnApply;
    page->OnValidate = OnValidate;
    page->OnReset = OnReset;
    page->UserData = UserData;
    page->Modified = FALSE;

    return S_OK;
}

/* Set active page */
static HRESULT ANXAPI PropertySheet_SetActivePage(
    ITuiPropertySheet *This,
    INT32 PageIndex
)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;

    if (PageIndex < 0 || PageIndex >= (INT32)impl->PageCount) {
        return E_INVALIDARG;
    }

    /* Deactivate current page */
    if (impl->CurrentPage >= 0 && impl->CurrentPage < (INT32)impl->PageCount) {
        PropertyPage *currentPage = &impl->Pages[impl->CurrentPage];
        if (currentPage->OnDeactivate) {
            currentPage->OnDeactivate(currentPage->Widget, currentPage->UserData);
        }
    }

    /* Activate new page */
    impl->CurrentPage = PageIndex;
    PropertyPage *newPage = &impl->Pages[PageIndex];
    if (newPage->OnActivate) {
        newPage->OnActivate(newPage->Widget, newPage->UserData);
    }

    return S_OK;
}

/* Mark page as modified */
static HRESULT ANXAPI PropertySheet_SetPageModified(
    ITuiPropertySheet *This,
    INT32 PageIndex,
    BOOLEAN Modified
)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;

    if (PageIndex < 0 || PageIndex >= (INT32)impl->PageCount) {
        return E_INVALIDARG;
    }

    impl->Pages[PageIndex].Modified = Modified;
    UpdateModifiedState(impl);

    return S_OK;
}

/* Apply changes */
static HRESULT ANXAPI PropertySheet_Apply(ITuiPropertySheet *This)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;

    /* Apply all modified pages */
    for (UINT32 i = 0; i < impl->PageCount; i++) {
        PropertyPage *page = &impl->Pages[i];

        if (page->Modified && page->OnApply) {
            BOOLEAN applied = FALSE;
            page->OnApply(page->Widget, page->UserData, &applied);

            if (applied) {
                page->Modified = FALSE;
            }
        }
    }

    impl->Applied = TRUE;
    UpdateModifiedState(impl);

    return S_OK;
}

/* OK button - apply and close */
static HRESULT ANXAPI PropertySheet_OK(ITuiPropertySheet *This)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;

    /* Apply changes */
    This->Vtbl->Apply(This);

    /* Call OK callback */
    if (impl->OnOK) {
        impl->OnOK(impl->UserData);
    }

    impl->State.Visible = FALSE;
    return S_OK;
}

/* Cancel button - discard changes and close */
static HRESULT ANXAPI PropertySheet_Cancel(ITuiPropertySheet *This)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;

    impl->Cancelled = TRUE;

    /* Call cancel callback */
    if (impl->OnCancel) {
        impl->OnCancel(impl->UserData);
    }

    impl->State.Visible = FALSE;
    return S_OK;
}

/* Reset all pages */
static HRESULT ANXAPI PropertySheet_Reset(ITuiPropertySheet *This)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;

    for (UINT32 i = 0; i < impl->PageCount; i++) {
        PropertyPage *page = &impl->Pages[i];

        if (page->OnReset) {
            page->OnReset(page->Widget, page->UserData);
        }

        page->Modified = FALSE;
    }

    UpdateModifiedState(impl);
    return S_OK;
}

/* Widget common methods */
static HRESULT ANXAPI PropertySheet_SetBounds(ITuiPropertySheet *This, CONST TUI_RECT *Bounds)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;
    impl->State.Bounds = *Bounds;
    return S_OK;
}

static HRESULT ANXAPI PropertySheet_GetBounds(ITuiPropertySheet *This, TUI_RECT *Bounds)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI PropertySheet_SetVisible(ITuiPropertySheet *This, BOOLEAN Visible)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;
    impl->State.Visible = Visible;
    return S_OK;
}

static BOOLEAN ANXAPI PropertySheet_IsVisible(ITuiPropertySheet *This)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;
    return impl->State.Visible;
}

static HRESULT ANXAPI PropertySheet_SetEnabled(ITuiPropertySheet *This, BOOLEAN Enabled)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;
    impl->State.Enabled = Enabled;
    return S_OK;
}

static BOOLEAN ANXAPI PropertySheet_IsEnabled(ITuiPropertySheet *This)
{
    TuiPropertySheetImpl *impl = (TuiPropertySheetImpl *)This;
    return impl->State.Enabled;
}

/* VTable */
static ITuiPropertySheetVtbl PropertySheetVtbl = {
    PropertySheet_QueryInterface,
    PropertySheet_AddRef,
    PropertySheet_Release,
    PropertySheet_Render,
    PropertySheet_HandleKey,
    PropertySheet_SetBounds,
    PropertySheet_GetBounds,
    PropertySheet_SetVisible,
    PropertySheet_IsVisible,
    PropertySheet_SetEnabled,
    PropertySheet_IsEnabled,
    PropertySheet_AddPage,
    PropertySheet_SetActivePage,
    PropertySheet_SetPageModified,
    PropertySheet_Apply,
    PropertySheet_OK,
    PropertySheet_Cancel,
    PropertySheet_Reset
};

/* Factory function */
HRESULT AnxTuiCreatePropertySheet(ITuiPropertySheet **OutPropertySheet)
{
    TuiPropertySheetImpl *impl;
    HRESULT hr;

    if (!OutPropertySheet) return E_INVALIDARG;

    impl = (TuiPropertySheetImpl *)malloc(sizeof(TuiPropertySheetImpl));
    if (!impl) return E_OUTOFMEMORY;

    memset(impl, 0, sizeof(TuiPropertySheetImpl));
    impl->Interface.Vtbl = &PropertySheetVtbl;
    InitWidgetState(&impl->State);

    impl->CurrentPage = 0;
    impl->Cancelled = FALSE;
    impl->Applied = FALSE;
    impl->AnyPageModified = FALSE;

    /* Create buttons */
    hr = AnxTuiCreateButton("OK", NULL, NULL, &impl->OKButton);
    if (FAILED(hr)) {
        free(impl);
        return hr;
    }

    hr = AnxTuiCreateButton("Cancel", NULL, NULL, &impl->CancelButton);
    if (FAILED(hr)) {
        impl->OKButton->Vtbl->Release(impl->OKButton);
        free(impl);
        return hr;
    }

    hr = AnxTuiCreateButton("Apply", NULL, NULL, &impl->ApplyButton);
    if (FAILED(hr)) {
        impl->OKButton->Vtbl->Release(impl->OKButton);
        impl->CancelButton->Vtbl->Release(impl->CancelButton);
        free(impl);
        return hr;
    }

    /* Apply button starts disabled */
    impl->ApplyButton->Vtbl->SetEnabled(impl->ApplyButton, FALSE);

    impl->State.Bounds.Width = PROPERTY_SHEET_WIDTH;
    impl->State.Bounds.Height = PROPERTY_SHEET_HEIGHT;

    *OutPropertySheet = &impl->Interface;
    return S_OK;
}
