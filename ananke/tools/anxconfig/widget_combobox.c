/*
 * ComboBox Widget Implementation
 *
 * Dropdown list with optional text editing.
 * Supports editable and non-editable modes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "widgets_common.h"

#define MAX_COMBO_ITEMS 256
#define MAX_ITEM_LENGTH 256

typedef struct {
    ITuiComboBox Interface;
    WIDGET_STATE State;
    CHAR8 Items[MAX_COMBO_ITEMS][MAX_ITEM_LENGTH];
    VOID *ItemData[MAX_COMBO_ITEMS];
    UINT32 ItemCount;
    INT32 SelectedIndex;
    CHAR8 EditText[MAX_ITEM_LENGTH];
    BOOLEAN Editable;
    BOOLEAN Dropped;          /* Drop-down list is open */
    UINT32 DropDownHeight;
    UINT32 ScrollOffset;
    UINT32 CursorPos;         /* For editable mode */
    HRESULT (*ChangeCallback)(VOID *UserData, INT32 Index);
    VOID *UserData;

    /* Virtual mode */
    BOOLEAN VirtualMode;
    UINT32 VirtualItemCount;
    HRESULT (*OnGetVirtualItem)(VOID *UserData, UINT32 Index, CHAR8 *OutText, UINTN TextSize);
    VOID *VirtualUserData;
} TuiComboBoxImpl;

/* IUnknown methods */
static HRESULT ANXAPI ComboBox_QueryInterface(
    ITuiComboBox *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI ComboBox_AddRef(ITuiComboBox *This)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI ComboBox_Release(ITuiComboBox *This)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        free(impl);
    }
    return refCount;
}

/* ITuiComboBox methods */
static HRESULT ANXAPI ComboBox_SetEditable(
    ITuiComboBox *This,
    BOOLEAN Editable
)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    impl->Editable = Editable;
    return S_OK;
}

static HRESULT ANXAPI ComboBox_AddItem(
    ITuiComboBox *This,
    CONST CHAR8 *Item,
    VOID *UserData
)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;

    if (Item == NULL) return E_POINTER;
    if (impl->ItemCount >= MAX_COMBO_ITEMS) return E_OUTOFMEMORY;

    strncpy(impl->Items[impl->ItemCount], Item, MAX_ITEM_LENGTH - 1);
    impl->Items[impl->ItemCount][MAX_ITEM_LENGTH - 1] = '\0';
    impl->ItemData[impl->ItemCount] = UserData;
    impl->ItemCount++;

    return S_OK;
}

static HRESULT ANXAPI ComboBox_RemoveItem(
    ITuiComboBox *This,
    INT32 Index
)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    UINT32 i;

    if (Index < 0 || (UINT32)Index >= impl->ItemCount) return E_INVALIDARG;

    /* Shift remaining items */
    for (i = Index; i < impl->ItemCount - 1; i++) {
        strcpy(impl->Items[i], impl->Items[i + 1]);
        impl->ItemData[i] = impl->ItemData[i + 1];
    }

    impl->ItemCount--;

    /* Adjust selection */
    if (impl->SelectedIndex >= (INT32)impl->ItemCount) {
        impl->SelectedIndex = impl->ItemCount - 1;
    }

    return S_OK;
}

static HRESULT ANXAPI ComboBox_Clear(ITuiComboBox *This)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    impl->ItemCount = 0;
    impl->SelectedIndex = -1;
    impl->EditText[0] = '\0';
    return S_OK;
}

static HRESULT ANXAPI ComboBox_GetSelectedIndex(
    ITuiComboBox *This,
    INT32 *Index
)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    if (Index == NULL) return E_POINTER;
    *Index = impl->SelectedIndex;
    return S_OK;
}

static HRESULT ANXAPI ComboBox_SetSelectedIndex(
    ITuiComboBox *This,
    INT32 Index
)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;

    if (Index < -1 || (Index >= 0 && (UINT32)Index >= impl->ItemCount)) {
        return E_INVALIDARG;
    }

    impl->SelectedIndex = Index;

    /* Update edit text */
    if (Index >= 0) {
        strcpy(impl->EditText, impl->Items[Index]);
        impl->CursorPos = strlen(impl->EditText);
    }

    /* Call change callback */
    if (impl->ChangeCallback != NULL) {
        impl->ChangeCallback(impl->UserData, Index);
    }

    return S_OK;
}

static HRESULT ANXAPI ComboBox_GetText(
    ITuiComboBox *This,
    CHAR8 *Buffer,
    UINTN BufferSize
)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    if (Buffer == NULL) return E_POINTER;

    strncpy(Buffer, impl->EditText, BufferSize - 1);
    Buffer[BufferSize - 1] = '\0';
    return S_OK;
}

static HRESULT ANXAPI ComboBox_SetText(
    ITuiComboBox *This,
    CONST CHAR8 *Text
)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    if (Text == NULL) return E_POINTER;

    strncpy(impl->EditText, Text, sizeof(impl->EditText) - 1);
    impl->EditText[sizeof(impl->EditText) - 1] = '\0';
    impl->CursorPos = strlen(impl->EditText);

    return S_OK;
}

static HRESULT ANXAPI ComboBox_SetChangeCallback(
    ITuiComboBox *This,
    HRESULT (*Callback)(VOID *UserData, INT32 Index),
    VOID *UserData
)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    impl->ChangeCallback = Callback;
    impl->UserData = UserData;
    return S_OK;
}

static HRESULT ANXAPI ComboBox_SetDropDownHeight(
    ITuiComboBox *This,
    UINT32 Height
)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    impl->DropDownHeight = Height;
    return S_OK;
}

static HRESULT ANXAPI ComboBox_Render(
    ITuiComboBox *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width
)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    CHAR8 display[512];
    TUI_COLOR fg, bg;
    UINT32 i;

    if (!impl->State.Visible) return S_OK;

    /* Choose colors for combo box field */
    if (!impl->State.Enabled) {
        fg = TuiColorBrightBlack;
        bg = TuiColorBlack;
    } else if (impl->State.Focused) {
        fg = TuiColorBlack;
        bg = TuiColorWhite;
    } else {
        fg = TuiColorWhite;
        bg = TuiColorBlue;
    }

    /* Render combo box field: [Text         ▼] */
    snprintf(display, sizeof(display), "[%-*s▼]",
             (int)(Width - 3), impl->EditText);
    Screen->Vtbl->WriteText(Screen, X, Y, display, fg, bg);

    /* Draw cursor if focused and editable */
    if (impl->State.Focused && impl->Editable && impl->State.Enabled) {
        INT32 cursorX = X + 1 + impl->CursorPos;
        Screen->Vtbl->WriteText(Screen, cursorX, Y, "_", fg, bg);
    }

    /* Render drop-down list if open */
    if (impl->Dropped) {
        UINT32 actualItemCount = impl->VirtualMode ? impl->VirtualItemCount : impl->ItemCount;
        UINT32 dropHeight = impl->DropDownHeight;
        if (dropHeight > actualItemCount) {
            dropHeight = actualItemCount;
        }
        if (dropHeight > 10) dropHeight = 10;  /* Max height */

        /* Draw shadow (offset by 1,1) */
        for (i = 0; i < dropHeight + 2; i++) {
            ClearRect(Screen, X + 1, Y + 1 + i + 1, Width, 1, TuiColorBlack);
        }

        /* Draw drop-down box */
        DrawBoxSingle(Screen, X, Y + 1, Width, dropHeight + 2,
                      TuiColorBlack, TuiColorWhite);

        /* Render items */
        for (i = 0; i < dropHeight && (impl->ScrollOffset + i) < actualItemCount; i++) {
            UINT32 itemIndex = impl->ScrollOffset + i;
            CHAR8 itemText[MAX_ITEM_LENGTH];

            /* Get item text from virtual mode or normal mode */
            if (impl->VirtualMode) {
                if (impl->OnGetVirtualItem) {
                    if (FAILED(impl->OnGetVirtualItem(impl->VirtualUserData, itemIndex, itemText, sizeof(itemText)))) {
                        strcpy(itemText, "");
                    }
                } else {
                    strcpy(itemText, "");
                }
            } else {
                strncpy(itemText, impl->Items[itemIndex], sizeof(itemText) - 1);
                itemText[sizeof(itemText) - 1] = '\0';
            }

            if ((INT32)itemIndex == impl->SelectedIndex) {
                fg = TuiColorBlack;
                bg = TuiColorCyan;
            } else {
                fg = TuiColorBlack;
                bg = TuiColorWhite;
            }

            snprintf(display, sizeof(display), " %-*s ",
                     (int)(Width - 4), itemText);
            Screen->Vtbl->WriteText(Screen, X + 1, Y + 2 + i, display, fg, bg);
        }

        /* Show scrollbar if needed */
        if (actualItemCount > dropHeight) {
            UINT32 scrollPos = (impl->ScrollOffset * dropHeight) / actualItemCount;
            Screen->Vtbl->WriteText(Screen, X + Width - 2, Y + 2 + scrollPos,
                                    "█", TuiColorBlack, TuiColorWhite);
        }
    }

    return S_OK;
}

static HRESULT ANXAPI ComboBox_HandleKey(
    ITuiComboBox *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiComboBoxImpl *impl = (TuiComboBoxImpl *)This;
    UINT32 len;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    if (impl->Dropped) {
        /* Drop-down is open, handle list navigation */
        switch (Key) {
            case TuiKeyUp:
                if (impl->SelectedIndex > 0) {
                    ComboBox_SetSelectedIndex(This, impl->SelectedIndex - 1);
                }
                *Handled = TRUE;
                return S_OK;

            case TuiKeyDown:
                if (impl->SelectedIndex < (INT32)(impl->ItemCount - 1)) {
                    ComboBox_SetSelectedIndex(This, impl->SelectedIndex + 1);
                }
                *Handled = TRUE;
                return S_OK;

            case TuiKeyEnter:
                /* Close drop-down */
                impl->Dropped = FALSE;
                *Handled = TRUE;
                return S_OK;

            case TuiKeyEsc:
                /* Cancel and close */
                impl->Dropped = FALSE;
                *Handled = TRUE;
                return S_OK;
        }
    } else {
        /* Drop-down is closed */
        if (Key == TuiKeyDown || Key == ' ') {
            /* Open drop-down */
            impl->Dropped = TRUE;
            *Handled = TRUE;
            return S_OK;
        }

        /* Handle text editing if editable */
        if (impl->Editable) {
            len = strlen(impl->EditText);

            switch (Key) {
                case TuiKeyBackspace:
                    if (impl->CursorPos > 0) {
                        memmove(&impl->EditText[impl->CursorPos - 1],
                                &impl->EditText[impl->CursorPos],
                                len - impl->CursorPos + 1);
                        impl->CursorPos--;
                    }
                    *Handled = TRUE;
                    return S_OK;

                case TuiKeyDelete:
                    if (impl->CursorPos < len) {
                        memmove(&impl->EditText[impl->CursorPos],
                                &impl->EditText[impl->CursorPos + 1],
                                len - impl->CursorPos);
                    }
                    *Handled = TRUE;
                    return S_OK;

                case TuiKeyLeft:
                    if (impl->CursorPos > 0) impl->CursorPos--;
                    *Handled = TRUE;
                    return S_OK;

                case TuiKeyRight:
                    if (impl->CursorPos < len) impl->CursorPos++;
                    *Handled = TRUE;
                    return S_OK;

                case TuiKeyHome:
                    impl->CursorPos = 0;
                    *Handled = TRUE;
                    return S_OK;

                case TuiKeyEnd:
                    impl->CursorPos = len;
                    *Handled = TRUE;
                    return S_OK;

                default:
                    /* Handle printable characters */
                    if (Key >= 32 && Key <= 126) {
                        if (len < sizeof(impl->EditText) - 1) {
                            memmove(&impl->EditText[impl->CursorPos + 1],
                                    &impl->EditText[impl->CursorPos],
                                    len - impl->CursorPos + 1);
                            impl->EditText[impl->CursorPos] = (CHAR8)Key;
                            impl->CursorPos++;
                        }
                        *Handled = TRUE;
                        return S_OK;
                    }
                    break;
            }
        }
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiComboBox_Vtbl ComboBoxVtbl = {
    ComboBox_QueryInterface,
    ComboBox_AddRef,
    ComboBox_Release,
    ComboBox_SetEditable,
    ComboBox_AddItem,
    ComboBox_RemoveItem,
    ComboBox_Clear,
    ComboBox_GetSelectedIndex,
    ComboBox_SetSelectedIndex,
    ComboBox_GetText,
    ComboBox_SetText,
    ComboBox_SetChangeCallback,
    ComboBox_SetDropDownHeight,
    ComboBox_Render,
    ComboBox_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateComboBox(
    IN  BOOLEAN Editable,
    OUT ITuiComboBox **ComboBox
)
{
    TuiComboBoxImpl *impl;

    if (ComboBox == NULL) return E_POINTER;

    impl = (TuiComboBoxImpl *)calloc(1, sizeof(TuiComboBoxImpl));
    if (impl == NULL) {
        *ComboBox = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &ComboBoxVtbl;
    InitWidgetState(&impl->State);

    impl->ItemCount = 0;
    impl->SelectedIndex = -1;
    impl->EditText[0] = '\0';
    impl->Editable = Editable;
    impl->Dropped = FALSE;
    impl->DropDownHeight = 10;
    impl->ScrollOffset = 0;
    impl->CursorPos = 0;
    impl->ChangeCallback = NULL;
    impl->UserData = NULL;

    *ComboBox = &impl->Interface;
    return S_OK;
}
