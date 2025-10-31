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
typedef struct _ITuiCheckbox ITuiCheckbox;
typedef struct _ITuiRadioGroup ITuiRadioGroup;
typedef struct _ITuiButton ITuiButton;
typedef struct _ITuiHelpViewer ITuiHelpViewer;
typedef struct _ITuiListBox ITuiListBox;
typedef struct _ITuiComboBox ITuiComboBox;
typedef struct _ITuiDropDown ITuiDropDown;
typedef struct _ITuiMenuBar ITuiMenuBar;
typedef struct _ITuiStatusBar ITuiStatusBar;
typedef struct _ITuiDesktop ITuiDesktop;
typedef struct _ITuiTheme ITuiTheme;

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
// TUI Mouse Event
//
typedef enum _TUI_MOUSE_BUTTON {
    TuiMouseNone = 0,
    TuiMouseLeft = 1,
    TuiMouseMiddle = 2,
    TuiMouseRight = 3,
    TuiMouseWheelUp = 4,
    TuiMouseWheelDown = 5
} TUI_MOUSE_BUTTON;

typedef enum _TUI_MOUSE_EVENT_TYPE {
    TuiMousePress,
    TuiMouseRelease,
    TuiMouseMove,
    TuiMouseDoubleClick,
    TuiMouseDrag
} TUI_MOUSE_EVENT_TYPE;

typedef struct _TUI_MOUSE_EVENT {
    TUI_MOUSE_EVENT_TYPE Type;
    TUI_MOUSE_BUTTON Button;
    INT32 X;
    INT32 Y;
    BOOLEAN Shift;
    BOOLEAN Ctrl;
    BOOLEAN Alt;
} TUI_MOUSE_EVENT;

//
// TUI Input Event (keyboard or mouse)
//
typedef enum _TUI_INPUT_TYPE {
    TuiInputKeyboard,
    TuiInputMouse
} TUI_INPUT_EVENT_TYPE;

typedef struct _TUI_INPUT_EVENT {
    TUI_INPUT_EVENT_TYPE Type;
    union {
        TUI_KEY Key;
        TUI_MOUSE_EVENT Mouse;
    };
} TUI_INPUT_EVENT;

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

    /**
      Enable or disable mouse support.
    **/
    HRESULT (ANXAPI *SetMouseEnabled)(
        ITuiScreen *This,
        BOOLEAN Enabled
    );

    /**
      Get next input event (keyboard or mouse, blocking).
    **/
    HRESULT (ANXAPI *GetInputEvent)(
        ITuiScreen *This,
        TUI_INPUT_EVENT *Event
    );

    /**
      Get mouse position.
    **/
    HRESULT (ANXAPI *GetMousePosition)(
        ITuiScreen *This,
        INT32 *X,
        INT32 *Y
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

// {D4E5F6A7-B8C9-4D0E-1F2A-3B4C5D6E7F8A}
DEFINE_GUID(IID_ITuiCheckbox,
    0xD4E5F6A7, 0xB8C9, 0x4D0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A);

/**
  ITuiCheckbox Interface

  Represents a checkbox widget (boolean on/off toggle).
**/
typedef struct _ITuiCheckbox_Vtbl {
    //
    // IUnknown methods
    //
    HRESULT (ANXAPI *QueryInterface)(
        ITuiCheckbox *This,
        REFIID riid,
        VOID **ppvObject
    );

    UINTN (ANXAPI *AddRef)(
        ITuiCheckbox *This
    );

    UINTN (ANXAPI *Release)(
        ITuiCheckbox *This
    );

    //
    // ITuiCheckbox methods
    //

    /**
      Set checkbox label.
    **/
    HRESULT (ANXAPI *SetLabel)(
        ITuiCheckbox *This,
        CONST CHAR8 *Label
    );

    /**
      Get checkbox state.
    **/
    HRESULT (ANXAPI *GetChecked)(
        ITuiCheckbox *This,
        BOOLEAN *Checked
    );

    /**
      Set checkbox state.
    **/
    HRESULT (ANXAPI *SetChecked)(
        ITuiCheckbox *This,
        BOOLEAN Checked
    );

    /**
      Set tristate mode (Y/N/M for kernel modules).
    **/
    HRESULT (ANXAPI *SetTristate)(
        ITuiCheckbox *This,
        BOOLEAN Tristate
    );

    /**
      Get tristate value (0=N, 1=M, 2=Y).
    **/
    HRESULT (ANXAPI *GetTristateValue)(
        ITuiCheckbox *This,
        UINT8 *Value
    );

    /**
      Set tristate value.
    **/
    HRESULT (ANXAPI *SetTristateValue)(
        ITuiCheckbox *This,
        UINT8 Value
    );

    /**
      Render checkbox at position.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiCheckbox *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y,
        BOOLEAN Focused
    );

    /**
      Handle key input.
    **/
    HRESULT (ANXAPI *HandleKey)(
        ITuiCheckbox *This,
        TUI_KEY Key,
        BOOLEAN *Handled
    );

} ITuiCheckbox_Vtbl;

struct _ITuiCheckbox {
    CONST ITuiCheckbox_Vtbl *Vtbl;
};

// {E5F6A7B8-C9D0-4E1F-2A3B-4C5D6E7F8A9B}
DEFINE_GUID(IID_ITuiInput,
    0xE5F6A7B8, 0xC9D0, 0x4E1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B);

/**
  Input Field Type
**/
typedef enum _TUI_INPUT_TYPE {
    TuiInputString,
    TuiInputInteger,
    TuiInputHex
} TUI_INPUT_TYPE;

/**
  ITuiInput Interface

  Represents an input field widget (string, integer, hex).
**/
typedef struct _ITuiInput_Vtbl {
    //
    // IUnknown methods
    //
    HRESULT (ANXAPI *QueryInterface)(
        ITuiInput *This,
        REFIID riid,
        VOID **ppvObject
    );

    UINTN (ANXAPI *AddRef)(
        ITuiInput *This
    );

    UINTN (ANXAPI *Release)(
        ITuiInput *This
    );

    //
    // ITuiInput methods
    //

    /**
      Set input type.
    **/
    HRESULT (ANXAPI *SetType)(
        ITuiInput *This,
        TUI_INPUT_TYPE Type
    );

    /**
      Set input label/prompt.
    **/
    HRESULT (ANXAPI *SetLabel)(
        ITuiInput *This,
        CONST CHAR8 *Label
    );

    /**
      Get input value as string.
    **/
    HRESULT (ANXAPI *GetValue)(
        ITuiInput *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Set input value from string.
    **/
    HRESULT (ANXAPI *SetValue)(
        ITuiInput *This,
        CONST CHAR8 *Value
    );

    /**
      Set integer range (for integer inputs).
    **/
    HRESULT (ANXAPI *SetRange)(
        ITuiInput *This,
        INT64 Min,
        INT64 Max
    );

    /**
      Render input field at position.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiInput *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y,
        BOOLEAN Focused
    );

    /**
      Handle key input.
    **/
    HRESULT (ANXAPI *HandleKey)(
        ITuiInput *This,
        TUI_KEY Key,
        BOOLEAN *Handled
    );

} ITuiInput_Vtbl;

struct _ITuiInput {
    CONST ITuiInput_Vtbl *Vtbl;
};

// {F6A7B8C9-D0E1-4F2A-3B4C-5D6E7F8A9B0C}
DEFINE_GUID(IID_ITuiRadioGroup,
    0xF6A7B8C9, 0xD0E1, 0x4F2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C);

/**
  ITuiRadioGroup Interface

  Represents a radio button group (choice selection).
**/
typedef struct _ITuiRadioGroup_Vtbl {
    //
    // IUnknown methods
    //
    HRESULT (ANXAPI *QueryInterface)(
        ITuiRadioGroup *This,
        REFIID riid,
        VOID **ppvObject
    );

    UINTN (ANXAPI *AddRef)(
        ITuiRadioGroup *This
    );

    UINTN (ANXAPI *Release)(
        ITuiRadioGroup *This
    );

    //
    // ITuiRadioGroup methods
    //

    /**
      Set group label.
    **/
    HRESULT (ANXAPI *SetLabel)(
        ITuiRadioGroup *This,
        CONST CHAR8 *Label
    );

    /**
      Add a choice option.
    **/
    HRESULT (ANXAPI *AddChoice)(
        ITuiRadioGroup *This,
        CONST CHAR8 *Label,
        CONST CHAR8 *Value
    );

    /**
      Get selected choice index.
    **/
    HRESULT (ANXAPI *GetSelectedIndex)(
        ITuiRadioGroup *This,
        UINT32 *Index
    );

    /**
      Set selected choice index.
    **/
    HRESULT (ANXAPI *SetSelectedIndex)(
        ITuiRadioGroup *This,
        UINT32 Index
    );

    /**
      Get selected choice value.
    **/
    HRESULT (ANXAPI *GetSelectedValue)(
        ITuiRadioGroup *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Render radio group at position.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiRadioGroup *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y,
        BOOLEAN Focused
    );

    /**
      Handle key input.
    **/
    HRESULT (ANXAPI *HandleKey)(
        ITuiRadioGroup *This,
        TUI_KEY Key,
        BOOLEAN *Handled
    );

} ITuiRadioGroup_Vtbl;

struct _ITuiRadioGroup {
    CONST ITuiRadioGroup_Vtbl *Vtbl;
};

// {A7B8C9D0-E1F2-4A3B-4C5D-6E7F8A9B0C1D}
DEFINE_GUID(IID_ITuiButton,
    0xA7B8C9D0, 0xE1F2, 0x4A3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D);

/**
  ITuiButton Interface

  Represents a clickable button widget.
**/
typedef struct _ITuiButton_Vtbl {
    //
    // IUnknown methods
    //
    HRESULT (ANXAPI *QueryInterface)(
        ITuiButton *This,
        REFIID riid,
        VOID **ppvObject
    );

    UINTN (ANXAPI *AddRef)(
        ITuiButton *This
    );

    UINTN (ANXAPI *Release)(
        ITuiButton *This
    );

    //
    // ITuiButton methods
    //

    /**
      Set button label.
    **/
    HRESULT (ANXAPI *SetLabel)(
        ITuiButton *This,
        CONST CHAR8 *Label
    );

    /**
      Set button callback.
    **/
    HRESULT (ANXAPI *SetCallback)(
        ITuiButton *This,
        HRESULT (*Callback)(VOID *UserData),
        VOID *UserData
    );

    /**
      Render button at position.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiButton *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y,
        BOOLEAN Focused
    );

    /**
      Handle key input (Enter activates button).
    **/
    HRESULT (ANXAPI *HandleKey)(
        ITuiButton *This,
        TUI_KEY Key,
        BOOLEAN *Handled
    );

} ITuiButton_Vtbl;

struct _ITuiButton {
    CONST ITuiButton_Vtbl *Vtbl;
};

// {B8C9D0E1-F2A3-4B4C-5D6E-7F8A9B0C1D2E}
DEFINE_GUID(IID_ITuiHelpViewer,
    0xB8C9D0E1, 0xF2A3, 0x4B4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E);

/**
  ITuiHelpViewer Interface

  Displays hyperlinked help text with navigation.
**/
typedef struct _ITuiHelpViewer_Vtbl {
    //
    // IUnknown methods
    //
    HRESULT (ANXAPI *QueryInterface)(
        ITuiHelpViewer *This,
        REFIID riid,
        VOID **ppvObject
    );

    UINTN (ANXAPI *AddRef)(
        ITuiHelpViewer *This
    );

    UINTN (ANXAPI *Release)(
        ITuiHelpViewer *This
    );

    //
    // ITuiHelpViewer methods
    //

    /**
      Set help text content (supports markup for links).
    **/
    HRESULT (ANXAPI *SetContent)(
        ITuiHelpViewer *This,
        CONST CHAR8 *Content
    );

    /**
      Add a hyperlink target.
    **/
    HRESULT (ANXAPI *AddLink)(
        ITuiHelpViewer *This,
        CONST CHAR8 *LinkId,
        CONST CHAR8 *Target
    );

    /**
      Show help viewer (modal dialog).
    **/
    HRESULT (ANXAPI *Show)(
        ITuiHelpViewer *This,
        ITuiScreen *Screen
    );

    /**
      Navigate to link.
    **/
    HRESULT (ANXAPI *NavigateTo)(
        ITuiHelpViewer *This,
        CONST CHAR8 *LinkId
    );

} ITuiHelpViewer_Vtbl;

struct _ITuiHelpViewer {
    CONST ITuiHelpViewer_Vtbl *Vtbl;
};

// {C9D0E1F2-A3B4-4C5D-6E7F-8A9B0C1D2E3F}
DEFINE_GUID(IID_ITuiListBox,
    0xC9D0E1F2, 0xA3B4, 0x4C5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F);

/**
  ITuiListBox Interface

  Scrollable list of selectable items.
**/
typedef struct _ITuiListBox_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiListBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiListBox *This);
    UINTN (ANXAPI *Release)(ITuiListBox *This);

    HRESULT (ANXAPI *AddItem)(ITuiListBox *This, CONST CHAR8 *Text, VOID *UserData);
    HRESULT (ANXAPI *RemoveItem)(ITuiListBox *This, UINT32 Index);
    HRESULT (ANXAPI *Clear)(ITuiListBox *This);
    HRESULT (ANXAPI *GetItemCount)(ITuiListBox *This, UINT32 *Count);
    HRESULT (ANXAPI *GetSelectedIndex)(ITuiListBox *This, INT32 *Index);
    HRESULT (ANXAPI *SetSelectedIndex)(ITuiListBox *This, INT32 Index);
    HRESULT (ANXAPI *GetItemText)(ITuiListBox *This, UINT32 Index, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *SetMultiSelect)(ITuiListBox *This, BOOLEAN Enabled);
    HRESULT (ANXAPI *Render)(ITuiListBox *This, ITuiScreen *Screen, INT32 X, INT32 Y, UINT32 Width, UINT32 Height, BOOLEAN Focused);
    HRESULT (ANXAPI *HandleInput)(ITuiListBox *This, CONST TUI_INPUT_EVENT *Event, BOOLEAN *Handled);
} ITuiListBox_Vtbl;

struct _ITuiListBox {
    CONST ITuiListBox_Vtbl *Vtbl;
};

// {D0E1F2A3-B4C5-4D6E-7F8A-9B0C1D2E3F4A}
DEFINE_GUID(IID_ITuiComboBox,
    0xD0E1F2A3, 0xB4C5, 0x4D6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A);

/**
  ITuiComboBox Interface

  Editable dropdown with autocomplete.
**/
typedef struct _ITuiComboBox_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiComboBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiComboBox *This);
    UINTN (ANXAPI *Release)(ITuiComboBox *This);

    HRESULT (ANXAPI *AddItem)(ITuiComboBox *This, CONST CHAR8 *Text);
    HRESULT (ANXAPI *GetText)(ITuiComboBox *This, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *SetText)(ITuiComboBox *This, CONST CHAR8 *Text);
    HRESULT (ANXAPI *SetEditable)(ITuiComboBox *This, BOOLEAN Editable);
    HRESULT (ANXAPI *SetAutocomplete)(ITuiComboBox *This, BOOLEAN Enabled);
    HRESULT (ANXAPI *Render)(ITuiComboBox *This, ITuiScreen *Screen, INT32 X, INT32 Y, UINT32 Width, BOOLEAN Focused);
    HRESULT (ANXAPI *HandleInput)(ITuiComboBox *This, CONST TUI_INPUT_EVENT *Event, BOOLEAN *Handled);
} ITuiComboBox_Vtbl;

struct _ITuiComboBox {
    CONST ITuiComboBox_Vtbl *Vtbl;
};

// {E1F2A3B4-C5D6-4E7F-8A9B-0C1D2E3F4A5B}
DEFINE_GUID(IID_ITuiDropDown,
    0xE1F2A3B4, 0xC5D6, 0x4E7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B);

/**
  ITuiDropDown Interface

  Dropdown menu (non-editable selection).
**/
typedef struct _ITuiDropDown_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiDropDown *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiDropDown *This);
    UINTN (ANXAPI *Release)(ITuiDropDown *This);

    HRESULT (ANXAPI *AddItem)(ITuiDropDown *This, CONST CHAR8 *Text, VOID *UserData);
    HRESULT (ANXAPI *GetSelectedIndex)(ITuiDropDown *This, INT32 *Index);
    HRESULT (ANXAPI *SetSelectedIndex)(ITuiDropDown *This, INT32 Index);
    HRESULT (ANXAPI *Render)(ITuiDropDown *This, ITuiScreen *Screen, INT32 X, INT32 Y, UINT32 Width, BOOLEAN Focused);
    HRESULT (ANXAPI *HandleInput)(ITuiDropDown *This, CONST TUI_INPUT_EVENT *Event, BOOLEAN *Handled);
} ITuiDropDown_Vtbl;

struct _ITuiDropDown {
    CONST ITuiDropDown_Vtbl *Vtbl;
};

// {F2A3B4C5-D6E7-4F8A-9B0C-1D2E3F4A5B6C}
DEFINE_GUID(IID_ITuiMenuBar,
    0xF2A3B4C5, 0xD6E7, 0x4F8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B, 0x6C);

/**
  ITuiMenuBar Interface

  Top menu bar with dropdown menus.
**/
typedef struct _ITuiMenuBar_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiMenuBar *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiMenuBar *This);
    UINTN (ANXAPI *Release)(ITuiMenuBar *This);

    HRESULT (ANXAPI *AddMenu)(ITuiMenuBar *This, CONST CHAR8 *Title, ITuiMenu *Menu);
    HRESULT (ANXAPI *SetHotkey)(ITuiMenuBar *This, UINT32 MenuIndex, CHAR8 Hotkey);
    HRESULT (ANXAPI *Render)(ITuiMenuBar *This, ITuiScreen *Screen);
    HRESULT (ANXAPI *HandleInput)(ITuiMenuBar *This, CONST TUI_INPUT_EVENT *Event, BOOLEAN *Handled);
} ITuiMenuBar_Vtbl;

struct _ITuiMenuBar {
    CONST ITuiMenuBar_Vtbl *Vtbl;
};

// {A3B4C5D6-E7F8-4A9B-0C1D-2E3F4A5B6C7D}
DEFINE_GUID(IID_ITuiStatusBar,
    0xA3B4C5D6, 0xE7F8, 0x4A9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B, 0x6C, 0x7D);

/**
  ITuiStatusBar Interface

  Bottom status bar with panels.
**/
typedef struct _ITuiStatusBar_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiStatusBar *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiStatusBar *This);
    UINTN (ANXAPI *Release)(ITuiStatusBar *This);

    HRESULT (ANXAPI *SetText)(ITuiStatusBar *This, UINT32 Panel, CONST CHAR8 *Text);
    HRESULT (ANXAPI *SetPanelCount)(ITuiStatusBar *This, UINT32 Count);
    HRESULT (ANXAPI *SetPanelWidth)(ITuiStatusBar *This, UINT32 Panel, UINT32 Width);
    HRESULT (ANXAPI *Render)(ITuiStatusBar *This, ITuiScreen *Screen);
} ITuiStatusBar_Vtbl;

struct _ITuiStatusBar {
    CONST ITuiStatusBar_Vtbl *Vtbl;
};

// {B4C5D6E7-F8A9-4B0C-1D2E-3F4A5B6C7D8E}
DEFINE_GUID(IID_ITuiDesktop,
    0xB4C5D6E7, 0xF8A9, 0x4B0C, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B, 0x6C, 0x7D, 0x8E);

/**
  ITuiDesktop Interface

  Window manager for TUI applications.
**/
typedef struct _ITuiDesktop_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiDesktop *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiDesktop *This);
    UINTN (ANXAPI *Release)(ITuiDesktop *This);

    HRESULT (ANXAPI *AddWindow)(ITuiDesktop *This, ITuiWindow *Window);
    HRESULT (ANXAPI *RemoveWindow)(ITuiDesktop *This, ITuiWindow *Window);
    HRESULT (ANXAPI *SetActiveWindow)(ITuiDesktop *This, ITuiWindow *Window);
    HRESULT (ANXAPI *GetActiveWindow)(ITuiDesktop *This, ITuiWindow **Window);
    HRESULT (ANXAPI *SetMenuBar)(ITuiDesktop *This, ITuiMenuBar *MenuBar);
    HRESULT (ANXAPI *SetStatusBar)(ITuiDesktop *This, ITuiStatusBar *StatusBar);
    HRESULT (ANXAPI *Render)(ITuiDesktop *This, ITuiScreen *Screen);
    HRESULT (ANXAPI *HandleInput)(ITuiDesktop *This, CONST TUI_INPUT_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *Run)(ITuiDesktop *This, ITuiScreen *Screen);  /* Main event loop */
} ITuiDesktop_Vtbl;

struct _ITuiDesktop {
    CONST ITuiDesktop_Vtbl *Vtbl;
};

// {C5D6E7F8-A9B0-4C1D-2E3F-4A5B6C7D8E9F}
DEFINE_GUID(IID_ITuiTheme,
    0xC5D6E7F8, 0xA9B0, 0x4C1D, 0x2E, 0x3F, 0x4A, 0x5B, 0x6C, 0x7D, 0x8E, 0x9F);

/**
  TUI Theme Components
**/
typedef enum _TUI_THEME_COMPONENT {
    TuiThemeMenuBar,
    TuiThemeMenu,
    TuiThemeMenuItem,
    TuiThemeMenuItemSelected,
    TuiThemeWindow,
    TuiThemeWindowBorder,
    TuiThemeWindowTitle,
    TuiThemeButton,
    TuiThemeButtonFocused,
    TuiThemeInput,
    TuiThemeInputFocused,
    TuiThemeCheckbox,
    TuiThemeCheckboxFocused,
    TuiThemeListBox,
    TuiThemeListBoxSelected,
    TuiThemeStatusBar,
    TuiThemeDesktop,
    TuiThemeSelection,
    TuiThemeDisabled
} TUI_THEME_COMPONENT;

/**
  TUI Theme Color Scheme
**/
typedef struct _TUI_THEME_COLORS {
    TUI_COLOR Foreground;
    TUI_COLOR Background;
    UINT32 Attributes;  /* Bold, underline, etc. */
} TUI_THEME_COLORS;

/**
  TUI Border Style
**/
typedef enum _TUI_BORDER_STYLE {
    TuiBorderNone,
    TuiBorderSingle,     /* ┌─┐ │ └─┘ */
    TuiBorderDouble,     /* ╔═╗ ║ ╚═╝ */
    TuiBorderRounded,    /* ╭─╮ │ ╰─╯ */
    TuiBorderAscii       /* +-+ | +-+ */
} TUI_BORDER_STYLE;

/**
  ITuiTheme Interface

  Customizable UI theme (colors, borders, styles).
**/
typedef struct _ITuiTheme_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiTheme *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiTheme *This);
    UINTN (ANXAPI *Release)(ITuiTheme *This);

    HRESULT (ANXAPI *SetColors)(ITuiTheme *This, TUI_THEME_COMPONENT Component, CONST TUI_THEME_COLORS *Colors);
    HRESULT (ANXAPI *GetColors)(ITuiTheme *This, TUI_THEME_COMPONENT Component, TUI_THEME_COLORS *Colors);
    HRESULT (ANXAPI *SetBorderStyle)(ITuiTheme *This, TUI_BORDER_STYLE Style);
    HRESULT (ANXAPI *GetBorderStyle)(ITuiTheme *This, TUI_BORDER_STYLE *Style);
    HRESULT (ANXAPI *LoadFromFile)(ITuiTheme *This, CONST CHAR8 *FilePath);
    HRESULT (ANXAPI *SaveToFile)(ITuiTheme *This, CONST CHAR8 *FilePath);
    HRESULT (ANXAPI *SetName)(ITuiTheme *This, CONST CHAR8 *Name);
    HRESULT (ANXAPI *GetName)(ITuiTheme *This, CHAR8 *Buffer, UINTN BufferSize);
} ITuiTheme_Vtbl;

struct _ITuiTheme {
    CONST ITuiTheme_Vtbl *Vtbl;
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

/**
  Create a TUI Checkbox instance.

  @param[out] Checkbox  Pointer to receive the checkbox interface.

  @retval S_OK        Checkbox created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateCheckbox(
    OUT ITuiCheckbox **Checkbox
);

/**
  Create a TUI Input Field instance.

  @param[in]  Type   Input field type (string/integer/hex).
  @param[out] Input  Pointer to receive the input field interface.

  @retval S_OK        Input field created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateInput(
    IN  TUI_INPUT_TYPE Type,
    OUT ITuiInput **Input
);

/**
  Create a TUI Radio Group instance.

  @param[out] RadioGroup  Pointer to receive the radio group interface.

  @retval S_OK        Radio group created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateRadioGroup(
    OUT ITuiRadioGroup **RadioGroup
);

/**
  Create a TUI Button instance.

  @param[in]  Label   Button label text.
  @param[out] Button  Pointer to receive the button interface.

  @retval S_OK        Button created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateButton(
    IN  CONST CHAR8 *Label,
    OUT ITuiButton **Button
);

/**
  Create a TUI Help Viewer instance.

  @param[out] HelpViewer  Pointer to receive the help viewer interface.

  @retval S_OK        Help viewer created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateHelpViewer(
    OUT ITuiHelpViewer **HelpViewer
);

/**
  Create a TUI List Box instance.

  @param[out] ListBox  Pointer to receive the list box interface.

  @retval S_OK        List box created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateListBox(
    OUT ITuiListBox **ListBox
);

/**
  Create a TUI Combo Box instance.

  @param[out] ComboBox  Pointer to receive the combo box interface.

  @retval S_OK        Combo box created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateComboBox(
    OUT ITuiComboBox **ComboBox
);

/**
  Create a TUI Drop Down instance.

  @param[out] DropDown  Pointer to receive the dropdown interface.

  @retval S_OK        Dropdown created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateDropDown(
    OUT ITuiDropDown **DropDown
);

/**
  Create a TUI Menu Bar instance.

  @param[out] MenuBar  Pointer to receive the menu bar interface.

  @retval S_OK        Menu bar created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateMenuBar(
    OUT ITuiMenuBar **MenuBar
);

/**
  Create a TUI Status Bar instance.

  @param[out] StatusBar  Pointer to receive the status bar interface.

  @retval S_OK        Status bar created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateStatusBar(
    OUT ITuiStatusBar **StatusBar
);

/**
  Create a TUI Desktop instance.

  @param[in]  Screen   Parent screen for the desktop.
  @param[out] Desktop  Pointer to receive the desktop interface.

  @retval S_OK        Desktop created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateDesktop(
    IN  ITuiScreen *Screen,
    OUT ITuiDesktop **Desktop
);

/**
  Create a TUI Theme instance.

  @param[out] Theme  Pointer to receive the theme interface.

  @retval S_OK        Theme created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateTheme(
    OUT ITuiTheme **Theme
);

/**
  Load a predefined theme by name.

  @param[in]  Name   Theme name ("Default", "Dark", "Light", "Classic", etc.).
  @param[out] Theme  Pointer to receive the theme interface.

  @retval S_OK        Theme loaded successfully.
  @retval E_NOTFOUND  Theme not found.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiLoadTheme(
    IN  CONST CHAR8 *Name,
    OUT ITuiTheme **Theme
);

#ifdef __cplusplus
}
#endif

#endif /* __ANANKE_TUI_H__ */
