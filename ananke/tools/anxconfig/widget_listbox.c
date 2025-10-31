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
    ITuiWidget WidgetInterface;
    ITuiListBox ListBoxInterface;
    ITuiKeyListener KeyListener;
    ITuiDrawListener DrawListener;

    WIDGET_STATE State;
    CHAR8 Items[MAX_LIST_ITEMS][MAX_ITEM_LENGTH];
    VOID *ItemData[MAX_LIST_ITEMS];
    UINT32 ItemCount;
    INT32 SelectedIndex;
    UINT32 ScrollOffset;
    UINT32 VisibleLines;
    BOOLEAN MultiSelect;
    BOOLEAN Selected[MAX_LIST_ITEMS];
    HRESULT (*SelectionCallback)(VOID *UserData, INT32 Index);
    VOID *UserData;

    /* Virtual mode */
    BOOLEAN VirtualMode;
    UINT32 VirtualItemCount;
    HRESULT (*OnGetVirtualItem)(VOID *UserData, UINT32 Index, CHAR8 *OutText, UINTN TextSize);
    VOID *VirtualUserData;

    ITuiResponder *NextResponder;
    ITuiSurface *Surface;
} TuiListBoxImpl;

/* Helper macros for interface conversions */
#define LISTBOX_FROM_WIDGET(w) ((TuiListBoxImpl*)((UINT8*)(w) - offsetof(TuiListBoxImpl, WidgetInterface)))
#define LISTBOX_FROM_LISTBOX(l) ((TuiListBoxImpl*)((UINT8*)(l) - offsetof(TuiListBoxImpl, ListBoxInterface)))
#define LISTBOX_FROM_KEY(k) ((TuiListBoxImpl*)((UINT8*)(k) - offsetof(TuiListBoxImpl, KeyListener)))
#define LISTBOX_FROM_DRAW(d) ((TuiListBoxImpl*)((UINT8*)(d) - offsetof(TuiListBoxImpl, DrawListener)))

/*=============================================================================
 * ITuiWidget Implementation
 *===========================================================================*/

static HRESULT ANXAPI ListBoxWidget_QueryInterface(ITuiWidget *This, REFIID riid, VOID **ppvObject)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (!ppvObject) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ITuiWidget)) {
        *ppvObject = &impl->WidgetInterface;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiKeyListener)) {
        *ppvObject = &impl->KeyListener;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiDrawListener)) {
        *ppvObject = &impl->DrawListener;
        impl->State.RefCount++;
        return S_OK;
    }
    if (IsEqualGUID(riid, &IID_ITuiListBox)) {
        *ppvObject = &impl->ListBoxInterface;
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

static UINTN ANXAPI ListBoxWidget_AddRef(ITuiWidget *This)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ListBoxWidget_Release(ITuiWidget *This)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
        free(impl);
    }
    return refCount;
}

static HRESULT ANXAPI ListBoxWidget_GetBounds(ITuiWidget *This, TUI_RECT *Bounds)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (!Bounds) return E_POINTER;
    *Bounds = impl->State.Bounds;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_SetBounds(ITuiWidget *This, CONST TUI_RECT *Bounds)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (!Bounds) return E_POINTER;
    impl->State.Bounds = *Bounds;
    impl->VisibleLines = Bounds->Bottom - Bounds->Top;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_GetPreferredSize(ITuiWidget *This, UINT32 *Width, UINT32 *Height)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (!Width || !Height) return E_POINTER;
    *Width = 40;
    *Height = impl->VisibleLines;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_SetVisible(ITuiWidget *This, BOOLEAN Visible)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    impl->State.Visible = Visible;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_IsVisible(ITuiWidget *This, BOOLEAN *Visible)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (!Visible) return E_POINTER;
    *Visible = impl->State.Visible;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_SetEnabled(ITuiWidget *This, BOOLEAN Enabled)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    impl->State.Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_IsEnabled(ITuiWidget *This, BOOLEAN *Enabled)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (!Enabled) return E_POINTER;
    *Enabled = impl->State.Enabled;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_SetFocused(ITuiWidget *This, BOOLEAN Focused)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    impl->State.Focused = Focused;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_IsFocused(ITuiWidget *This, BOOLEAN *Focused)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (!Focused) return E_POINTER;
    *Focused = impl->State.Focused;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_SetColors(ITuiWidget *This, TUI_COLOR Foreground, TUI_COLOR Background)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    impl->State.ForegroundColor = Foreground;
    impl->State.BackgroundColor = Background;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_GetColors(ITuiWidget *This, TUI_COLOR *Foreground, TUI_COLOR *Background)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (Foreground) *Foreground = impl->State.ForegroundColor;
    if (Background) *Background = impl->State.BackgroundColor;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_Invalidate(ITuiWidget *This, CONST TUI_RECT *Region)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (impl->Surface) {
        return impl->Surface->Vtbl->Invalidate(impl->Surface, Region);
    }
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_SetSurface(ITuiWidget *This, ITuiSurface *Surface)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (impl->Surface) impl->Surface->Vtbl->Release(impl->Surface);
    impl->Surface = Surface;
    if (Surface) Surface->Vtbl->AddRef(Surface);
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_GetSurface(ITuiWidget *This, ITuiSurface **Surface)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (!Surface) return E_POINTER;
    *Surface = impl->Surface;
    if (impl->Surface) impl->Surface->Vtbl->AddRef(impl->Surface);
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_GetNextResponder(ITuiWidget *This, ITuiResponder **NextResponder)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    if (!NextResponder) return E_POINTER;
    *NextResponder = impl->NextResponder;
    return S_OK;
}

static HRESULT ANXAPI ListBoxWidget_SetNextResponder(ITuiWidget *This, ITuiResponder *NextResponder)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_WIDGET(This);
    impl->NextResponder = NextResponder;
    return S_OK;
}

static CONST ITuiWidget_Vtbl ListBoxWidgetVtbl = {
    ListBoxWidget_QueryInterface,
    ListBoxWidget_AddRef,
    ListBoxWidget_Release,
    ListBoxWidget_GetBounds,
    ListBoxWidget_SetBounds,
    ListBoxWidget_GetPreferredSize,
    ListBoxWidget_SetVisible,
    ListBoxWidget_IsVisible,
    ListBoxWidget_SetEnabled,
    ListBoxWidget_IsEnabled,
    ListBoxWidget_SetFocused,
    ListBoxWidget_IsFocused,
    ListBoxWidget_SetColors,
    ListBoxWidget_GetColors,
    ListBoxWidget_Invalidate,
    ListBoxWidget_SetSurface,
    ListBoxWidget_GetSurface,
    ListBoxWidget_GetNextResponder,
    ListBoxWidget_SetNextResponder
};

/*=============================================================================
 * ITuiKeyListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI ListBoxKey_QueryInterface(ITuiKeyListener *This, REFIID riid, VOID **ppvObject)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_KEY(This);
    return ListBoxWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI ListBoxKey_AddRef(ITuiKeyListener *This)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_KEY(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ListBoxKey_Release(ITuiKeyListener *This)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_KEY(This);
    return ListBoxWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI ListBoxKey_OnKeyDown(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_KEY(This);
    UINT32 actualItemCount = impl->VirtualMode ? impl->VirtualItemCount : impl->ItemCount;

    *Handled = FALSE;

    if (!impl->State.Enabled || actualItemCount == 0) return S_OK;

    switch (Key) {
        case TuiKeyUp:
            if (impl->SelectedIndex > 0) {
                impl->SelectedIndex--;
                /* Ensure selected item is visible */
                if ((UINT32)impl->SelectedIndex < impl->ScrollOffset) {
                    impl->ScrollOffset = impl->SelectedIndex;
                }
                /* Call selection callback */
                if (impl->SelectionCallback) {
                    impl->SelectionCallback(impl->UserData, impl->SelectedIndex);
                }
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyDown:
            if (impl->SelectedIndex < (INT32)(actualItemCount - 1)) {
                impl->SelectedIndex++;
                /* Ensure selected item is visible */
                if ((UINT32)impl->SelectedIndex >= impl->ScrollOffset + impl->VisibleLines) {
                    impl->ScrollOffset = impl->SelectedIndex - impl->VisibleLines + 1;
                }
                /* Call selection callback */
                if (impl->SelectionCallback) {
                    impl->SelectionCallback(impl->UserData, impl->SelectedIndex);
                }
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageUp:
            if (impl->SelectedIndex > 0) {
                INT32 newIndex = impl->SelectedIndex - impl->VisibleLines;
                if (newIndex < 0) newIndex = 0;
                impl->SelectedIndex = newIndex;
                impl->ScrollOffset = newIndex;
                /* Call selection callback */
                if (impl->SelectionCallback) {
                    impl->SelectionCallback(impl->UserData, impl->SelectedIndex);
                }
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyPageDown:
            if (impl->SelectedIndex < (INT32)(actualItemCount - 1)) {
                INT32 newIndex = impl->SelectedIndex + impl->VisibleLines;
                if (newIndex >= (INT32)actualItemCount) {
                    newIndex = actualItemCount - 1;
                }
                impl->SelectedIndex = newIndex;
                if ((UINT32)impl->SelectedIndex >= impl->ScrollOffset + impl->VisibleLines) {
                    impl->ScrollOffset = impl->SelectedIndex - impl->VisibleLines + 1;
                }
                /* Call selection callback */
                if (impl->SelectionCallback) {
                    impl->SelectionCallback(impl->UserData, impl->SelectedIndex);
                }
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyHome:
            impl->SelectedIndex = 0;
            impl->ScrollOffset = 0;
            /* Call selection callback */
            if (impl->SelectionCallback) {
                impl->SelectionCallback(impl->UserData, impl->SelectedIndex);
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnd:
            impl->SelectedIndex = actualItemCount - 1;
            if ((UINT32)impl->SelectedIndex >= impl->ScrollOffset + impl->VisibleLines) {
                impl->ScrollOffset = impl->SelectedIndex - impl->VisibleLines + 1;
            }
            /* Call selection callback */
            if (impl->SelectionCallback) {
                impl->SelectionCallback(impl->UserData, impl->SelectedIndex);
            }
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

    return S_OK;
}

static HRESULT ANXAPI ListBoxKey_OnKeyUp(ITuiKeyListener *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled)
{
    *Handled = FALSE;
    return S_OK;
}

static CONST ITuiKeyListener_Vtbl ListBoxKeyVtbl = {
    ListBoxKey_QueryInterface,
    ListBoxKey_AddRef,
    ListBoxKey_Release,
    ListBoxKey_OnKeyDown,
    ListBoxKey_OnKeyUp
};

/*=============================================================================
 * ITuiDrawListener Implementation
 *===========================================================================*/

static HRESULT ANXAPI ListBoxDraw_QueryInterface(ITuiDrawListener *This, REFIID riid, VOID **ppvObject)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_DRAW(This);
    return ListBoxWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI ListBoxDraw_AddRef(ITuiDrawListener *This)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_DRAW(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ListBoxDraw_Release(ITuiDrawListener *This)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_DRAW(This);
    return ListBoxWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI ListBoxDraw_OnDraw(ITuiDrawListener *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_DRAW(This);
    UINT32 i, line;
    CHAR8 display[512];
    TUI_COLOR fg, bg;
    UINT32 width, height;

    if (!impl->State.Visible) return S_OK;

    width = impl->State.Bounds.Right - impl->State.Bounds.Left;
    height = impl->State.Bounds.Bottom - impl->State.Bounds.Top;

    /* Adjust visible lines to height */
    if (height < impl->VisibleLines) {
        impl->VisibleLines = height;
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
                     (int)(width - 4), itemText);
        } else {
            snprintf(display, sizeof(display), "%-*s",
                     (int)width, itemText);
        }

        Surface->Vtbl->WriteText(Surface, 0, line, display, fg, bg);
    }

    /* Render scrollbar indicator if needed */
    if (actualItemCount > impl->VisibleLines) {
        /* Show scroll position indicator on right edge */
        UINT32 scrollbarY = (impl->ScrollOffset * impl->VisibleLines) / actualItemCount;
        if (scrollbarY < impl->VisibleLines) {
            Surface->Vtbl->WriteText(Surface, width - 1, scrollbarY, "█",
                                    TuiColorWhite, TuiColorBlack);
        }
    }

    return S_OK;
}

static CONST ITuiDrawListener_Vtbl ListBoxDrawVtbl = {
    ListBoxDraw_QueryInterface,
    ListBoxDraw_AddRef,
    ListBoxDraw_Release,
    ListBoxDraw_OnDraw
};

/*=============================================================================
 * ITuiListBox Implementation (Backward Compatibility)
 *===========================================================================*/

static HRESULT ANXAPI ListBox_QueryInterface(ITuiListBox *This, REFIID riid, VOID **ppvObject)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);
    return ListBoxWidget_QueryInterface(&impl->WidgetInterface, riid, ppvObject);
}

static UINTN ANXAPI ListBox_AddRef(ITuiListBox *This)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ListBox_Release(ITuiListBox *This)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);
    return ListBoxWidget_Release(&impl->WidgetInterface);
}

static HRESULT ANXAPI ListBox_AddItem(ITuiListBox *This, CONST CHAR8 *Item, VOID *UserData)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);

    if (Item == NULL) return E_POINTER;
    if (impl->ItemCount >= MAX_LIST_ITEMS) return E_OUTOFMEMORY;

    strncpy(impl->Items[impl->ItemCount], Item, MAX_ITEM_LENGTH - 1);
    impl->Items[impl->ItemCount][MAX_ITEM_LENGTH - 1] = '\0';
    impl->ItemData[impl->ItemCount] = UserData;
    impl->Selected[impl->ItemCount] = FALSE;
    impl->ItemCount++;

    return S_OK;
}

static HRESULT ANXAPI ListBox_RemoveItem(ITuiListBox *This, INT32 Index)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);
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
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);
    impl->ItemCount = 0;
    impl->SelectedIndex = -1;
    impl->ScrollOffset = 0;
    return S_OK;
}

static HRESULT ANXAPI ListBox_GetItemCount(ITuiListBox *This, UINT32 *Count)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);
    if (Count == NULL) return E_POINTER;
    *Count = impl->ItemCount;
    return S_OK;
}

static HRESULT ANXAPI ListBox_GetSelectedIndex(ITuiListBox *This, INT32 *Index)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);
    if (Index == NULL) return E_POINTER;
    *Index = impl->SelectedIndex;
    return S_OK;
}

static HRESULT ANXAPI ListBox_SetSelectedIndex(ITuiListBox *This, INT32 Index)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);
    UINT32 actualItemCount = impl->VirtualMode ? impl->VirtualItemCount : impl->ItemCount;

    if (Index < -1 || (Index >= 0 && (UINT32)Index >= actualItemCount)) {
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

static HRESULT ANXAPI ListBox_GetItemText(ITuiListBox *This, INT32 Index, CHAR8 *Buffer, UINTN BufferSize)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);

    if (Buffer == NULL) return E_POINTER;
    if (Index < 0 || (UINT32)Index >= impl->ItemCount) return E_INVALIDARG;

    strncpy(Buffer, impl->Items[Index], BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI ListBox_SetSelectionCallback(ITuiListBox *This, HRESULT (*Callback)(VOID *UserData, INT32 Index), VOID *UserData)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);
    impl->SelectionCallback = Callback;
    impl->UserData = UserData;
    return S_OK;
}

static HRESULT ANXAPI ListBox_SetVisibleLines(ITuiListBox *This, UINT32 Lines)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);
    impl->VisibleLines = Lines;
    return S_OK;
}

static HRESULT ANXAPI ListBox_Render(ITuiListBox *This, ITuiScreen *Screen, INT32 X, INT32 Y, UINT32 Width, UINT32 Height)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);

    /* Legacy render method - delegate to OnDraw */
    if (impl->Surface) {
        TUI_RECT rect = { X, Y, X + Width, Y + Height };
        return ListBoxDraw_OnDraw(&impl->DrawListener, impl->Surface, &rect);
    }

    return S_OK;
}

static HRESULT ANXAPI ListBox_HandleKey(ITuiListBox *This, TUI_KEY Key, BOOLEAN *Handled)
{
    TuiListBoxImpl *impl = LISTBOX_FROM_LISTBOX(This);
    return ListBoxKey_OnKeyDown(&impl->KeyListener, Key, 0, Handled);
}

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

/*=============================================================================
 * Factory Function
 *===========================================================================*/

HRESULT ANXAPI AnxTuiCreateListBox(IN UINT32 VisibleLines, OUT ITuiListBox **ListBox)
{
    TuiListBoxImpl *impl;

    if (ListBox == NULL) return E_POINTER;

    impl = (TuiListBoxImpl *)calloc(1, sizeof(TuiListBoxImpl));
    if (impl == NULL) {
        *ListBox = NULL;
        return E_OUTOFMEMORY;
    }

    /* Initialize vtables */
    impl->WidgetInterface.Vtbl = &ListBoxWidgetVtbl;
    impl->ListBoxInterface.Vtbl = &ListBoxVtbl;
    impl->KeyListener.Vtbl = &ListBoxKeyVtbl;
    impl->DrawListener.Vtbl = &ListBoxDrawVtbl;

    /* Initialize widget state */
    InitWidgetState(&impl->State);

    /* Initialize listbox-specific state */
    impl->ItemCount = 0;
    impl->SelectedIndex = -1;
    impl->ScrollOffset = 0;
    impl->VisibleLines = VisibleLines;
    impl->MultiSelect = FALSE;
    impl->SelectionCallback = NULL;
    impl->UserData = NULL;
    impl->VirtualMode = FALSE;
    impl->VirtualItemCount = 0;
    impl->OnGetVirtualItem = NULL;
    impl->VirtualUserData = NULL;
    impl->NextResponder = NULL;
    impl->Surface = NULL;

    *ListBox = &impl->ListBoxInterface;
    return S_OK;
}
