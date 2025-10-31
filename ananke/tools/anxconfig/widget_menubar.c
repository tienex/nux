/*
 * MenuBar Widget Implementation
 *
 * Horizontal menu bar with hierarchical drop-down menus.
 * Supports hotkeys (Alt+key) and accelerators (Ctrl+key).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "widgets_common.h"

#define MAX_MENU_ITEMS 32

typedef struct _MenuItem MenuItem;

struct _MenuItem {
    CHAR8 Label[64];
    CHAR8 Hotkey;              /* Alt+key to activate */
    TUI_KEY Accelerator;       /* Keyboard shortcut (e.g., Ctrl+S) */
    BOOLEAN AccelCtrl;
    BOOLEAN AccelAlt;
    BOOLEAN AccelShift;
    BOOLEAN Separator;
    BOOLEAN Enabled;
    BOOLEAN Checked;
    HRESULT (*Callback)(VOID *UserData);
    VOID *UserData;
    MenuItem *SubMenu;         /* Hierarchical submenu */
    UINT32 SubMenuCount;
};

typedef struct {
    ITuiMenuBar Interface;
    WIDGET_STATE State;
    MenuItem Items[MAX_MENU_ITEMS];
    UINT32 ItemCount;
    INT32 SelectedIndex;
    INT32 OpenMenuIndex;       /* Currently open drop-down menu */
    BOOLEAN MenuOpen;
} TuiMenuBarImpl;

/* IUnknown methods */
static HRESULT ANXAPI MenuBar_QueryInterface(
    ITuiMenuBar *This,
    REFIID riid,
    VOID **ppvObject
)
{
    if (ppvObject == NULL) return E_POINTER;
    *ppvObject = This;
    This->Vtbl->AddRef(This);
    return S_OK;
}

static UINTN ANXAPI MenuBar_AddRef(ITuiMenuBar *This)
{
    TuiMenuBarImpl *impl = (TuiMenuBarImpl *)This;
    return ++impl->State.RefCount;
}

static UINTN ANXAPI MenuBar_Release(ITuiMenuBar *This)
{
    TuiMenuBarImpl *impl = (TuiMenuBarImpl *)This;
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

/* ITuiMenuBar methods */
static HRESULT ANXAPI MenuBar_AddMenu(
    ITuiMenuBar *This,
    CONST CHAR8 *Label,
    CHAR8 Hotkey
)
{
    TuiMenuBarImpl *impl = (TuiMenuBarImpl *)This;

    if (Label == NULL) return E_POINTER;
    if (impl->ItemCount >= MAX_MENU_ITEMS) return E_OUTOFMEMORY;

    strncpy(impl->Items[impl->ItemCount].Label, Label, sizeof(impl->Items[0].Label) - 1);
    impl->Items[impl->ItemCount].Label[sizeof(impl->Items[0].Label) - 1] = '\0';
    impl->Items[impl->ItemCount].Hotkey = Hotkey;
    impl->Items[impl->ItemCount].Separator = FALSE;
    impl->Items[impl->ItemCount].Enabled = TRUE;
    impl->Items[impl->ItemCount].Checked = FALSE;
    impl->Items[impl->ItemCount].Callback = NULL;
    impl->Items[impl->ItemCount].UserData = NULL;
    impl->Items[impl->ItemCount].SubMenu = NULL;
    impl->Items[impl->ItemCount].SubMenuCount = 0;
    impl->ItemCount++;

    return S_OK;
}

static HRESULT ANXAPI MenuBar_AddMenuItem(
    ITuiMenuBar *This,
    INT32 MenuIndex,
    CONST CHAR8 *Label,
    CHAR8 Hotkey,
    HRESULT (*Callback)(VOID *UserData),
    VOID *UserData
)
{
    TuiMenuBarImpl *impl = (TuiMenuBarImpl *)This;
    MenuItem *item;

    if (Label == NULL) return E_POINTER;
    if (MenuIndex < 0 || (UINT32)MenuIndex >= impl->ItemCount) return E_INVALIDARG;

    /* Allocate submenu array if needed */
    if (impl->Items[MenuIndex].SubMenu == NULL) {
        impl->Items[MenuIndex].SubMenu = (MenuItem *)calloc(MAX_MENU_ITEMS, sizeof(MenuItem));
        if (impl->Items[MenuIndex].SubMenu == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    if (impl->Items[MenuIndex].SubMenuCount >= MAX_MENU_ITEMS) {
        return E_OUTOFMEMORY;
    }

    item = &impl->Items[MenuIndex].SubMenu[impl->Items[MenuIndex].SubMenuCount];

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

    impl->Items[MenuIndex].SubMenuCount++;

    return S_OK;
}

static HRESULT ANXAPI MenuBar_AddSeparator(
    ITuiMenuBar *This,
    INT32 MenuIndex
)
{
    TuiMenuBarImpl *impl = (TuiMenuBarImpl *)This;
    MenuItem *item;

    if (MenuIndex < 0 || (UINT32)MenuIndex >= impl->ItemCount) return E_INVALIDARG;

    /* Allocate submenu array if needed */
    if (impl->Items[MenuIndex].SubMenu == NULL) {
        impl->Items[MenuIndex].SubMenu = (MenuItem *)calloc(MAX_MENU_ITEMS, sizeof(MenuItem));
        if (impl->Items[MenuIndex].SubMenu == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    if (impl->Items[MenuIndex].SubMenuCount >= MAX_MENU_ITEMS) {
        return E_OUTOFMEMORY;
    }

    item = &impl->Items[MenuIndex].SubMenu[impl->Items[MenuIndex].SubMenuCount];
    item->Separator = TRUE;
    item->Enabled = FALSE;

    impl->Items[MenuIndex].SubMenuCount++;

    return S_OK;
}

static HRESULT ANXAPI MenuBar_SetItemAccelerator(
    ITuiMenuBar *This,
    INT32 MenuIndex,
    INT32 ItemIndex,
    TUI_KEY Key,
    BOOLEAN Ctrl,
    BOOLEAN Alt,
    BOOLEAN Shift
)
{
    TuiMenuBarImpl *impl = (TuiMenuBarImpl *)This;
    MenuItem *item;

    if (MenuIndex < 0 || (UINT32)MenuIndex >= impl->ItemCount) return E_INVALIDARG;
    if (impl->Items[MenuIndex].SubMenu == NULL) return E_INVALIDARG;
    if (ItemIndex < 0 || (UINT32)ItemIndex >= impl->Items[MenuIndex].SubMenuCount) {
        return E_INVALIDARG;
    }

    item = &impl->Items[MenuIndex].SubMenu[ItemIndex];
    item->Accelerator = Key;
    item->AccelCtrl = Ctrl;
    item->AccelAlt = Alt;
    item->AccelShift = Shift;

    return S_OK;
}

static HRESULT ANXAPI MenuBar_SetItemEnabled(
    ITuiMenuBar *This,
    INT32 MenuIndex,
    INT32 ItemIndex,
    BOOLEAN Enabled
)
{
    TuiMenuBarImpl *impl = (TuiMenuBarImpl *)This;
    MenuItem *item;

    if (MenuIndex < 0 || (UINT32)MenuIndex >= impl->ItemCount) return E_INVALIDARG;
    if (impl->Items[MenuIndex].SubMenu == NULL) return E_INVALIDARG;
    if (ItemIndex < 0 || (UINT32)ItemIndex >= impl->Items[MenuIndex].SubMenuCount) {
        return E_INVALIDARG;
    }

    item = &impl->Items[MenuIndex].SubMenu[ItemIndex];
    item->Enabled = Enabled;

    return S_OK;
}

static HRESULT ANXAPI MenuBar_SetItemChecked(
    ITuiMenuBar *This,
    INT32 MenuIndex,
    INT32 ItemIndex,
    BOOLEAN Checked
)
{
    TuiMenuBarImpl *impl = (TuiMenuBarImpl *)This;
    MenuItem *item;

    if (MenuIndex < 0 || (UINT32)MenuIndex >= impl->ItemCount) return E_INVALIDARG;
    if (impl->Items[MenuIndex].SubMenu == NULL) return E_INVALIDARG;
    if (ItemIndex < 0 || (UINT32)ItemIndex >= impl->Items[MenuIndex].SubMenuCount) {
        return E_INVALIDARG;
    }

    item = &impl->Items[MenuIndex].SubMenu[ItemIndex];
    item->Checked = Checked;

    return S_OK;
}

/* Helper: Find hotkey position in label */
static INT32 FindHotkeyInLabel(CONST CHAR8 *Label, CHAR8 Hotkey)
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

/* Helper: Format accelerator string */
static VOID FormatAccelerator(CONST MenuItem *item, CHAR8 *buffer, UINTN size)
{
    buffer[0] = '\0';

    if (item->Accelerator == 0) return;

    if (item->AccelCtrl) strcat(buffer, "Ctrl+");
    if (item->AccelAlt) strcat(buffer, "Alt+");
    if (item->AccelShift) strcat(buffer, "Shift+");

    /* Append key name */
    if (item->Accelerator >= 'a' && item->Accelerator <= 'z') {
        CHAR8 key[2] = { (CHAR8)toupper(item->Accelerator), '\0' };
        strcat(buffer, key);
    } else if (item->Accelerator >= TuiKeyF1 && item->Accelerator <= TuiKeyF12) {
        CHAR8 fkey[8];
        snprintf(fkey, sizeof(fkey), "F%d", (int)(item->Accelerator - TuiKeyF1 + 1));
        strcat(buffer, fkey);
    }
    /* Add more key names as needed */
}

static HRESULT ANXAPI MenuBar_Render(
    ITuiMenuBar *This,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    UINT32 Width
)
{
    TuiMenuBarImpl *impl = (TuiMenuBarImpl *)This;
    UINT32 i, j;
    INT32 currentX = X;
    CHAR8 display[128];
    TUI_COLOR fg, bg;

    if (!impl->State.Visible) return S_OK;

    /* Clear menu bar background */
    ClearRect(Screen, X, Y, Width, 1, TuiColorBlue);

    /* Render menu items */
    for (i = 0; i < impl->ItemCount; i++) {
        INT32 hotkeyPos;

        /* Choose colors */
        if ((INT32)i == impl->SelectedIndex || (INT32)i == impl->OpenMenuIndex) {
            fg = TuiColorBlack;
            bg = TuiColorWhite;
        } else {
            fg = TuiColorWhite;
            bg = TuiColorBlue;
        }

        /* Format: " Label " */
        snprintf(display, sizeof(display), " %s ", impl->Items[i].Label);
        Screen->Vtbl->WriteText(Screen, currentX, Y, display, fg, bg);

        /* Underline hotkey */
        hotkeyPos = FindHotkeyInLabel(impl->Items[i].Label, impl->Items[i].Hotkey);
        if (hotkeyPos >= 0) {
            CHAR8 hotChar[2] = { impl->Items[i].Label[hotkeyPos], '\0' };
            Screen->Vtbl->WriteText(Screen, currentX + 1 + hotkeyPos, Y, hotChar,
                                    TuiColorYellow, bg);
        }

        currentX += strlen(display);
    }

    /* Render open drop-down menu */
    if (impl->MenuOpen && impl->OpenMenuIndex >= 0 &&
        (UINT32)impl->OpenMenuIndex < impl->ItemCount) {

        MenuItem *menu = impl->Items[impl->OpenMenuIndex].SubMenu;
        UINT32 menuCount = impl->Items[impl->OpenMenuIndex].SubMenuCount;

        if (menu != NULL && menuCount > 0) {
            /* Calculate menu position */
            INT32 menuX = X;
            for (i = 0; i < (UINT32)impl->OpenMenuIndex; i++) {
                menuX += strlen(impl->Items[i].Label) + 2;
            }
            INT32 menuY = Y + 1;

            /* Find max width */
            UINT32 maxWidth = 20;
            for (j = 0; j < menuCount; j++) {
                UINT32 len = strlen(menu[j].Label);
                if (len > maxWidth) maxWidth = len;
            }
            maxWidth += 10;  /* Room for accelerator */

            /* Draw shadow first (offset by 1,1) */
            for (j = 0; j < menuCount + 2; j++) {
                ClearRect(Screen, menuX + 1, menuY + j + 1, maxWidth + 2, 1,
                          TuiColorBlack);
            }

            /* Draw menu box */
            DrawBoxSingle(Screen, menuX, menuY, maxWidth + 2, menuCount + 2,
                          TuiColorBlack, TuiColorWhite);

            /* Draw menu items */
            for (j = 0; j < menuCount; j++) {
                if (menu[j].Separator) {
                    /* Separator line */
                    for (i = 1; i < maxWidth + 1; i++) {
                        Screen->Vtbl->WriteText(Screen, menuX + i, menuY + 1 + j,
                                                "─", TuiColorBlack, TuiColorWhite);
                    }
                } else {
                    CHAR8 accel[32];
                    INT32 hotkeyPos;

                    fg = menu[j].Enabled ? TuiColorBlack : TuiColorBrightBlack;
                    bg = TuiColorWhite;

                    /* Format: " [X] Label        Ctrl+S " */
                    FormatAccelerator(&menu[j], accel, sizeof(accel));

                    snprintf(display, sizeof(display), " %c%c%c %-*s %s ",
                             menu[j].Checked ? '[' : ' ',
                             menu[j].Checked ? 'X' : ' ',
                             menu[j].Checked ? ']' : ' ',
                             (int)(maxWidth - 10), menu[j].Label,
                             accel);

                    Screen->Vtbl->WriteText(Screen, menuX + 1, menuY + 1 + j,
                                            display, fg, bg);

                    /* Underline hotkey */
                    hotkeyPos = FindHotkeyInLabel(menu[j].Label, menu[j].Hotkey);
                    if (hotkeyPos >= 0 && menu[j].Enabled) {
                        CHAR8 hotChar[2] = { menu[j].Label[hotkeyPos], '\0' };
                        Screen->Vtbl->WriteText(Screen, menuX + 5 + hotkeyPos,
                                                menuY + 1 + j, hotChar,
                                                TuiColorRed, bg);
                    }
                }
            }
        }
    }

    return S_OK;
}

static HRESULT ANXAPI MenuBar_HandleKey(
    ITuiMenuBar *This,
    TUI_KEY Key,
    BOOLEAN *Handled
)
{
    TuiMenuBarImpl *impl = (TuiMenuBarImpl *)This;

    if (!impl->State.Enabled) {
        *Handled = FALSE;
        return S_OK;
    }

    /* Check for accelerators (global shortcuts) */
    /* TODO: Implement accelerator matching */

    /* Handle menu navigation */
    if (impl->MenuOpen) {
        /* Menu is open, handle menu item navigation */
        /* TODO: Implement menu item selection */
    } else {
        /* Menu bar navigation */
        switch (Key) {
            case TuiKeyLeft:
                if (impl->SelectedIndex > 0) {
                    impl->SelectedIndex--;
                }
                *Handled = TRUE;
                return S_OK;

            case TuiKeyRight:
                if (impl->SelectedIndex < (INT32)(impl->ItemCount - 1)) {
                    impl->SelectedIndex++;
                }
                *Handled = TRUE;
                return S_OK;

            case TuiKeyEnter:
            case TuiKeyDown:
                /* Open selected menu */
                impl->MenuOpen = TRUE;
                impl->OpenMenuIndex = impl->SelectedIndex;
                *Handled = TRUE;
                return S_OK;
        }
    }

    *Handled = FALSE;
    return S_OK;
}

/* Vtable */
static CONST ITuiMenuBar_Vtbl MenuBarVtbl = {
    MenuBar_QueryInterface,
    MenuBar_AddRef,
    MenuBar_Release,
    MenuBar_AddMenu,
    MenuBar_AddMenuItem,
    MenuBar_AddSeparator,
    MenuBar_SetItemAccelerator,
    MenuBar_SetItemEnabled,
    MenuBar_SetItemChecked,
    MenuBar_Render,
    MenuBar_HandleKey
};

/* Factory function */
HRESULT ANXAPI AnxTuiCreateMenuBar(OUT ITuiMenuBar **MenuBar)
{
    TuiMenuBarImpl *impl;

    if (MenuBar == NULL) return E_POINTER;

    impl = (TuiMenuBarImpl *)calloc(1, sizeof(TuiMenuBarImpl));
    if (impl == NULL) {
        *MenuBar = NULL;
        return E_OUTOFMEMORY;
    }

    impl->Interface.Vtbl = &MenuBarVtbl;
    InitWidgetState(&impl->State);

    impl->ItemCount = 0;
    impl->SelectedIndex = 0;
    impl->OpenMenuIndex = -1;
    impl->MenuOpen = FALSE;

    *MenuBar = &impl->Interface;
    return S_OK;
}
