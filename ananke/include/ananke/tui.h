/** @file
  ANANKE Portable Text User Interface

  Provides a portable TUI library using COM interfaces.
  Works across all platforms and compilers.

  Copyright (C) 2025 A•NUX Project
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ANANKE_TUI_H__
#define __ANANKE_TUI_H__

#include <ananke/base.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Forward declarations
//
typedef struct _ITuiScreen ITuiScreen;
typedef struct _ITuiWindow ITuiWindow;
typedef struct _ITuiMenu ITuiMenu;
typedef struct _ITuiInput ITuiInput;

//
// TUI Color Attributes
//
typedef enum _TUI_COLOR {
    TuiColorBlack = 0,
    TuiColorRed,
    TuiColorGreen,
    TuiColorYellow,
    TuiColorBlue,
    TuiColorMagenta,
    TuiColorCyan,
    TuiColorWhite,
    TuiColorBrightBlack,
    TuiColorBrightRed,
    TuiColorBrightGreen,
    TuiColorBrightYellow,
    TuiColorBrightBlue,
    TuiColorBrightMagenta,
    TuiColorBrightCyan,
    TuiColorBrightWhite
} TUI_COLOR;

typedef enum _TUI_ATTRIBUTE {
    TuiAttrNormal = 0x00,
    TuiAttrBold = 0x01,
    TuiAttrDim = 0x02,
    TuiAttrUnderline = 0x04,
    TuiAttrBlink = 0x08,
    TuiAttrReverse = 0x10
} TUI_ATTRIBUTE;

//
// TUI Key Codes
//
typedef enum _TUI_KEY {
    TuiKeyNone = 0,
    TuiKeyEnter = 13,
    TuiKeyEscape = 27,
    TuiKeyBackspace = 8,
    TuiKeyTab = 9,
    TuiKeyUp = 256,
    TuiKeyDown,
    TuiKeyLeft,
    TuiKeyRight,
    TuiKeyHome,
    TuiKeyEnd,
    TuiKeyPageUp,
    TuiKeyPageDown,
    TuiKeyInsert,
    TuiKeyDelete,
    TuiKeyF1,
    TuiKeyF2,
    TuiKeyF3,
    TuiKeyF4,
    TuiKeyF5,
    TuiKeyF6,
    TuiKeyF7,
    TuiKeyF8,
    TuiKeyF9,
    TuiKeyF10,
    TuiKeyF11,
    TuiKeyF12
} TUI_KEY;

//
// TUI Rectangle
//
typedef struct _TUI_RECT {
    INT32 X;
    INT32 Y;
    INT32 Width;
    INT32 Height;
} TUI_RECT;

//
// TUI Cell (character + attributes)
//
typedef struct _TUI_CELL {
    CHAR16 Character;
    TUI_COLOR Foreground;
    TUI_COLOR Background;
    UINT32 Attributes;
} TUI_CELL;

// {8F3D5E1A-2B4C-4D9E-A1F3-7C8E9D0A1B2C}
DEFINE_GUID(IID_ITuiScreen,
    0x8F3D5E1A, 0x2B4C, 0x4D9E, 0xA1, 0xF3, 0x7C, 0x8E, 0x9D, 0x0A, 0x1B, 0x2C);

/**
  ITuiScreen Interface

  Represents the entire terminal screen.
**/
typedef struct _ITuiScreen_Vtbl {
    //
    // IUnknown methods
    //
    HRESULT (ANXAPI *QueryInterface)(
        ITuiScreen *This,
        REFIID riid,
        VOID **ppvObject
    );

    UINTN (ANXAPI *AddRef)(
        ITuiScreen *This
    );

    UINTN (ANXAPI *Release)(
        ITuiScreen *This
    );

    //
    // ITuiScreen methods
    //

    /**
      Initialize the screen and terminal.
    **/
    HRESULT (ANXAPI *Initialize)(
        ITuiScreen *This
    );

    /**
      Shutdown and restore terminal.
    **/
    HRESULT (ANXAPI *Shutdown)(
        ITuiScreen *This
    );

    /**
      Get screen dimensions.
    **/
    HRESULT (ANXAPI *GetDimensions)(
        ITuiScreen *This,
        UINT32 *Width,
        UINT32 *Height
    );

    /**
      Clear the screen.
    **/
    HRESULT (ANXAPI *Clear)(
        ITuiScreen *This
    );

    /**
      Refresh the screen (update display).
    **/
    HRESULT (ANXAPI *Refresh)(
        ITuiScreen *This
    );

    /**
      Write text at position.
    **/
    HRESULT (ANXAPI *WriteText)(
        ITuiScreen *This,
        INT32 X,
        INT32 Y,
        CONST CHAR8 *Text,
        TUI_COLOR Foreground,
        TUI_COLOR Background
    );

    /**
      Draw a box with border.
    **/
    HRESULT (ANXAPI *DrawBox)(
        ITuiScreen *This,
        CONST TUI_RECT *Rect,
        CONST CHAR8 *Title,
        TUI_COLOR Foreground,
        TUI_COLOR Background
    );

    /**
      Get next key input (blocking).
    **/
    HRESULT (ANXAPI *GetKey)(
        ITuiScreen *This,
        TUI_KEY *Key
    );

} ITuiScreen_Vtbl;

struct _ITuiScreen {
    CONST ITuiScreen_Vtbl *Vtbl;
};

// {B2C3D4E5-F6A7-4B8C-9D0E-1F2A3B4C5D6E}
DEFINE_GUID(IID_ITuiWindow,
    0xB2C3D4E5, 0xF6A7, 0x4B8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E);

/**
  ITuiWindow Interface

  Represents a window within the screen.
**/
typedef struct _ITuiWindow_Vtbl {
    //
    // IUnknown methods
    //
    HRESULT (ANXAPI *QueryInterface)(
        ITuiWindow *This,
        REFIID riid,
        VOID **ppvObject
    );

    UINTN (ANXAPI *AddRef)(
        ITuiWindow *This
    );

    UINTN (ANXAPI *Release)(
        ITuiWindow *This
    );

    //
    // ITuiWindow methods
    //

    /**
      Set window position and size.
    **/
    HRESULT (ANXAPI *SetBounds)(
        ITuiWindow *This,
        CONST TUI_RECT *Rect
    );

    /**
      Show or hide window.
    **/
    HRESULT (ANXAPI *SetVisible)(
        ITuiWindow *This,
        BOOLEAN Visible
    );

    /**
      Clear window contents.
    **/
    HRESULT (ANXAPI *Clear)(
        ITuiWindow *This
    );

    /**
      Write text in window.
    **/
    HRESULT (ANXAPI *WriteText)(
        ITuiWindow *This,
        INT32 X,
        INT32 Y,
        CONST CHAR8 *Text
    );

    /**
      Scroll window contents.
    **/
    HRESULT (ANXAPI *Scroll)(
        ITuiWindow *This,
        INT32 Lines
    );

} ITuiWindow_Vtbl;

struct _ITuiWindow {
    CONST ITuiWindow_Vtbl *Vtbl;
};

// {C3D4E5F6-A7B8-4C9D-0E1F-2A3B4C5D6E7F}
DEFINE_GUID(IID_ITuiMenu,
    0xC3D4E5F6, 0xA7B8, 0x4C9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F);

/**
  Menu Item Type
**/
typedef enum _TUI_MENU_ITEM_TYPE {
    TuiMenuItemSubmenu,
    TuiMenuItemBoolean,
    TuiMenuItemChoice,
    TuiMenuItemString,
    TuiMenuItemInteger,
    TuiMenuItemSeparator,
    TuiMenuItemInfo
} TUI_MENU_ITEM_TYPE;

/**
  ITuiMenu Interface

  Represents an interactive menu.
**/
typedef struct _ITuiMenu_Vtbl {
    //
    // IUnknown methods
    //
    HRESULT (ANXAPI *QueryInterface)(
        ITuiMenu *This,
        REFIID riid,
        VOID **ppvObject
    );

    UINTN (ANXAPI *AddRef)(
        ITuiMenu *This
    );

    UINTN (ANXAPI *Release)(
        ITuiMenu *This
    );

    //
    // ITuiMenu methods
    //

    /**
      Add a menu item.
    **/
    HRESULT (ANXAPI *AddItem)(
        ITuiMenu *This,
        TUI_MENU_ITEM_TYPE Type,
        CONST CHAR8 *Label,
        CONST CHAR8 *Help,
        VOID *UserData
    );

    /**
      Display menu and run event loop.
      Returns selected item index or -1 if cancelled.
    **/
    HRESULT (ANXAPI *Run)(
        ITuiMenu *This,
        INT32 *SelectedIndex
    );

    /**
      Get item value (for boolean/choice/string/integer items).
    **/
    HRESULT (ANXAPI *GetItemValue)(
        ITuiMenu *This,
        INT32 Index,
        VOID *Value,
        UINTN ValueSize
    );

    /**
      Set item value.
    **/
    HRESULT (ANXAPI *SetItemValue)(
        ITuiMenu *This,
        INT32 Index,
        CONST VOID *Value,
        UINTN ValueSize
    );

} ITuiMenu_Vtbl;

struct _ITuiMenu {
    CONST ITuiMenu_Vtbl *Vtbl;
};

//
// Factory functions
//

/**
  Create a TUI Screen instance.

  @param[out] Screen  Pointer to receive the screen interface.

  @retval S_OK        Screen created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateScreen(
    OUT ITuiScreen **Screen
);

/**
  Create a TUI Window instance.

  @param[in]  ParentScreen  Parent screen for this window.
  @param[out] Window        Pointer to receive the window interface.

  @retval S_OK        Window created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateWindow(
    IN  ITuiScreen *ParentScreen,
    OUT ITuiWindow **Window
);

/**
  Create a TUI Menu instance.

  @param[in]  ParentScreen  Parent screen for this menu.
  @param[in]  Title         Menu title.
  @param[out] Menu          Pointer to receive the menu interface.

  @retval S_OK        Menu created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateMenu(
    IN  ITuiScreen *ParentScreen,
    IN  CONST CHAR8 *Title,
    OUT ITuiMenu **Menu
);

#ifdef __cplusplus
}
#endif

#endif /* __ANANKE_TUI_H__ */
