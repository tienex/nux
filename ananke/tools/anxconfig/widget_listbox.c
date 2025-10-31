/*
 * ListBox Widget Implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "widgets_common.h"

#define MAX_LIST_ITEMS 256
#define MAX_ITEM_LENGTH 256

typedef struct {
    ITuiListBox Interface;
    WIDGET_STATE State;
    CHAR8 Items[MAX_LIST_ITEMS][MAX_ITEM_LENGTH];
    VOID *ItemData[MAX_LIST_ITEMS];  /* Optional user data per item */
    UINT32 ItemCount;
    INT32 SelectedIndex;
    UINT32 ScrollOffset;
    UINT32 VisibleLines;
    BOOLEAN MultiSelect;
    BOOLEAN Selected[MAX_LIST_ITEMS];  /* For multi-select */
    HRESULT (*SelectionCallback)(VOID *UserData, INT32 Index);
    VOID *UserData;

    /* Virtual mode */
    BOOLEAN VirtualMode;
    UINT32 VirtualItemCount;
    HRESULT (*OnGetVirtualItem)(VOID *UserData, UINT32 Index, CHAR8 *OutText, UINTN TextSize);
    VOID *VirtualUserData;
} TuiListBoxImpl;

/* IUnknown methods */
static HRESULT ANXAPI ListBox_QueryInterface(
    ITuiListBox *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI ListBox_AddRef(ITuiListBox *This)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ListBox_Release(ITuiListBox *This)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiListBox methods */
static HRESULT ANXAPI ListBox_AddItem(
    ITuiListBox *This,
    CONST CHAR8 *Item,
    VOID *UserData
)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;

    if (Item == NULL) return E_POINTER;
    if (impl->ItemCount >= MAX_LIST_ITEMS) return E_OUTOFMEMORY;

    strncpy(impl->Items[impl->ItemCount], Item, MAX_ITEM_LENGTH - 1);
    impl->Items[impl->ItemCount][MAX_ITEM_LENGTH - 1] = '\0';
    impl->ItemData[impl->ItemCount] = UserData;
    impl->Selected[impl->ItemCount] = FALSE;
    impl->ItemCount++;

    return S_OK;
}

static HRESULT ANXAPI ListBox_RemoveItem(
    ITuiListBox *This,
    INT32 Index
)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;
    UINT32 i;

    if (Index < 0 || (UINT32)Index >= impl->ItemCount) return E_INVALIDARG;

    /* Shift remaining items */
    for (i = Index; i < impl->ItemCount - 1; i++) {
        strcpy(impl->Items[i], impl->Items[i + 1]);
        impl->ItemData[i] = impl->ItemData[i + 1];
        impl->Selected[i] = impl->Selected[i + 1];
    }

    impl->ItemCount--;

    /* Adjust selection */
    if (impl->SelectedIndex >= (INT32)impl->ItemCount) {
        impl->SelectedIndex = impl->ItemCount - 1;
    }

    return S_OK;
}

static HRESULT ANXAPI ListBox_Clear(ITuiListBox *This)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;
    impl->ItemCount = 0;
    impl->SelectedIndex = -1;
    impl->ScrollOffset = 0;
    return S_OK;
}

static HRESULT ANXAPI ListBox_GetItemCount(
    ITuiListBox *This,
    UINT32 *Count
)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;
    if (Count == NULL) return E_POINTER;
    *Count = impl->ItemCount;
    return S_OK;
}

static HRESULT ANXAPI ListBox_GetSelectedIndex(
    ITuiListBox *This,
    INT32 *Index
)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;
    if (Index == NULL) return E_POINTER;
    *Index = impl->SelectedIndex;
    return S_OK;
}

static HRESULT ANXAPI ListBox_SetSelectedIndex(
    ITuiListBox *This,
    INT32 Index
)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;

    if (Index < -1 || (Index >= 0 && (UINT32)Index >= impl->ItemCount)) {
        return E_INVALIDARG;
    }

    impl->SelectedIndex = Index;

    /* Ensure selected item is visible */
    if (Index >= 0) {
        if ((UINT32)Index < impl->ScrollOffset) {
            impl->ScrollOffset = Index;
        } else if ((UINT32)Index >= impl->ScrollOffset + impl->VisibleLines) {
            impl->ScrollOffset = Index - impl->VisibleLines + 1;
        }
    }

    /* Call selection callback */
    if (impl->SelectionCallback != NULL) {
        impl->SelectionCallback(impl->UserData, Index);
    }

    return S_OK;
}

static HRESULT ANXAPI ListBox_GetItemText(
    ITuiListBox *This,
    INT32 Index,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;

    if (Buffer == NULL) return E_POINTER;
    if (Index < 0 || (UINT32)Index >= impl->ItemCount) return E_INVALIDARG;

    strncpy(Buffer, impl->Items[Index], BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI ListBox_SetSelectionCallback(
    ITuiListBox *This,
    HRESULT (*Callback)(VOID *UserData, INT32 Index),
    VOID *UserData
)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;
    impl->SelectionCallback = Callback;
    impl->UserData = UserData;
    return S_OK;
}

static HRESULT ANXAPI ListBox_SetVisibleLines(
    ITuiListBox *This,
    UINT32 Lines
)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;
    impl->VisibleLines = Lines;
    return S_OK;
}

static HRESULT ANXAPI ListBox_Render(
    ITuiListBox *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width,
    UINT32 Height
)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;
    UINT32 i, line;
    CHAR8 display[512];
    TUI_COLOR fg, bg;

    if (!impl->State.Visible) return S_OK;

    /* Adjust visible lines to height */
    if (Height < impl->VisibleLines) {
        impl->VisibleLines = Height;
    }

    /* Render visible items */
    UINT32 actualItemCount = impl->VirtualMode ? impl->VirtualItemCount : impl->ItemCount;

    for (line = 0; line < impl->VisibleLines && line < actualItemCount; line++) {
        i = impl->ScrollOffset + line;
        if (i >= actualItemCount) break;

        CHAR8 itemText[MAX_ITEM_LENGTH];

        /* Get item text from virtual mode or normal mode */
        if (impl->VirtualMode) {
            if (impl->OnGetVirtualItem) {
                if (FAILED(impl->OnGetVirtualItem(impl->VirtualUserData, i, itemText, sizeof(itemText)))) {
                    strcpy(itemText, "");
                }
            } else {
                strcpy(itemText, "");
            }
        } else {
            strncpy(itemText, impl->Items[i], sizeof(itemText) - 1);
            itemText[sizeof(itemText) - 1] = '\0';
        }

        /* Choose colors */
        if ((INT32)i == impl->SelectedIndex) {
            /* Selected item */
            fg = TuiColorBlack;
            bg = TuiColorCyan;
        } else if (impl->MultiSelect && i < MAX_LIST_ITEMS && impl->Selected[i]) {
            /* Multi-selected item */
            fg = TuiColorBlack;
            bg = TuiColorYellow;
        } else if (!impl->State.Enabled) {
            fg = TuiColorBrightBlack;
            bg = TuiColorBlack;
        } else {
            fg = impl->State.ForegroundColor;
            bg = impl->State.BackgroundColor;
        }

        /* Format item text */
        if (impl->MultiSelect && i < MAX_LIST_ITEMS) {
            snprintf(display, sizeof(display), "[%c] %-*s",
                     impl->Selected[i] ? 'X' : ' ',
                     (int)(Width - 4), itemText);
        } else {
            snprintf(display, sizeof(display), "%-*s",
                     (int)Width, itemText);
        }

        Screen->Vtbl->WriteText(Screen, X, Y + line, display, fg, bg);
    }

    /* Render scrollbar indicator if needed */
    if (impl->ItemCount > impl->VisibleLines) {
        /* Show scroll position indicator on right edge */
        UINT32 scrollbarY = (impl->ScrollOffset * impl->VisibleLines) / impl->ItemCount;
        Screen->Vtbl->WriteText(Screen, X + Width - 1, Y + scrollbarY, "█",
                                TuiColorWhite, TuiColorBlack);
    }

    return S_OK;
}

static HRESULT ANXAPI ListBox_HandleKey(
    ITuiListBox *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiListBoxImpl *impl = (TuiListBoxImpl *)This;

    if (!impl->State.Enabled || impl->ItemCount == 0) {
        *Handled = FALSE;
        return S_OK;
    }

    switch (Key) {
        case TuiKeyUp:
            if (impl->SelectedIndex > 0) {
                ListBox_SetSelectedIndex(This, impl->SelectedIndex - 1);
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyDown:
            if (impl->SelectedIndex < (INT32)(impl->ItemCount - 1)) {
                ListBox_SetSelectedIndex(This, impl->SelectedIndex + 1);
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageUp:
            if (impl->SelectedIndex > 0) {
                INT32 newIndex = impl->SelectedIndex - impl->VisibleLines;
                if (newIndex < 0) newIndex = 0;
                ListBox_SetSelectedIndex(This, newIndex);
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageDown:
            if (impl->SelectedIndex < (INT32)(impl->ItemCount - 1)) {
                INT32 newIndex = impl->SelectedIndex + impl->VisibleLines;
                if (newIndex >= (INT32)impl->ItemCount) {
                    newIndex = impl->ItemCount - 1;
                }
                ListBox_SetSelectedIndex(This, newIndex);
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyHome:
            ListBox_SetSelectedIndex(This, 0);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnd:
            ListBox_SetSelectedIndex(This, impl->ItemCount - 1);
            *Handled = TRUE;
            return S_OK;

        case ' ':
            /* Toggle multi-select if enabled */
            if (impl->MultiSelect && impl->SelectedIndex >= 0) {
                impl->Selected[impl->SelectedIndex] = !impl->Selected[impl->SelectedIndex];
                *Handled = TRUE;
                return S_OK;
            }
            break;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiListBox_Vtbl ListBoxVtbl = {
    ListBox_QueryInterface,
    ListBox_AddRef,
    ListBox_Release,
    ListBox_AddItem,
    ListBox_RemoveItem,
    ListBox_Clear,
    ListBox_GetItemCount,
    ListBox_GetSelectedIndex,
    ListBox_SetSelectedIndex,
    ListBox_GetItemText,
    ListBox_SetSelectionCallback,
    ListBox_SetVisibleLines,
    ListBox_Render,
    ListBox_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateListBox(
    IN  UINT32 VisibleLines,
    OUT ITuiListBox **ListBox
)
{
    TuiListBoxImpl *impl;

    if (ListBox == NULL) return E_POINTER;

    impl = (TuiListBoxImpl *)calloc(1, sizeof(TuiListBoxImpl));
    if (impl == NULL) {
        *ListBox = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &ListBoxVtbl;
    InitWidgetState(&impl->State);

    impl->ItemCount = 0;
    impl->SelectedIndex = -1;
    impl->ScrollOffset = 0;
    impl->VisibleLines = VisibleLines;
    impl->MultiSelect = FALSE;
    impl->SelectionCallback = NULL;
    impl->UserData = NULL;

    *ListBox = &impl->Interface;
    return S_OK;
}
