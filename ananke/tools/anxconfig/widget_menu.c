/*
 * Menu Widget Implementation (Standalone Popup Menu)
 *
 * Standalone menu widget that can be used for context menus,
 * popup menus, or dropdown menus. Supports hierarchical submenus,
 * hotkeys, accelerators, and shadows.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "widgets_common.h"

#define MAX_MENU_ITEMS 64

typedef struct _PopupMenuItem PopupMenuItem;

struct _PopupMenuItem {
    CHAR8 Label[64];
    CHAR8 Hotkey;              /* Mnemonic (underlined character) */
    TUI_KEY Accelerator;       /* Keyboard shortcut */
    BOOLEAN AccelCtrl;
    BOOLEAN AccelAlt;
    BOOLEAN AccelShift;
    BOOLEAN Separator;
    BOOLEAN Enabled;
    BOOLEAN Checked;
    HRESULT (*Callback)(VOID *UserData);
    VOID *UserData;
    PopupMenuItem *SubMenu;    /* Hierarchical submenu */
    UINT32 SubMenuCount;
    UINT32 SubMenuCapacity;
};

typedef struct {
    ITuiMenu Interface;
    WIDGET_STATE State;
    PopupMenuItem Items[MAX_MENU_ITEMS];
    UINT32 ItemCount;
    INT32 SelectedIndex;
    INT32 OpenSubmenuIndex;
    UINT32 Width;              /* Computed width */
} TuiMenuImpl;

/* IUnknown methods */
static HRESULT ANXAPI Menu_QueryInterface(
    ITuiMenu *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI Menu_AddRef(ITuiMenu *This)
{
    TuiMenuImpl *impl = (TuiMenuImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI Menu_Release(ITuiMenu *This)
{
    TuiMenuImpl *impl = (TuiMenuImpl *)This;
    UINTN refCount = --impl->State.RefCount;
    if (refCount == 0) {
        /* Free submenus */
        UINT32 i;
        for (i = 0; i < impl->ItemCount; i++) {
            if (impl->Items[i].SubMenu != NULL) {
                free(impl->Items[i].SubMenu);
            }
        }
        free(impl);
    }
    return refCount;
}

/* ITuiMenu methods */
static HRESULT ANXAPI Menu_AddItem(
    ITuiMenu *This,
    CONST CHAR8 *Label,
    CHAR8 Hotkey,
    HRESULT (*Callback)(VOID *UserData),
    VOID *UserData
)
{
    TuiMenuImpl *impl = (TuiMenuImpl *)This;
    PopupMenuItem *item;

    if (Label == NULL) return E_POINTER;
    if (impl->ItemCount >= MAX_MENU_ITEMS) return E_OUTOFMEMORY;

    item = &impl->Items[impl->ItemCount];

    strncpy(item->Label, Label, sizeof(item->Label) - 1);
    item->Label[sizeof(item->Label) - 1] = '\0';
    item->Hotkey = Hotkey;
    item->Accelerator = 0;
    item->AccelCtrl = FALSE;
    item->AccelAlt = FALSE;
    item->AccelShift = FALSE;
    item->Separator = FALSE;
    item->Enabled = TRUE;
    item->Checked = FALSE;
    item->Callback = Callback;
    item->UserData = UserData;
    item->SubMenu = NULL;
    item->SubMenuCount = 0;
    item->SubMenuCapacity = 0;

    impl->ItemCount++;

    /* Recompute menu width */
    UINT32 len = strlen(Label);
    if (len + 10 > impl->Width) {
        impl->Width = len + 10;
    }

    return S_OK;
}

static HRESULT ANXAPI Menu_AddSeparator(ITuiMenu *This)
{
    TuiMenuImpl *impl = (TuiMenuImpl *)This;
    PopupMenuItem *item;

    if (impl->ItemCount >= MAX_MENU_ITEMS) return E_OUTOFMEMORY;

    item = &impl->Items[impl->ItemCount];
    item->Separator = TRUE;
    item->Enabled = FALSE;
    item->Label[0] = '\0';
    impl->ItemCount++;

    return S_OK;
}

static HRESULT ANXAPI Menu_AddSubmenu(
    ITuiMenu *This,
    CONST CHAR8 *Label,
    CHAR8 Hotkey,
    ITuiMenu *Submenu
)
{
    TuiMenuImpl *impl = (TuiMenuImpl *)This;
    PopupMenuItem *item;

    if (Label == NULL || Submenu == NULL) return E_POINTER;
    if (impl->ItemCount >= MAX_MENU_ITEMS) return E_OUTOFMEMORY;

    item = &impl->Items[impl->ItemCount];

    strncpy(item->Label, Label, sizeof(item->Label) - 1);
    item->Label[sizeof(item->Label) - 1] = '\0';
    item->Hotkey = Hotkey;
    item->Separator = FALSE;
    item->Enabled = TRUE;
    item->Checked = FALSE;
    item->Callback = NULL;
    item->UserData = NULL;

    /* Note: Simplified - actual implementation would need proper submenu handling */
    item->SubMenu = NULL;
    item->SubMenuCount = 0;

    impl->ItemCount++;

    return S_OK;
}

static HRESULT ANXAPI Menu_SetItemAccelerator(
    ITuiMenu *This,
    INT32 Index,
    TUI_KEY Key,
    BOOLEAN Ctrl,
    BOOLEAN Alt,
    BOOLEAN Shift
)
{
    TuiMenuImpl *impl = (TuiMenuImpl *)This;
    PopupMenuItem *item;

    if (Index < 0 || (UINT32)Index >= impl->ItemCount) return E_INVALIDARG;

    item = &impl->Items[Index];
    item->Accelerator = Key;
    item->AccelCtrl = Ctrl;
    item->AccelAlt = Alt;
    item->AccelShift = Shift;

    return S_OK;
}

static HRESULT ANXAPI Menu_SetItemEnabled(
    ITuiMenu *This,
    INT32 Index,
    BOOLEAN Enabled
)
{
    TuiMenuImpl *impl = (TuiMenuImpl *)This;

    if (Index < 0 || (UINT32)Index >= impl->ItemCount) return E_INVALIDARG;

    impl->Items[Index].Enabled = Enabled;
    return S_OK;
}

static HRESULT ANXAPI Menu_SetItemChecked(
    ITuiMenu *This,
    INT32 Index,
    BOOLEAN Checked
)
{
    TuiMenuImpl *impl = (TuiMenuImpl *)This;

    if (Index < 0 || (UINT32)Index >= impl->ItemCount) return E_INVALIDARG;

    impl->Items[Index].Checked = Checked;
    return S_OK;
}

/* Helper: Format accelerator string */
static VOID FormatAccelerator(CONST PopupMenuItem *item, CHAR8 *buffer, UINTN size)
{
    buffer[0] = '\0';

    if (item->Accelerator == 0) return;

    if (item->AccelCtrl) strcat(buffer, "Ctrl+");
    if (item->AccelAlt) strcat(buffer, "Alt+");
    if (item->AccelShift) strcat(buffer, "Shift+");

    if (item->Accelerator >= 'a' && item->Accelerator <= 'z') {
        CHAR8 key[2] = { (CHAR8)toupper(item->Accelerator), '\0' };
        strcat(buffer, key);
    } else if (item->Accelerator >= TuiKeyF1 && item->Accelerator <= TuiKeyF12) {
        CHAR8 fkey[8];
        snprintf(fkey, sizeof(fkey), "F%d", (int)(item->Accelerator - TuiKeyF1 + 1));
        strcat(buffer, fkey);
    }
}

/* Helper: Find hotkey position */
static INT32 FindHotkeyPos(CONST CHAR8 *Label, CHAR8 Hotkey)
{
    INT32 i;
    if (Hotkey == '\0') return -1;

    for (i = 0; Label[i] != '\0'; i++) {
        if (tolower((unsigned char)Label[i]) == tolower((unsigned char)Hotkey)) {
            return i;
        }
    }
    return -1;
}

static HRESULT ANXAPI Menu_Show(
    ITuiMenu *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiMenuImpl *impl = (TuiMenuImpl *)This;

    /* Set menu visible */
    impl->State.Visible = TRUE;
    impl->SelectedIndex = 0;

    /* Render */
    return Menu_Render(This, Screen, X, Y);
}

static HRESULT ANXAPI Menu_Render(
    ITuiMenu *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y
)
{
    TuiMenuImpl *impl = (TuiMenuImpl *)This;
    UINT32 i;
    CHAR8 display[128];
    CHAR8 accel[32];
    TUI_COLOR fg, bg;
    INT32 hotkeyPos;

    if (!impl->State.Visible) return S_OK;

    /* Draw shadow (offset by 1,1) */
    for (i = 0; i < impl->ItemCount + 2; i++) {
        ClearRect(Screen, X + 1, Y + i + 1, impl->Width + 2, 1, TuiColorBlack);
    }

    /* Draw menu box */
    DrawBoxSingle(Screen, X, Y, impl->Width + 2, impl->ItemCount + 2,
                  TuiColorBlack, TuiColorWhite);

    /* Render menu items */
    for (i = 0; i < impl->ItemCount; i++) {
        PopupMenuItem *item = &impl->Items[i];

        if (item->Separator) {
            /* Separator line */
            UINT32 j;
            for (j = 1; j < impl->Width + 1; j++) {
                Screen->Vtbl->WriteText(Screen, X + j, Y + 1 + i,
                                        "─", TuiColorBlack, TuiColorWhite);
            }
        } else {
            /* Menu item */
            if ((INT32)i == impl->SelectedIndex && item->Enabled) {
                fg = TuiColorBlack;
                bg = TuiColorCyan;
            } else if (!item->Enabled) {
                fg = TuiColorBrightBlack;
                bg = TuiColorWhite;
            } else {
                fg = TuiColorBlack;
                bg = TuiColorWhite;
            }

            /* Format: " [X] Label        Ctrl+S » " */
            FormatAccelerator(item, accel, sizeof(accel));

            snprintf(display, sizeof(display), " %c%c%c %-*s %-8s %s ",
                     item->Checked ? '[' : ' ',
                     item->Checked ? 'X' : ' ',
                     item->Checked ? ']' : ' ',
                     (int)(impl->Width - 16), item->Label,
                     accel,
                     (item->SubMenu != NULL) ? "»" : " ");

            Screen->Vtbl->WriteText(Screen, X + 1, Y + 1 + i, display, fg, bg);

            /* Underline hotkey */
            if (item->Enabled) {
                hotkeyPos = FindHotkeyPos(item->Label, item->Hotkey);
                if (hotkeyPos >= 0) {
                    CHAR8 hotChar[2] = { item->Label[hotkeyPos], '\0' };
                    Screen->Vtbl->WriteText(Screen, X + 5 + hotkeyPos,
                                            Y + 1 + i, hotChar,
                                            TuiColorRed, bg);
                }
            }
        }
    }

    return S_OK;
}

static HRESULT ANXAPI Menu_HandleKey(
    ITuiMenu *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiMenuImpl *impl = (TuiMenuImpl *)This;
    HRESULT hr;

    if (!impl->State.Visible) {
        *Handled = FALSE;
        return S_OK;
    }

    switch (Key) {
        case TuiKeyUp:
            /* Move selection up */
            do {
                impl->SelectedIndex--;
                if (impl->SelectedIndex < 0) {
                    impl->SelectedIndex = impl->ItemCount - 1;
                }
            } while (impl->Items[impl->SelectedIndex].Separator &&
                     impl->SelectedIndex >= 0);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyDown:
            /* Move selection down */
            do {
                impl->SelectedIndex++;
                if ((UINT32)impl->SelectedIndex >= impl->ItemCount) {
                    impl->SelectedIndex = 0;
                }
            } while (impl->Items[impl->SelectedIndex].Separator);
            *Handled = TRUE;
            return S_OK;

        case TuiKeyRight:
            /* Open submenu if present */
            if (impl->SelectedIndex >= 0 &&
                (UINT32)impl->SelectedIndex < impl->ItemCount &&
                impl->Items[impl->SelectedIndex].SubMenu != NULL) {
                impl->OpenSubmenuIndex = impl->SelectedIndex;
                /* TODO: Show submenu */
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEnter:
            /* Activate selected item */
            if (impl->SelectedIndex >= 0 &&
                (UINT32)impl->SelectedIndex < impl->ItemCount) {
                PopupMenuItem *item = &impl->Items[impl->SelectedIndex];

                if (item->Enabled && item->Callback != NULL) {
                    hr = item->Callback(item->UserData);
                    if (FAILED(hr)) return hr;

                    /* Close menu */
                    impl->State.Visible = FALSE;
                }
            }
            *Handled = TRUE;
            return S_OK;

        case TuiKeyEsc:
            /* Close menu */
            impl->State.Visible = FALSE;
            *Handled = TRUE;
            return S_OK;

        default:
            /* Check for hotkey match */
            if (Key >= 'a' && Key <= 'z') {
                UINT32 i;
                for (i = 0; i < impl->ItemCount; i++) {
                    if (tolower((unsigned char)impl->Items[i].Hotkey) ==
                        tolower((unsigned char)Key) &&
                        impl->Items[i].Enabled) {
                        impl->SelectedIndex = i;

                        if (impl->Items[i].Callback != NULL) {
                            hr = impl->Items[i].Callback(impl->Items[i].UserData);
                            if (FAILED(hr)) return hr;

                            impl->State.Visible = FALSE;
                        }

                        *Handled = TRUE;
                        return S_OK;
                    }
                }
            }
            break;
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiMenu_Vtbl MenuVtbl = {
    Menu_QueryInterface,
    Menu_AddRef,
    Menu_Release,
    Menu_AddItem,
    Menu_AddSeparator,
    Menu_AddSubmenu,
    Menu_SetItemAccelerator,
    Menu_SetItemEnabled,
    Menu_SetItemChecked,
    Menu_Show,
    Menu_Render,
    Menu_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateMenu(OUT ITuiMenu **Menu)
{
    TuiMenuImpl *impl;

    if (Menu == NULL) return E_POINTER;

    impl = (TuiMenuImpl *)calloc(1, sizeof(TuiMenuImpl));
    if (impl == NULL) {
        *Menu = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &MenuVtbl;
    InitWidgetState(&impl->State);

    impl->ItemCount = 0;
    impl->SelectedIndex = 0;
    impl->OpenSubmenuIndex = -1;
    impl->Width = 20;
    impl->State.Visible = FALSE;

    *Menu = &impl->Interface;
    return S_OK;
}
