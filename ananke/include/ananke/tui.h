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

// Base interfaces
typedef struct _ITuiSerializable ITuiSerializable;
typedef struct _ITuiResponder ITuiResponder;
typedef struct _ITuiWidget ITuiWidget;

// Universal resource and serialization system
typedef struct _IAmxSerializer IAmxSerializer;
typedef struct _IAmxResource IAmxResource;
typedef struct _IAmxResourceManager IAmxResourceManager;

// Event listener interfaces
typedef struct _ITuiDrawListener ITuiDrawListener;
typedef struct _ITuiKeyListener ITuiKeyListener;
typedef struct _ITuiMouseListener ITuiMouseListener;
typedef struct _ITuiTimerListener ITuiTimerListener;
typedef struct _ITuiNotificationListener ITuiNotificationListener;

// Window management
typedef struct _ITuiWindowManager ITuiWindowManager;
typedef struct _ITuiComposer ITuiComposer;

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
typedef struct _ITuiTabControl ITuiTabControl;
typedef struct _ITuiProgressBar ITuiProgressBar;
typedef struct _ITuiColorPicker ITuiColorPicker;
typedef struct _ITuiGroupBox ITuiGroupBox;
typedef struct _ITuiFocusManager ITuiFocusManager;
typedef struct _ITuiTextEditor ITuiTextEditor;
typedef struct _ITuiScrollView ITuiScrollView;
typedef struct _ITuiLabel ITuiLabel;
typedef struct _ITuiHyperlink ITuiHyperlink;
typedef struct _ITuiElastic ITuiElastic;
typedef struct _ITuiScrollBar ITuiScrollBar;
typedef struct _ITuiMessageBox ITuiMessageBox;
typedef struct _ITuiSlider ITuiSlider;
typedef struct _ITuiSpinner ITuiSpinner;
typedef struct _ITuiFKeyBar ITuiFKeyBar;
typedef struct _ITuiMarkdownViewer ITuiMarkdownViewer;
typedef struct _ITuiPrintDialog ITuiPrintDialog;
typedef struct _ITuiLongOpDialog ITuiLongOpDialog;
typedef struct _ITuiDirectoryDialog ITuiDirectoryDialog;
typedef struct _ITuiTerminal ITuiTerminal;
typedef struct _ITuiTreeView ITuiTreeView;
typedef struct _ITuiListView ITuiListView;
typedef struct _ITuiWizard ITuiWizard;
typedef struct _ITuiFlexContainer ITuiFlexContainer;
typedef struct _ITuiVBox ITuiVBox;
typedef struct _ITuiHBox ITuiHBox;
typedef struct _ITuiGrid ITuiGrid;
typedef struct _ITuiSplitView ITuiSplitView;
typedef struct _ITuiSurface ITuiSurface;
typedef struct _ITuiPropertySheet ITuiPropertySheet;
typedef struct _ITuiSpreadsheet ITuiSpreadsheet;
typedef struct _ITuiRuler ITuiRuler;
typedef struct _ITuiRichTextEditor ITuiRichTextEditor;
typedef struct _ITuiHeaderView ITuiHeaderView;

//
// Text Direction for BiDi Support
//
typedef enum _TUI_TEXT_DIRECTION {
    TuiTextLTR,         /* Left-to-right (Latin, Cyrillic, etc.) */
    TuiTextRTL,         /* Right-to-left (Hebrew, Arabic, etc.) */
    TuiTextAuto         /* Auto-detect based on content */
} TUI_TEXT_DIRECTION;

//
// Unicode Support Level
//
typedef enum _TUI_UNICODE_LEVEL {
    TuiUnicodeNone,     /* ASCII only */
    TuiUnicodeBasic,    /* Basic Multilingual Plane (BMP) */
    TuiUnicodeFull      /* Full Unicode with supplementary planes */
} TUI_UNICODE_LEVEL;

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

//
// Base Responder and Widget Architecture (Cocoa-style)
//

// {0A1B2C3D-4E5F-6A7B-8C9D-0E1F2A3B4C5D}
DEFINE_GUID(IID_ITuiSerializable,
    0x0A1B2C3D, 0x4E5F, 0x6A7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D);

/**
  ITuiSerializable Interface

  Base interface for objects that can be serialized/deserialized.
  Enables YAML persistence and universal resource storage/retrieval.
**/
typedef struct _ITuiSerializable_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiSerializable *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiSerializable *This);
    UINTN (ANXAPI *Release)(ITuiSerializable *This);

    /**
      Serialize object to YAML string.
    **/
    HRESULT (ANXAPI *SerializeToYaml)(
        ITuiSerializable *This,
        CHAR8 **OutYaml,
        UINTN *OutLength
    );

    /**
      Deserialize object from YAML string.
    **/
    HRESULT (ANXAPI *DeserializeFromYaml)(
        ITuiSerializable *This,
        CONST CHAR8 *Yaml,
        UINTN Length
    );

    /**
      Get object type name for serialization.
    **/
    HRESULT (ANXAPI *GetTypeName)(
        ITuiSerializable *This,
        CONST CHAR8 **OutTypeName
    );

    /**
      Clone the object.
    **/
    HRESULT (ANXAPI *Clone)(
        ITuiSerializable *This,
        ITuiSerializable **OutClone
    );
} ITuiSerializable_Vtbl;

struct _ITuiSerializable {
    CONST ITuiSerializable_Vtbl *Vtbl;
};

#ifdef COBJMACROS
#define ITuiSerializable_QueryInterface(This,riid,ppvObject) (This)->Vtbl->QueryInterface(This,riid,ppvObject)
#define ITuiSerializable_AddRef(This) (This)->Vtbl->AddRef(This)
#define ITuiSerializable_Release(This) (This)->Vtbl->Release(This)
#define ITuiSerializable_SerializeToYaml(This,OutYaml,OutLength) (This)->Vtbl->SerializeToYaml(This,OutYaml,OutLength)
#define ITuiSerializable_DeserializeFromYaml(This,Yaml,Length) (This)->Vtbl->DeserializeFromYaml(This,Yaml,Length)
#define ITuiSerializable_GetTypeName(This,OutTypeName) (This)->Vtbl->GetTypeName(This,OutTypeName)
#define ITuiSerializable_Clone(This,OutClone) (This)->Vtbl->Clone(This,OutClone)
#endif

//
// Universal Resource and Serialization System
//

// {A1B2C3D4-E5F6-7A8B-9C0D-1E2F3A4B5C6D}
DEFINE_GUID(IID_IAmxSerializer,
    0xA1B2C3D4, 0xE5F6, 0x7A8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D);

/**
  IAmxSerializer Interface

  Universal YAML serializer for all objects.
  Handles serialization/deserialization of arbitrary objects to/from YAML.
**/
typedef struct _IAmxSerializer_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(IAmxSerializer *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(IAmxSerializer *This);
    UINTN (ANXAPI *Release)(IAmxSerializer *This);

    /**
      Serialize an object to YAML string.
    **/
    HRESULT (ANXAPI *Serialize)(
        IAmxSerializer *This,
        ITuiSerializable *Object,
        CHAR8 **OutYaml,
        UINTN *OutLength
    );

    /**
      Deserialize YAML string to an object.
    **/
    HRESULT (ANXAPI *Deserialize)(
        IAmxSerializer *This,
        CONST CHAR8 *Yaml,
        UINTN Length,
        ITuiSerializable **OutObject
    );

    /**
      Serialize object to YAML file.
    **/
    HRESULT (ANXAPI *SerializeToFile)(
        IAmxSerializer *This,
        ITuiSerializable *Object,
        CONST CHAR8 *FilePath
    );

    /**
      Deserialize object from YAML file.
    **/
    HRESULT (ANXAPI *DeserializeFromFile)(
        IAmxSerializer *This,
        CONST CHAR8 *FilePath,
        ITuiSerializable **OutObject
    );

    /**
      Register a type factory for deserialization.
      Maps type names to factory functions.
    **/
    HRESULT (ANXAPI *RegisterTypeFactory)(
        IAmxSerializer *This,
        CONST CHAR8 *TypeName,
        HRESULT (*FactoryFunc)(ITuiSerializable **OutObject)
    );

    /**
      Validate YAML syntax.
    **/
    HRESULT (ANXAPI *ValidateYaml)(
        IAmxSerializer *This,
        CONST CHAR8 *Yaml,
        UINTN Length,
        BOOLEAN *IsValid,
        CHAR8 **ErrorMessage
    );
} IAmxSerializer_Vtbl;

struct _IAmxSerializer {
    CONST IAmxSerializer_Vtbl *Vtbl;
};

#ifdef COBJMACROS
#define IAmxSerializer_QueryInterface(This,riid,ppvObject) (This)->Vtbl->QueryInterface(This,riid,ppvObject)
#define IAmxSerializer_AddRef(This) (This)->Vtbl->AddRef(This)
#define IAmxSerializer_Release(This) (This)->Vtbl->Release(This)
#define IAmxSerializer_Serialize(This,Object,OutYaml,OutLength) (This)->Vtbl->Serialize(This,Object,OutYaml,OutLength)
#define IAmxSerializer_Deserialize(This,Yaml,Length,OutObject) (This)->Vtbl->Deserialize(This,Yaml,Length,OutObject)
#define IAmxSerializer_SerializeToFile(This,Object,FilePath) (This)->Vtbl->SerializeToFile(This,Object,FilePath)
#define IAmxSerializer_DeserializeFromFile(This,FilePath,OutObject) (This)->Vtbl->DeserializeFromFile(This,FilePath,OutObject)
#define IAmxSerializer_RegisterTypeFactory(This,TypeName,FactoryFunc) (This)->Vtbl->RegisterTypeFactory(This,TypeName,FactoryFunc)
#define IAmxSerializer_ValidateYaml(This,Yaml,Length,IsValid,ErrorMessage) (This)->Vtbl->ValidateYaml(This,Yaml,Length,IsValid,ErrorMessage)
#endif

// {B2C3D4E5-F6A7-8B9C-0D1E-2F3A4B5C6D7E}
DEFINE_GUID(IID_IAmxResource,
    0xB2C3D4E5, 0xF6A7, 0x8B9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E);

/**
  IAmxResource Interface

  Universal resource object.
  Represents any resource (widget, config, theme, etc.) that can be stored and retrieved.
**/
typedef struct _IAmxResource_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(IAmxResource *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(IAmxResource *This);
    UINTN (ANXAPI *Release)(IAmxResource *This);

    /**
      Get resource ID (URI).
    **/
    HRESULT (ANXAPI *GetId)(
        IAmxResource *This,
        CONST CHAR8 **OutId
    );

    /**
      Get resource type.
    **/
    HRESULT (ANXAPI *GetType)(
        IAmxResource *This,
        CONST CHAR8 **OutType
    );

    /**
      Get the underlying object (ITuiSerializable).
    **/
    HRESULT (ANXAPI *GetObject)(
        IAmxResource *This,
        ITuiSerializable **OutObject
    );

    /**
      Get resource metadata.
    **/
    HRESULT (ANXAPI *GetMetadata)(
        IAmxResource *This,
        CONST CHAR8 *Key,
        CONST CHAR8 **OutValue
    );

    /**
      Set resource metadata.
    **/
    HRESULT (ANXAPI *SetMetadata)(
        IAmxResource *This,
        CONST CHAR8 *Key,
        CONST CHAR8 *Value
    );
} IAmxResource_Vtbl;

struct _IAmxResource {
    CONST IAmxResource_Vtbl *Vtbl;
};

#ifdef COBJMACROS
#define IAmxResource_QueryInterface(This,riid,ppvObject) (This)->Vtbl->QueryInterface(This,riid,ppvObject)
#define IAmxResource_AddRef(This) (This)->Vtbl->AddRef(This)
#define IAmxResource_Release(This) (This)->Vtbl->Release(This)
#define IAmxResource_GetId(This,OutId) (This)->Vtbl->GetId(This,OutId)
#define IAmxResource_GetType(This,OutType) (This)->Vtbl->GetType(This,OutType)
#define IAmxResource_GetObject(This,OutObject) (This)->Vtbl->GetObject(This,OutObject)
#define IAmxResource_GetMetadata(This,Key,OutValue) (This)->Vtbl->GetMetadata(This,Key,OutValue)
#define IAmxResource_SetMetadata(This,Key,Value) (This)->Vtbl->SetMetadata(This,Key,Value)
#endif

// {C3D4E5F6-A7B8-9C0D-1E2F-3A4B5C6D7E8F}
DEFINE_GUID(IID_IAmxResourceManager,
    0xC3D4E5F6, 0xA7B8, 0x9C0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F);

/**
  IAmxResourceManager Interface

  Universal resource manager.
  Stores and retrieves resources by URI, with YAML persistence.
**/
typedef struct _IAmxResourceManager_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(IAmxResourceManager *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(IAmxResourceManager *This);
    UINTN (ANXAPI *Release)(IAmxResourceManager *This);

    /**
      Register a resource.
    **/
    HRESULT (ANXAPI *RegisterResource)(
        IAmxResourceManager *This,
        CONST CHAR8 *Uri,
        ITuiSerializable *Object,
        IAmxResource **OutResource
    );

    /**
      Get a resource by URI.
    **/
    HRESULT (ANXAPI *GetResource)(
        IAmxResourceManager *This,
        CONST CHAR8 *Uri,
        IAmxResource **OutResource
    );

    /**
      Remove a resource.
    **/
    HRESULT (ANXAPI *RemoveResource)(
        IAmxResourceManager *This,
        CONST CHAR8 *Uri
    );

    /**
      Load resources from YAML file.
    **/
    HRESULT (ANXAPI *LoadFromFile)(
        IAmxResourceManager *This,
        CONST CHAR8 *FilePath
    );

    /**
      Save resources to YAML file.
    **/
    HRESULT (ANXAPI *SaveToFile)(
        IAmxResourceManager *This,
        CONST CHAR8 *FilePath
    );

    /**
      Enumerate all resources.
    **/
    HRESULT (ANXAPI *EnumerateResources)(
        IAmxResourceManager *This,
        HRESULT (*Callback)(CONST CHAR8 *Uri, IAmxResource *Resource, VOID *UserData),
        VOID *UserData
    );

    /**
      Get serializer instance.
    **/
    HRESULT (ANXAPI *GetSerializer)(
        IAmxResourceManager *This,
        IAmxSerializer **OutSerializer
    );
} IAmxResourceManager_Vtbl;

struct _IAmxResourceManager {
    CONST IAmxResourceManager_Vtbl *Vtbl;
};

#ifdef COBJMACROS
#define IAmxResourceManager_QueryInterface(This,riid,ppvObject) (This)->Vtbl->QueryInterface(This,riid,ppvObject)
#define IAmxResourceManager_AddRef(This) (This)->Vtbl->AddRef(This)
#define IAmxResourceManager_Release(This) (This)->Vtbl->Release(This)
#define IAmxResourceManager_RegisterResource(This,Uri,Object,OutResource) (This)->Vtbl->RegisterResource(This,Uri,Object,OutResource)
#define IAmxResourceManager_GetResource(This,Uri,OutResource) (This)->Vtbl->GetResource(This,Uri,OutResource)
#define IAmxResourceManager_RemoveResource(This,Uri) (This)->Vtbl->RemoveResource(This,Uri)
#define IAmxResourceManager_LoadFromFile(This,FilePath) (This)->Vtbl->LoadFromFile(This,FilePath)
#define IAmxResourceManager_SaveToFile(This,FilePath) (This)->Vtbl->SaveToFile(This,FilePath)
#define IAmxResourceManager_EnumerateResources(This,Callback,UserData) (This)->Vtbl->EnumerateResources(This,Callback,UserData)
#define IAmxResourceManager_GetSerializer(This,OutSerializer) (This)->Vtbl->GetSerializer(This,OutSerializer)
#endif

// {1A2B3C4D-5E6F-7A8B-9C0D-1E2F3A4B5C6D}
DEFINE_GUID(IID_ITuiResponder,
    0x1A2B3C4D, 0x5E6F, 0x7A8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D);

/**
  ITuiResponder Interface

  Base interface for objects that respond to events.
  Inherits from ITuiSerializable. Similar to NSResponder in Cocoa - handles event dispatching.
**/
typedef struct _ITuiResponder_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiResponder *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiResponder *This);
    UINTN (ANXAPI *Release)(ITuiResponder *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiResponder *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiResponder *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiResponder *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiResponder *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    /**
      Get the next responder in the responder chain.
    **/
    HRESULT (ANXAPI *GetNextResponder)(
        ITuiResponder *This,
        ITuiResponder **NextResponder
    );

    /**
      Set the next responder in the responder chain.
    **/
    HRESULT (ANXAPI *SetNextResponder)(
        ITuiResponder *This,
        ITuiResponder *NextResponder
    );

    /**
      Check if this responder accepts first responder status.
    **/
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(
        ITuiResponder *This
    );

    /**
      Become the first responder.
    **/
    HRESULT (ANXAPI *BecomeFirstResponder)(
        ITuiResponder *This
    );

    /**
      Resign first responder status.
    **/
    HRESULT (ANXAPI *ResignFirstResponder)(
        ITuiResponder *This
    );
} ITuiResponder_Vtbl;

struct _ITuiResponder {
    CONST ITuiResponder_Vtbl *Vtbl;
};

#ifdef COBJMACROS
#define ITuiResponder_QueryInterface(This,riid,ppvObject) (This)->Vtbl->QueryInterface(This,riid,ppvObject)
#define ITuiResponder_AddRef(This) (This)->Vtbl->AddRef(This)
#define ITuiResponder_Release(This) (This)->Vtbl->Release(This)
#define ITuiResponder_SerializeToYaml(This,OutYaml,OutLength) (This)->Vtbl->SerializeToYaml(This,OutYaml,OutLength)
#define ITuiResponder_DeserializeFromYaml(This,Yaml,Length) (This)->Vtbl->DeserializeFromYaml(This,Yaml,Length)
#define ITuiResponder_GetTypeName(This,OutTypeName) (This)->Vtbl->GetTypeName(This,OutTypeName)
#define ITuiResponder_Clone(This,OutClone) (This)->Vtbl->Clone(This,OutClone)
#define ITuiResponder_GetNextResponder(This,NextResponder) (This)->Vtbl->GetNextResponder(This,NextResponder)
#define ITuiResponder_SetNextResponder(This,NextResponder) (This)->Vtbl->SetNextResponder(This,NextResponder)
#define ITuiResponder_AcceptsFirstResponder(This) (This)->Vtbl->AcceptsFirstResponder(This)
#define ITuiResponder_BecomeFirstResponder(This) (This)->Vtbl->BecomeFirstResponder(This)
#define ITuiResponder_ResignFirstResponder(This) (This)->Vtbl->ResignFirstResponder(This)
#endif

// {2B3C4D5E-6F7A-8B9C-0D1E-2F3A4B5C6D7E}
DEFINE_GUID(IID_ITuiWidget,
    0x2B3C4D5E, 0x6F7A, 0x8B9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E);

/**
  ITuiWidget Interface

  Base interface for all TUI widgets. Inherits from ITuiResponder.
  Provides common widget functionality (bounds, visibility, enable state).
**/
typedef struct _ITuiWidget_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiWidget *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiWidget *This);
    UINTN (ANXAPI *Release)(ITuiWidget *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiWidget *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiWidget *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiWidget *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiWidget *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiWidget *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiWidget *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiWidget *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiWidget *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiWidget *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiWidget *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiWidget *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiWidget *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiWidget *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiWidget *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiWidget *This);
    HRESULT (ANXAPI *SetParent)(ITuiWidget *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiWidget *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiWidget *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiWidget *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiWidget *This, BOOLEAN Needed);
} ITuiWidget_Vtbl;

struct _ITuiWidget {
    CONST ITuiWidget_Vtbl *Vtbl;
};

#ifdef COBJMACROS
#define ITuiWidget_QueryInterface(This,riid,ppvObject) (This)->Vtbl->QueryInterface(This,riid,ppvObject)
#define ITuiWidget_AddRef(This) (This)->Vtbl->AddRef(This)
#define ITuiWidget_Release(This) (This)->Vtbl->Release(This)
#define ITuiWidget_SerializeToYaml(This,OutYaml,OutLength) (This)->Vtbl->SerializeToYaml(This,OutYaml,OutLength)
#define ITuiWidget_DeserializeFromYaml(This,Yaml,Length) (This)->Vtbl->DeserializeFromYaml(This,Yaml,Length)
#define ITuiWidget_GetTypeName(This,OutTypeName) (This)->Vtbl->GetTypeName(This,OutTypeName)
#define ITuiWidget_Clone(This,OutClone) (This)->Vtbl->Clone(This,OutClone)
#define ITuiWidget_GetNextResponder(This,NextResponder) (This)->Vtbl->GetNextResponder(This,NextResponder)
#define ITuiWidget_SetNextResponder(This,NextResponder) (This)->Vtbl->SetNextResponder(This,NextResponder)
#define ITuiWidget_AcceptsFirstResponder(This) (This)->Vtbl->AcceptsFirstResponder(This)
#define ITuiWidget_BecomeFirstResponder(This) (This)->Vtbl->BecomeFirstResponder(This)
#define ITuiWidget_ResignFirstResponder(This) (This)->Vtbl->ResignFirstResponder(This)
#define ITuiWidget_SetBounds(This,Bounds) (This)->Vtbl->SetBounds(This,Bounds)
#define ITuiWidget_GetBounds(This,Bounds) (This)->Vtbl->GetBounds(This,Bounds)
#define ITuiWidget_SetVisible(This,Visible) (This)->Vtbl->SetVisible(This,Visible)
#define ITuiWidget_IsVisible(This) (This)->Vtbl->IsVisible(This)
#define ITuiWidget_SetEnabled(This,Enabled) (This)->Vtbl->SetEnabled(This,Enabled)
#define ITuiWidget_IsEnabled(This) (This)->Vtbl->IsEnabled(This)
#define ITuiWidget_SetParent(This,Parent) (This)->Vtbl->SetParent(This,Parent)
#define ITuiWidget_GetParent(This,Parent) (This)->Vtbl->GetParent(This,Parent)
#define ITuiWidget_AddChild(This,Child) (This)->Vtbl->AddChild(This,Child)
#define ITuiWidget_RemoveChild(This,Child) (This)->Vtbl->RemoveChild(This,Child)
#define ITuiWidget_SetNeedsDisplay(This,Needed) (This)->Vtbl->SetNeedsDisplay(This,Needed)
#endif

// {4E5F6A7B-8C9D-0E1F-2A3B-4C5D6E7F8A9B}
DEFINE_GUID(IID_ITuiThemedWidget,
    0x4E5F6A7B, 0x8C9D, 0x0E1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B);

/**
  ITuiThemedWidget Interface

  Base interface for all themed widgets. Inherits from ITuiWidget.
  Provides theming support for widgets.
**/
typedef struct _ITuiThemedWidget_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedWidget *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedWidget *This);
    UINTN (ANXAPI *Release)(ITuiThemedWidget *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedWidget *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedWidget *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedWidget *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedWidget *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedWidget *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedWidget *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedWidget *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedWidget *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedWidget *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedWidget *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedWidget *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedWidget *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedWidget *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedWidget *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedWidget *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedWidget *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedWidget *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedWidget *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedWidget *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedWidget *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    /**
      Apply a theme to this widget.
    **/
    HRESULT (ANXAPI *ApplyTheme)(
        ITuiThemedWidget *This,
        ITuiTheme *Theme
    );

    /**
      Get the current foreground and background colors.
    **/
    HRESULT (ANXAPI *GetColors)(
        ITuiThemedWidget *This,
        TUI_COLOR *Foreground,
        TUI_COLOR *Background
    );

    /**
      Set the foreground and background colors.
    **/
    HRESULT (ANXAPI *SetColors)(
        ITuiThemedWidget *This,
        TUI_COLOR Foreground,
        TUI_COLOR Background
    );

    /**
      Handle mouse events.
    **/
    HRESULT (ANXAPI *OnMouseEvent)(
        ITuiThemedWidget *This,
        CONST TUI_MOUSE_EVENT *Event,
        BOOLEAN *Handled
    );

    /**
      Handle key events.
    **/
    HRESULT (ANXAPI *OnKeyEvent)(
        ITuiThemedWidget *This,
        TUI_KEY Key,
        UINT32 Modifiers,
        BOOLEAN *Handled
    );

    /**
      Draw the widget.
    **/
    HRESULT (ANXAPI *Draw)(
        ITuiThemedWidget *This,
        ITuiSurface *Surface,
        CONST TUI_RECT *DirtyRect
    );
} ITuiThemedWidget_Vtbl;

struct _ITuiThemedWidget {
    CONST ITuiThemedWidget_Vtbl *Vtbl;
};

#ifdef COBJMACROS
#define ITuiThemedWidget_QueryInterface(This,riid,ppvObject) (This)->Vtbl->QueryInterface(This,riid,ppvObject)
#define ITuiThemedWidget_AddRef(This) (This)->Vtbl->AddRef(This)
#define ITuiThemedWidget_Release(This) (This)->Vtbl->Release(This)
#define ITuiThemedWidget_SerializeToYaml(This,OutYaml,OutLength) (This)->Vtbl->SerializeToYaml(This,OutYaml,OutLength)
#define ITuiThemedWidget_DeserializeFromYaml(This,Yaml,Length) (This)->Vtbl->DeserializeFromYaml(This,Yaml,Length)
#define ITuiThemedWidget_GetTypeName(This,OutTypeName) (This)->Vtbl->GetTypeName(This,OutTypeName)
#define ITuiThemedWidget_Clone(This,OutClone) (This)->Vtbl->Clone(This,OutClone)
#define ITuiThemedWidget_GetNextResponder(This,NextResponder) (This)->Vtbl->GetNextResponder(This,NextResponder)
#define ITuiThemedWidget_SetNextResponder(This,NextResponder) (This)->Vtbl->SetNextResponder(This,NextResponder)
#define ITuiThemedWidget_AcceptsFirstResponder(This) (This)->Vtbl->AcceptsFirstResponder(This)
#define ITuiThemedWidget_BecomeFirstResponder(This) (This)->Vtbl->BecomeFirstResponder(This)
#define ITuiThemedWidget_ResignFirstResponder(This) (This)->Vtbl->ResignFirstResponder(This)
#define ITuiThemedWidget_SetBounds(This,Bounds) (This)->Vtbl->SetBounds(This,Bounds)
#define ITuiThemedWidget_GetBounds(This,Bounds) (This)->Vtbl->GetBounds(This,Bounds)
#define ITuiThemedWidget_SetVisible(This,Visible) (This)->Vtbl->SetVisible(This,Visible)
#define ITuiThemedWidget_IsVisible(This) (This)->Vtbl->IsVisible(This)
#define ITuiThemedWidget_SetEnabled(This,Enabled) (This)->Vtbl->SetEnabled(This,Enabled)
#define ITuiThemedWidget_IsEnabled(This) (This)->Vtbl->IsEnabled(This)
#define ITuiThemedWidget_SetParent(This,Parent) (This)->Vtbl->SetParent(This,Parent)
#define ITuiThemedWidget_GetParent(This,Parent) (This)->Vtbl->GetParent(This,Parent)
#define ITuiThemedWidget_AddChild(This,Child) (This)->Vtbl->AddChild(This,Child)
#define ITuiThemedWidget_RemoveChild(This,Child) (This)->Vtbl->RemoveChild(This,Child)
#define ITuiThemedWidget_SetNeedsDisplay(This,Needed) (This)->Vtbl->SetNeedsDisplay(This,Needed)
#define ITuiThemedWidget_ApplyTheme(This,Theme) (This)->Vtbl->ApplyTheme(This,Theme)
#define ITuiThemedWidget_GetColors(This,Foreground,Background) (This)->Vtbl->GetColors(This,Foreground,Background)
#define ITuiThemedWidget_SetColors(This,Foreground,Background) (This)->Vtbl->SetColors(This,Foreground,Background)
#define ITuiThemedWidget_OnMouseEvent(This,Event,Handled) (This)->Vtbl->OnMouseEvent(This,Event,Handled)
#define ITuiThemedWidget_OnKeyEvent(This,Key,Modifiers,Handled) (This)->Vtbl->OnKeyEvent(This,Key,Modifiers,Handled)
#define ITuiThemedWidget_Draw(This,Surface,DirtyRect) (This)->Vtbl->Draw(This,Surface,DirtyRect)
#endif

//
// Event Listener Interfaces
//

// {3C4D5E6F-7A8B-9C0D-1E2F-3A4B5C6D7E8F}
DEFINE_GUID(IID_ITuiDrawListener,
    0x3C4D5E6F, 0x7A8B, 0x9C0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F);

/**
  ITuiDrawListener Interface

  Listener for draw events. Widgets implement this to receive drawing requests.
**/
typedef struct _ITuiDrawListener_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiDrawListener *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiDrawListener *This);
    UINTN (ANXAPI *Release)(ITuiDrawListener *This);

    /**
      Called when the widget needs to be drawn.
    **/
    HRESULT (ANXAPI *OnDraw)(
        ITuiDrawListener *This,
        ITuiSurface *Surface,
        CONST TUI_RECT *DirtyRect
    );

    /**
      Called to get the widget's preferred size.
    **/
    HRESULT (ANXAPI *OnGetPreferredSize)(
        ITuiDrawListener *This,
        UINT32 *Width,
        UINT32 *Height
    );
} ITuiDrawListener_Vtbl;

struct _ITuiDrawListener {
    CONST ITuiDrawListener_Vtbl *Vtbl;
};

// {4D5E6F7A-8B9C-0D1E-2F3A-4B5C6D7E8F9A}
DEFINE_GUID(IID_ITuiKeyListener,
    0x4D5E6F7A, 0x8B9C, 0x0D1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A);

/**
  ITuiKeyListener Interface

  Listener for keyboard events.
**/
typedef struct _ITuiKeyListener_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiKeyListener *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiKeyListener *This);
    UINTN (ANXAPI *Release)(ITuiKeyListener *This);

    /**
      Called when a key is pressed.
    **/
    HRESULT (ANXAPI *OnKeyDown)(
        ITuiKeyListener *This,
        TUI_KEY Key,
        UINT32 Modifiers,
        BOOLEAN *Handled
    );

    /**
      Called when a key is released.
    **/
    HRESULT (ANXAPI *OnKeyUp)(
        ITuiKeyListener *This,
        TUI_KEY Key,
        UINT32 Modifiers,
        BOOLEAN *Handled
    );

    /**
      Called when a character is typed.
    **/
    HRESULT (ANXAPI *OnChar)(
        ITuiKeyListener *This,
        CHAR16 Character,
        BOOLEAN *Handled
    );
} ITuiKeyListener_Vtbl;

struct _ITuiKeyListener {
    CONST ITuiKeyListener_Vtbl *Vtbl;
};

// {5E6F7A8B-9C0D-1E2F-3A4B-5C6D7E8F9A0B}
DEFINE_GUID(IID_ITuiMouseListener,
    0x5E6F7A8B, 0x9C0D, 0x1E2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B);

/**
  ITuiMouseListener Interface

  Listener for mouse events.
**/
typedef struct _ITuiMouseListener_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiMouseListener *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiMouseListener *This);
    UINTN (ANXAPI *Release)(ITuiMouseListener *This);

    /**
      Called when mouse button is pressed.
    **/
    HRESULT (ANXAPI *OnMouseDown)(
        ITuiMouseListener *This,
        CONST TUI_MOUSE_EVENT *Event,
        BOOLEAN *Handled
    );

    /**
      Called when mouse button is released.
    **/
    HRESULT (ANXAPI *OnMouseUp)(
        ITuiMouseListener *This,
        CONST TUI_MOUSE_EVENT *Event,
        BOOLEAN *Handled
    );

    /**
      Called when mouse is moved.
    **/
    HRESULT (ANXAPI *OnMouseMove)(
        ITuiMouseListener *This,
        CONST TUI_MOUSE_EVENT *Event,
        BOOLEAN *Handled
    );

    /**
      Called when mouse is dragged.
    **/
    HRESULT (ANXAPI *OnMouseDrag)(
        ITuiMouseListener *This,
        CONST TUI_MOUSE_EVENT *Event,
        BOOLEAN *Handled
    );

    /**
      Called when mouse enters widget.
    **/
    HRESULT (ANXAPI *OnMouseEnter)(
        ITuiMouseListener *This,
        CONST TUI_MOUSE_EVENT *Event
    );

    /**
      Called when mouse leaves widget.
    **/
    HRESULT (ANXAPI *OnMouseLeave)(
        ITuiMouseListener *This,
        CONST TUI_MOUSE_EVENT *Event
    );
} ITuiMouseListener_Vtbl;

struct _ITuiMouseListener {
    CONST ITuiMouseListener_Vtbl *Vtbl;
};

// {6F7A8B9C-0D1E-2F3A-4B5C-6D7E8F9A0B1C}
DEFINE_GUID(IID_ITuiTimerListener,
    0x6F7A8B9C, 0x0D1E, 0x2F3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C);

/**
  ITuiTimerListener Interface

  Listener for timer events.
**/
typedef struct _ITuiTimerListener_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiTimerListener *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiTimerListener *This);
    UINTN (ANXAPI *Release)(ITuiTimerListener *This);

    /**
      Called when a timer fires.
    **/
    HRESULT (ANXAPI *OnTimer)(
        ITuiTimerListener *This,
        UINT32 TimerId,
        VOID *UserData
    );
} ITuiTimerListener_Vtbl;

struct _ITuiTimerListener {
    CONST ITuiTimerListener_Vtbl *Vtbl;
};

// {7A8B9C0D-1E2F-3A4B-5C6D-7E8F9A0B1C2D}
DEFINE_GUID(IID_ITuiNotificationListener,
    0x7A8B9C0D, 0x1E2F, 0x3A4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D);

/**
  ITuiNotificationListener Interface

  Listener for system notifications and widget events.
**/
typedef struct _ITuiNotificationListener_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiNotificationListener *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiNotificationListener *This);
    UINTN (ANXAPI *Release)(ITuiNotificationListener *This);

    /**
      Called when a notification is posted.
    **/
    HRESULT (ANXAPI *OnNotification)(
        ITuiNotificationListener *This,
        CONST CHAR8 *NotificationName,
        VOID *UserInfo
    );
} ITuiNotificationListener_Vtbl;

struct _ITuiNotificationListener {
    CONST ITuiNotificationListener_Vtbl *Vtbl;
};

// {8B9C0D1E-2F3A-4B5C-6D7E-8F9A0B1C2D3E}
DEFINE_GUID(IID_ITuiComposer,
    0x8B9C0D1E, 0x2F3A, 0x4B5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E);

/**
  ITuiComposer Interface

  Manages drawing surfaces and composites them to the screen.
  Replaces direct screen access - widgets draw to surfaces instead.
**/
typedef struct _ITuiComposer_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiComposer *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiComposer *This);
    UINTN (ANXAPI *Release)(ITuiComposer *This);

    /**
      Create a new drawing surface.
    **/
    HRESULT (ANXAPI *CreateSurface)(
        ITuiComposer *This,
        UINT32 Width,
        UINT32 Height,
        ITuiSurface **OutSurface
    );

    /**
      Composite all dirty surfaces to the screen.
    **/
    HRESULT (ANXAPI *Composite)(
        ITuiComposer *This,
        ITuiScreen *Screen
    );

    /**
      Mark a region as needing redraw.
    **/
    HRESULT (ANXAPI *MarkDirty)(
        ITuiComposer *This,
        CONST TUI_RECT *Rect
    );

    /**
      Register a widget's surface.
    **/
    HRESULT (ANXAPI *RegisterSurface)(
        ITuiComposer *This,
        ITuiWidget *Widget,
        ITuiSurface *Surface,
        INT32 ZOrder
    );

    /**
      Unregister a widget's surface.
    **/
    HRESULT (ANXAPI *UnregisterSurface)(
        ITuiComposer *This,
        ITuiWidget *Widget
    );
} ITuiComposer_Vtbl;

struct _ITuiComposer {
    CONST ITuiComposer_Vtbl *Vtbl;
};

// {9C0D1E2F-3A4B-5C6D-7E8F-9A0B1C2D3E4F}
DEFINE_GUID(IID_ITuiWindowManager,
    0x9C0D1E2F, 0x3A4B, 0x5C6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F);

/**
  ITuiWindowManager Interface

  Manages windows and routes events using QueryInterface to determine
  which listener interfaces widgets support.
**/
typedef struct _ITuiWindowManager_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiWindowManager *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiWindowManager *This);
    UINTN (ANXAPI *Release)(ITuiWindowManager *This);

    /**
      Register a window with the window manager.
    **/
    HRESULT (ANXAPI *RegisterWindow)(
        ITuiWindowManager *This,
        ITuiWindow *Window,
        ITuiWidget *RootWidget
    );

    /**
      Unregister a window.
    **/
    HRESULT (ANXAPI *UnregisterWindow)(
        ITuiWindowManager *This,
        ITuiWindow *Window
    );

    /**
      Get the currently focused window.
    **/
    HRESULT (ANXAPI *GetFocusedWindow)(
        ITuiWindowManager *This,
        ITuiWindow **Window
    );

    /**
      Set the focused window.
    **/
    HRESULT (ANXAPI *SetFocusedWindow)(
        ITuiWindowManager *This,
        ITuiWindow *Window
    );

    /**
      Dispatch an event to the appropriate widget.
      Uses QueryInterface to check for listener interfaces.
    **/
    HRESULT (ANXAPI *DispatchEvent)(
        ITuiWindowManager *This,
        CONST VOID *Event,
        REFIID EventType
    );

    /**
      Process all pending events.
    **/
    HRESULT (ANXAPI *ProcessEvents)(
        ITuiWindowManager *This
    );

    /**
      Get the first responder (focused widget).
    **/
    HRESULT (ANXAPI *GetFirstResponder)(
        ITuiWindowManager *This,
        ITuiResponder **Responder
    );

    /**
      Set the first responder.
    **/
    HRESULT (ANXAPI *SetFirstResponder)(
        ITuiWindowManager *This,
        ITuiResponder *Responder
    );
} ITuiWindowManager_Vtbl;

struct _ITuiWindowManager {
    CONST ITuiWindowManager_Vtbl *Vtbl;
};

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

    /**
      Set Unicode support level.
    **/
    HRESULT (ANXAPI *SetUnicodeLevel)(
        ITuiScreen *This,
        TUI_UNICODE_LEVEL Level
    );

    /**
      Get Unicode support level.
    **/
    HRESULT (ANXAPI *GetUnicodeLevel)(
        ITuiScreen *This,
        TUI_UNICODE_LEVEL *Level
    );

    /**
      Set default text direction.
    **/
    HRESULT (ANXAPI *SetTextDirection)(
        ITuiScreen *This,
        TUI_TEXT_DIRECTION Direction
    );

    /**
      Get default text direction.
    **/
    HRESULT (ANXAPI *GetTextDirection)(
        ITuiScreen *This,
        TUI_TEXT_DIRECTION *Direction
    );

    /**
      Write Unicode text with optional BiDi override.
    **/
    HRESULT (ANXAPI *WriteTextUnicode)(
        ITuiScreen *This,
        INT32 X,
        INT32 Y,
        CONST CHAR16 *Text,
        TUI_COLOR Foreground,
        TUI_COLOR Background,
        TUI_TEXT_DIRECTION Direction
    );

    /**
      Write UTF-8 text with BiDi support.
    **/
    HRESULT (ANXAPI *WriteTextUTF8)(
        ITuiScreen *This,
        INT32 X,
        INT32 Y,
        CONST CHAR8 *Text,
        TUI_COLOR Foreground,
        TUI_COLOR Background,
        TUI_TEXT_DIRECTION Direction
    );

    /**
      Get text width in columns (accounting for wide chars).
    **/
    HRESULT (ANXAPI *GetTextWidth)(
        ITuiScreen *This,
        CONST CHAR8 *Text,
        UINT32 *Width
    );

} ITuiScreen_Vtbl;

struct _ITuiScreen {
    CONST ITuiScreen_Vtbl *Vtbl;
};

/**
  Window State
**/
typedef enum _TUI_WINDOW_STATE {
    TuiWindowNormal,      /* Normal state */
    TuiWindowMinimized,   /* Minimized to taskbar/title only */
    TuiWindowMaximized,   /* Maximized to full screen */
    TuiWindowFolded       /* Folded (title bar only, collapsible) */
} TUI_WINDOW_STATE;

/**
  Window Flags
**/
typedef enum _TUI_WINDOW_FLAGS {
    TuiWindowResizable   = 0x0001,  /* Window can be resized */
    TuiWindowDraggable   = 0x0002,  /* Window can be moved */
    TuiWindowMinimizable = 0x0004,  /* Window can be minimized */
    TuiWindowMaximizable = 0x0008,  /* Window can be maximized */
    TuiWindowFoldable    = 0x0010,  /* Window can be folded */
    TuiWindowClosable    = 0x0020,  /* Window has close button */
    TuiWindowModal       = 0x0040,  /* Modal window (blocks others) */
    TuiWindowTopmost     = 0x0080   /* Always on top */
} TUI_WINDOW_FLAGS;

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

    /**
      Set window title.
    **/
    HRESULT (ANXAPI *SetTitle)(
        ITuiWindow *This,
        CONST CHAR8 *Title
    );

    /**
      Get window title.
    **/
    HRESULT (ANXAPI *GetTitle)(
        ITuiWindow *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Set window flags (resizable, draggable, etc.).
    **/
    HRESULT (ANXAPI *SetFlags)(
        ITuiWindow *This,
        UINT32 Flags
    );

    /**
      Get window flags.
    **/
    HRESULT (ANXAPI *GetFlags)(
        ITuiWindow *This,
        UINT32 *Flags
    );

    /**
      Set window state (normal, minimized, maximized, folded).
    **/
    HRESULT (ANXAPI *SetState)(
        ITuiWindow *This,
        TUI_WINDOW_STATE State
    );

    /**
      Get window state.
    **/
    HRESULT (ANXAPI *GetState)(
        ITuiWindow *This,
        TUI_WINDOW_STATE *State
    );

    /**
      Set window border style.
    **/
    HRESULT (ANXAPI *SetBorderStyle)(
        ITuiWindow *This,
        TUI_BORDER_STYLE Style
    );

    /**
      Start interactive resize mode (user drags edge).
    **/
    HRESULT (ANXAPI *BeginResize)(
        ITuiWindow *This
    );

    /**
      Start interactive move mode (user drags window).
    **/
    HRESULT (ANXAPI *BeginMove)(
        ITuiWindow *This
    );

    /**
      Minimize window.
    **/
    HRESULT (ANXAPI *Minimize)(
        ITuiWindow *This
    );

    /**
      Maximize window.
    **/
    HRESULT (ANXAPI *Maximize)(
        ITuiWindow *This
    );

    /**
      Restore window to normal state.
    **/
    HRESULT (ANXAPI *Restore)(
        ITuiWindow *This
    );

    /**
      Fold/unfold window (collapse to title bar only).
    **/
    HRESULT (ANXAPI *Fold)(
        ITuiWindow *This,
        BOOLEAN Folded
    );

    /**
      Bring window to front.
    **/
    HRESULT (ANXAPI *BringToFront)(
        ITuiWindow *This
    );

    /**
      Send window to back.
    **/
    HRESULT (ANXAPI *SendToBack)(
        ITuiWindow *This
    );

    /**
      Handle mouse event (for dragging, resizing, etc.).
      Returns TRUE if event was handled.
    **/
    HRESULT (ANXAPI *HandleMouse)(
        ITuiWindow *This,
        CONST TUI_MOUSE_EVENT *Event,
        BOOLEAN *Handled
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

    /**
      Set hotkey for menu item (e.g., 'F' for "File").
      Pressing Alt+F will activate this item.
    **/
    HRESULT (ANXAPI *SetItemHotkey)(
        ITuiMenu *This,
        INT32 Index,
        CHAR8 Hotkey
    );

    /**
      Set accelerator key for menu item (e.g., Ctrl+S for Save).
    **/
    HRESULT (ANXAPI *SetItemAccelerator)(
        ITuiMenu *This,
        INT32 Index,
        TUI_KEY Key,
        BOOLEAN Ctrl,
        BOOLEAN Alt,
        BOOLEAN Shift
    );

    /**
      Get item hotkey.
    **/
    HRESULT (ANXAPI *GetItemHotkey)(
        ITuiMenu *This,
        INT32 Index,
        CHAR8 *Hotkey
    );

    /**
      Enable/disable menu item.
    **/
    HRESULT (ANXAPI *SetItemEnabled)(
        ITuiMenu *This,
        INT32 Index,
        BOOLEAN Enabled
    );

    /**
      Check/uncheck menu item (for toggle items).
    **/
    HRESULT (ANXAPI *SetItemChecked)(
        ITuiMenu *This,
        INT32 Index,
        BOOLEAN Checked
    );

} ITuiMenu_Vtbl;

struct _ITuiMenu {
    CONST ITuiMenu_Vtbl *Vtbl;
};

// {7B8C9D0E-1F2A-3B4C-5D6E-7F8A9B0C1D2E}
DEFINE_GUID(IID_ITuiThemedCheckbox,
    0x7B8C9D0E, 0x1F2A, 0x3B4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E);

/**
  ITuiThemedCheckbox Interface

  Checkbox theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedCheckbox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedCheckbox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedCheckbox *This);
    UINTN (ANXAPI *Release)(ITuiThemedCheckbox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedCheckbox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedCheckbox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedCheckbox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedCheckbox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedCheckbox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedCheckbox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedCheckbox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedCheckbox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedCheckbox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedCheckbox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedCheckbox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedCheckbox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedCheckbox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedCheckbox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedCheckbox *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedCheckbox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedCheckbox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedCheckbox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedCheckbox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedCheckbox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedCheckbox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedCheckbox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedCheckbox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedCheckbox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedCheckbox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedCheckbox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedCheckbox methods
    /**
      Get checkbox character style ([ ], [X], etc).
    **/
    HRESULT (ANXAPI *GetCheckboxStyle)(
        ITuiThemedCheckbox *This,
        CHAR8 *UncheckedChar,
        CHAR8 *CheckedChar,
        CHAR8 *TristateChar
    );

    /**
      Set checkbox character style.
    **/
    HRESULT (ANXAPI *SetCheckboxStyle)(
        ITuiThemedCheckbox *This,
        CHAR8 UncheckedChar,
        CHAR8 CheckedChar,
        CHAR8 TristateChar
    );
} ITuiThemedCheckbox_Vtbl;

struct _ITuiThemedCheckbox {
    CONST ITuiThemedCheckbox_Vtbl *Vtbl;
};

// {D4E5F6A7-B8C9-4D0E-1F2A-3B4C5D6E7F8A}
DEFINE_GUID(IID_ITuiCheckbox,
    0xD4E5F6A7, 0xB8C9, 0x4D0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A);

/**
  ITuiCheckbox Interface

  Checkbox widget with tristate support. Inherits from ITuiThemedCheckbox.
**/
typedef struct _ITuiCheckbox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiCheckbox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiCheckbox *This);
    UINTN (ANXAPI *Release)(ITuiCheckbox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiCheckbox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiCheckbox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiCheckbox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiCheckbox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiCheckbox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiCheckbox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiCheckbox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiCheckbox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiCheckbox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiCheckbox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiCheckbox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiCheckbox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiCheckbox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiCheckbox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiCheckbox *This);
    HRESULT (ANXAPI *SetParent)(ITuiCheckbox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiCheckbox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiCheckbox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiCheckbox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiCheckbox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiCheckbox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiCheckbox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiCheckbox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiCheckbox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiCheckbox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiCheckbox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedCheckbox methods
    HRESULT (ANXAPI *GetCheckboxStyle)(ITuiCheckbox *This, CHAR8 *UncheckedChar, CHAR8 *CheckedChar, CHAR8 *TristateChar);
    HRESULT (ANXAPI *SetCheckboxStyle)(ITuiCheckbox *This, CHAR8 UncheckedChar, CHAR8 CheckedChar, CHAR8 TristateChar);

    // ITuiCheckbox methods
    /**
      Set checkbox label.
    **/
    HRESULT (ANXAPI *SetLabel)(
        ITuiCheckbox *This,
        CONST CHAR8 *Label
    );

    /**
      Get checkbox label.
    **/
    HRESULT (ANXAPI *GetLabel)(
        ITuiCheckbox *This,
        CHAR8 *Buffer,
        UINTN BufferSize
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
      Check if tristate mode is enabled.
    **/
    BOOLEAN (ANXAPI *IsTristate)(
        ITuiCheckbox *This
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
} ITuiCheckbox_Vtbl;

struct _ITuiCheckbox {
    CONST ITuiCheckbox_Vtbl *Vtbl;
};

/**
  Input Field Type
**/
typedef enum _TUI_INPUT_TYPE {
    TuiInputString,
    TuiInputInteger,
    TuiInputHex
} TUI_INPUT_TYPE;

// {8C9D0E1F-2A3B-4C5D-6E7F-8A9B0C1D2E3F}
DEFINE_GUID(IID_ITuiThemedInput,
    0x8C9D0E1F, 0x2A3B, 0x4C5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F);

/**
  ITuiThemedInput Interface

  Input field theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedInput_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedInput *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedInput *This);
    UINTN (ANXAPI *Release)(ITuiThemedInput *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedInput *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedInput *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedInput *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedInput *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedInput *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedInput *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedInput *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedInput *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedInput *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedInput *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedInput *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedInput *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedInput *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedInput *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedInput *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedInput *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedInput *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedInput *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedInput *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedInput *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedInput *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedInput *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedInput *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedInput *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedInput *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedInput *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedInput methods
    /**
      Get cursor style and color.
    **/
    HRESULT (ANXAPI *GetCursorStyle)(
        ITuiThemedInput *This,
        TUI_COLOR *CursorColor,
        BOOLEAN *BlockCursor
    );

    /**
      Set cursor style and color.
    **/
    HRESULT (ANXAPI *SetCursorStyle)(
        ITuiThemedInput *This,
        TUI_COLOR CursorColor,
        BOOLEAN BlockCursor
    );

    /**
      Get selection colors.
    **/
    HRESULT (ANXAPI *GetSelectionColors)(
        ITuiThemedInput *This,
        TUI_COLOR *Foreground,
        TUI_COLOR *Background
    );

    /**
      Set selection colors.
    **/
    HRESULT (ANXAPI *SetSelectionColors)(
        ITuiThemedInput *This,
        TUI_COLOR Foreground,
        TUI_COLOR Background
    );
} ITuiThemedInput_Vtbl;

struct _ITuiThemedInput {
    CONST ITuiThemedInput_Vtbl *Vtbl;
};

// {E5F6A7B8-C9D0-4E1F-2A3B-4C5D6E7F8A9B}
DEFINE_GUID(IID_ITuiInput,
    0xE5F6A7B8, 0xC9D0, 0x4E1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B);

/**
  ITuiInput Interface

  Text input field widget (string, integer, hex). Inherits from ITuiThemedInput.
**/
typedef struct _ITuiInput_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiInput *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiInput *This);
    UINTN (ANXAPI *Release)(ITuiInput *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiInput *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiInput *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiInput *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiInput *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiInput *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiInput *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiInput *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiInput *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiInput *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiInput *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiInput *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiInput *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiInput *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiInput *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiInput *This);
    HRESULT (ANXAPI *SetParent)(ITuiInput *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiInput *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiInput *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiInput *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiInput *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiInput *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiInput *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiInput *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiInput *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiInput *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiInput *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedInput methods
    HRESULT (ANXAPI *GetCursorStyle)(ITuiInput *This, TUI_COLOR *CursorColor, BOOLEAN *BlockCursor);
    HRESULT (ANXAPI *SetCursorStyle)(ITuiInput *This, TUI_COLOR CursorColor, BOOLEAN BlockCursor);
    HRESULT (ANXAPI *GetSelectionColors)(ITuiInput *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectionColors)(ITuiInput *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiInput methods
    /**
      Get input type.
    **/
    TUI_INPUT_TYPE (ANXAPI *GetType)(
        ITuiInput *This
    );

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
      Get input label.
    **/
    HRESULT (ANXAPI *GetLabel)(
        ITuiInput *This,
        CHAR8 *Buffer,
        UINTN BufferSize
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
      Get integer range.
    **/
    HRESULT (ANXAPI *GetRange)(
        ITuiInput *This,
        INT64 *Min,
        INT64 *Max
    );

    /**
      Set maximum input length.
    **/
    HRESULT (ANXAPI *SetMaxLength)(
        ITuiInput *This,
        UINTN MaxLength
    );

    /**
      Set password mode (display asterisks).
    **/
    HRESULT (ANXAPI *SetPasswordMode)(
        ITuiInput *This,
        BOOLEAN PasswordMode
    );

    /**
      Check if password mode is enabled.
    **/
    BOOLEAN (ANXAPI *IsPasswordMode)(
        ITuiInput *This
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

// {6A7B8C9D-0E1F-2A3B-4C5D-6E7F8A9B0C1D}
DEFINE_GUID(IID_ITuiThemedButton,
    0x6A7B8C9D, 0x0E1F, 0x2A3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D);

/**
  ITuiThemedButton Interface

  Button theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedButton_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedButton *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedButton *This);
    UINTN (ANXAPI *Release)(ITuiThemedButton *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedButton *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedButton *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedButton *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedButton *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedButton *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedButton *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedButton *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedButton *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedButton *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedButton *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedButton *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedButton *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedButton *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedButton *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedButton *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedButton *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedButton *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedButton *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedButton *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedButton *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedButton *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedButton *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedButton *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedButton *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedButton *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedButton *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedButton methods
    /**
      Get button style (border, flat, 3D, etc).
    **/
    HRESULT (ANXAPI *GetButtonStyle)(
        ITuiThemedButton *This,
        TUI_BORDER_STYLE *Style
    );

    /**
      Set button style (border, flat, 3D, etc).
    **/
    HRESULT (ANXAPI *SetButtonStyle)(
        ITuiThemedButton *This,
        TUI_BORDER_STYLE Style
    );

    /**
      Get focused/highlighted colors.
    **/
    HRESULT (ANXAPI *GetFocusedColors)(
        ITuiThemedButton *This,
        TUI_COLOR *Foreground,
        TUI_COLOR *Background
    );

    /**
      Set focused/highlighted colors.
    **/
    HRESULT (ANXAPI *SetFocusedColors)(
        ITuiThemedButton *This,
        TUI_COLOR Foreground,
        TUI_COLOR Background
    );
} ITuiThemedButton_Vtbl;

struct _ITuiThemedButton {
    CONST ITuiThemedButton_Vtbl *Vtbl;
};

// {A7B8C9D0-E1F2-4A3B-4C5D-6E7F8A9B0C1D}
DEFINE_GUID(IID_ITuiButton,
    0xA7B8C9D0, 0xE1F2, 0x4A3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D);

/**
  ITuiButton Interface

  Clickable button widget. Inherits from ITuiThemedButton.
**/
typedef struct _ITuiButton_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiButton *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiButton *This);
    UINTN (ANXAPI *Release)(ITuiButton *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiButton *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiButton *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiButton *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiButton *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiButton *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiButton *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiButton *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiButton *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiButton *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiButton *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiButton *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiButton *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiButton *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiButton *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiButton *This);
    HRESULT (ANXAPI *SetParent)(ITuiButton *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiButton *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiButton *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiButton *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiButton *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiButton *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiButton *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiButton *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiButton *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiButton *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiButton *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedButton methods
    HRESULT (ANXAPI *GetButtonStyle)(ITuiButton *This, TUI_BORDER_STYLE *Style);
    HRESULT (ANXAPI *SetButtonStyle)(ITuiButton *This, TUI_BORDER_STYLE Style);
    HRESULT (ANXAPI *GetFocusedColors)(ITuiButton *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetFocusedColors)(ITuiButton *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiButton methods
    /**
      Set button label.
    **/
    HRESULT (ANXAPI *SetLabel)(
        ITuiButton *This,
        CONST CHAR8 *Label
    );

    /**
      Get button label.
    **/
    HRESULT (ANXAPI *GetLabel)(
        ITuiButton *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Set button callback (called when activated).
    **/
    HRESULT (ANXAPI *SetCallback)(
        ITuiButton *This,
        HRESULT (*Callback)(VOID *UserData),
        VOID *UserData
    );

    /**
      Check if button is default (activated by Enter anywhere in window).
    **/
    BOOLEAN (ANXAPI *IsDefault)(
        ITuiButton *This
    );

    /**
      Set button as default button.
    **/
    HRESULT (ANXAPI *SetDefault)(
        ITuiButton *This,
        BOOLEAN IsDefault
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

// {0E1F2A3B-4C5D-6E7F-8A9B-0C1D2E3F4A5B}
DEFINE_GUID(IID_ITuiThemedListBox,
    0x0E1F2A3B, 0x4C5D, 0x6E7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B);

/**
  ITuiThemedListBox Interface

  ListBox theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedListBox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedListBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedListBox *This);
    UINTN (ANXAPI *Release)(ITuiThemedListBox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedListBox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedListBox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedListBox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedListBox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedListBox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedListBox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedListBox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedListBox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedListBox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedListBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedListBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedListBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedListBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedListBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedListBox *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedListBox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedListBox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedListBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedListBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedListBox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedListBox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedListBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedListBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedListBox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedListBox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedListBox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedListBox methods
    /**
      Get selection colors.
    **/
    HRESULT (ANXAPI *GetSelectionColors)(
        ITuiThemedListBox *This,
        TUI_COLOR *Foreground,
        TUI_COLOR *Background
    );

    /**
      Set selection colors.
    **/
    HRESULT (ANXAPI *SetSelectionColors)(
        ITuiThemedListBox *This,
        TUI_COLOR Foreground,
        TUI_COLOR Background
    );

    /**
      Get scrollbar colors.
    **/
    HRESULT (ANXAPI *GetScrollbarColors)(
        ITuiThemedListBox *This,
        TUI_COLOR *BarColor,
        TUI_COLOR *ThumbColor
    );

    /**
      Set scrollbar colors.
    **/
    HRESULT (ANXAPI *SetScrollbarColors)(
        ITuiThemedListBox *This,
        TUI_COLOR BarColor,
        TUI_COLOR ThumbColor
    );
} ITuiThemedListBox_Vtbl;

struct _ITuiThemedListBox {
    CONST ITuiThemedListBox_Vtbl *Vtbl;
};

// {C9D0E1F2-A3B4-4C5D-6E7F-8A9B0C1D2E3F}
DEFINE_GUID(IID_ITuiListBox,
    0xC9D0E1F2, 0xA3B4, 0x4C5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F);

/**
  ITuiListBox Interface

  Scrollable list of selectable items. Inherits from ITuiThemedListBox.
**/
typedef struct _ITuiListBox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiListBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiListBox *This);
    UINTN (ANXAPI *Release)(ITuiListBox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiListBox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiListBox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiListBox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiListBox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiListBox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiListBox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiListBox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiListBox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiListBox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiListBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiListBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiListBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiListBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiListBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiListBox *This);
    HRESULT (ANXAPI *SetParent)(ITuiListBox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiListBox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiListBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiListBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiListBox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiListBox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiListBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiListBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiListBox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiListBox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiListBox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedListBox methods
    HRESULT (ANXAPI *GetSelectionColors)(ITuiListBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectionColors)(ITuiListBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetScrollbarColors)(ITuiListBox *This, TUI_COLOR *BarColor, TUI_COLOR *ThumbColor);
    HRESULT (ANXAPI *SetScrollbarColors)(ITuiListBox *This, TUI_COLOR BarColor, TUI_COLOR ThumbColor);

    // ITuiListBox methods
    /**
      Add item to list.
    **/
    HRESULT (ANXAPI *AddItem)(
        ITuiListBox *This,
        CONST CHAR8 *Text,
        VOID *UserData
    );

    /**
      Remove item at index.
    **/
    HRESULT (ANXAPI *RemoveItem)(
        ITuiListBox *This,
        UINT32 Index
    );

    /**
      Clear all items.
    **/
    HRESULT (ANXAPI *Clear)(
        ITuiListBox *This
    );

    /**
      Get number of items.
    **/
    HRESULT (ANXAPI *GetItemCount)(
        ITuiListBox *This,
        UINT32 *Count
    );

    /**
      Get selected item index (-1 if none).
    **/
    HRESULT (ANXAPI *GetSelectedIndex)(
        ITuiListBox *This,
        INT32 *Index
    );

    /**
      Set selected item index.
    **/
    HRESULT (ANXAPI *SetSelectedIndex)(
        ITuiListBox *This,
        INT32 Index
    );

    /**
      Get item text at index.
    **/
    HRESULT (ANXAPI *GetItemText)(
        ITuiListBox *This,
        UINT32 Index,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Get item user data at index.
    **/
    HRESULT (ANXAPI *GetItemUserData)(
        ITuiListBox *This,
        UINT32 Index,
        VOID **UserData
    );

    /**
      Check if multi-select is enabled.
    **/
    BOOLEAN (ANXAPI *IsMultiSelect)(
        ITuiListBox *This
    );

    /**
      Enable/disable multi-select mode.
    **/
    HRESULT (ANXAPI *SetMultiSelect)(
        ITuiListBox *This,
        BOOLEAN Enabled
    );

    /**
      Get selected indices (for multi-select).
    **/
    HRESULT (ANXAPI *GetSelectedIndices)(
        ITuiListBox *This,
        INT32 *Indices,
        UINT32 *Count
    );
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

    /* Basic styles */
    TuiBorderSingle,        /* ┌─┐ │ └─┘ */
    TuiBorderDouble,        /* ╔═╗ ║ ╚═╝ */
    TuiBorderRounded,       /* ╭─╮ │ ╰─╯ */
    TuiBorderAscii,         /* +-+ | +-+ */

    /* Mixed styles */
    TuiBorderSingleDouble,  /* ╓─╖ ║ ╙─╜ (single horiz, double vert) */
    TuiBorderDoubleSingle,  /* ╒═╕ │ ╘═╛ (double horiz, single vert) */
    TuiBorderRoundedSingle, /* ╭─╮ │ ╰─╯ (rounded corners, single) */

    /* 3D effects */
    TuiBorderFlat,          /* Simple flat appearance */
    TuiBorderSunken,        /* Appears recessed (dark top/left, light bottom/right) */
    TuiBorderRisen,         /* Appears raised (light top/left, dark bottom/right) */
    TuiBorder3D,            /* Full 3D effect with shadows */
    TuiBorderEtched,        /* Etched/engraved appearance */
    TuiBorderRidge,         /* Ridge (opposite of etched) */

    /* Special styles */
    TuiBorderDashed,        /* ┌ ─ ┐ (dashed lines) */
    TuiBorderDotted,        /* ┌···┐ (dotted lines) */
    TuiBorderThick,         /* ┏━┓ ┃ ┗━┛ (thick lines) */
    TuiBorderBlock          /* ▛▀▜ █ ▙▄▟ (block characters) */
} TUI_BORDER_STYLE;

/**
  Theme Rendering Delegates

  Allow themes to completely override widget rendering and sizing
**/

/* Widget rendering context passed to theme renderers */
typedef struct {
    ITuiSurface *Surface;
    CONST TUI_RECT *Bounds;
    BOOLEAN Focused;
    BOOLEAN Enabled;
    BOOLEAN Pressed;
    VOID *WidgetData;  /* Widget-specific data */
} TUI_RENDER_CONTEXT;

/* Button rendering delegate */
typedef HRESULT (ANXAPI *TUI_BUTTON_RENDERER)(
    ITuiTheme *Theme,
    CONST TUI_RENDER_CONTEXT *Context,
    CONST CHAR8 *Label
);

/* Button size calculator */
typedef HRESULT (ANXAPI *TUI_BUTTON_SIZER)(
    ITuiTheme *Theme,
    CONST CHAR8 *Label,
    UINT32 *Width,
    UINT32 *Height
);

/* Checkbox rendering delegate */
typedef HRESULT (ANXAPI *TUI_CHECKBOX_RENDERER)(
    ITuiTheme *Theme,
    CONST TUI_RENDER_CONTEXT *Context,
    CONST CHAR8 *Label,
    BOOLEAN Checked,
    UINT8 TristateValue
);

/* Checkbox size calculator */
typedef HRESULT (ANXAPI *TUI_CHECKBOX_SIZER)(
    ITuiTheme *Theme,
    CONST CHAR8 *Label,
    UINT32 *Width,
    UINT32 *Height
);

/* Input field rendering delegate */
typedef HRESULT (ANXAPI *TUI_INPUT_RENDERER)(
    ITuiTheme *Theme,
    CONST TUI_RENDER_CONTEXT *Context,
    CONST CHAR8 *Label,
    CONST CHAR8 *Value,
    UINT32 CursorPos,
    UINT32 Width
);

/* Window frame rendering delegate */
typedef HRESULT (ANXAPI *TUI_WINDOW_RENDERER)(
    ITuiTheme *Theme,
    CONST TUI_RENDER_CONTEXT *Context,
    CONST CHAR8 *Title,
    TUI_BORDER_STYLE BorderStyle
);

/**
  ITuiTheme Interface

  Customizable UI theme (colors, borders, styles, and rendering).
**/
typedef struct _ITuiTheme_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiTheme *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiTheme *This);
    UINTN (ANXAPI *Release)(ITuiTheme *This);

    HRESULT (ANXAPI *SetColors)(ITuiTheme *This, TUI_THEME_COMPONENT Component, CONST TUI_THEME_COLORS *Colors);
    HRESULT (ANXAPI *GetColors)(ITuiTheme *This, TUI_THEME_COMPONENT Component, TUI_THEME_COLORS *Colors);
    HRESULT (ANXAPI *SetBorderStyle)(ITuiTheme *This, TUI_BORDER_STYLE Style);
    HRESULT (ANXAPI *GetBorderStyle)(ITuiTheme *This, TUI_BORDER_STYLE *Style);
    HRESULT (ANXAPI *SetWindowShadow)(ITuiTheme *This, BOOLEAN Enabled, CHAR8 ShadowChar);
    HRESULT (ANXAPI *GetWindowShadow)(ITuiTheme *This, BOOLEAN *Enabled, CHAR8 *ShadowChar);
    HRESULT (ANXAPI *SetButtonStyle)(ITuiTheme *This, TUI_BORDER_STYLE Style);
    HRESULT (ANXAPI *GetButtonStyle)(ITuiTheme *This, TUI_BORDER_STYLE *Style);
    HRESULT (ANXAPI *SetUseUnicode)(ITuiTheme *This, BOOLEAN UseUnicode);
    HRESULT (ANXAPI *GetUseUnicode)(ITuiTheme *This, BOOLEAN *UseUnicode);
    HRESULT (ANXAPI *SetButtonRenderer)(ITuiTheme *This, TUI_BUTTON_RENDERER Renderer, TUI_BUTTON_SIZER Sizer);
    HRESULT (ANXAPI *GetButtonRenderer)(ITuiTheme *This, TUI_BUTTON_RENDERER *Renderer, TUI_BUTTON_SIZER *Sizer);
    HRESULT (ANXAPI *SetCheckboxRenderer)(ITuiTheme *This, TUI_CHECKBOX_RENDERER Renderer, TUI_CHECKBOX_SIZER Sizer);
    HRESULT (ANXAPI *GetCheckboxRenderer)(ITuiTheme *This, TUI_CHECKBOX_RENDERER *Renderer, TUI_CHECKBOX_SIZER *Sizer);
    HRESULT (ANXAPI *SetInputRenderer)(ITuiTheme *This, TUI_INPUT_RENDERER Renderer);
    HRESULT (ANXAPI *GetInputRenderer)(ITuiTheme *This, TUI_INPUT_RENDERER *Renderer);
    HRESULT (ANXAPI *SetWindowRenderer)(ITuiTheme *This, TUI_WINDOW_RENDERER Renderer);
    HRESULT (ANXAPI *GetWindowRenderer)(ITuiTheme *This, TUI_WINDOW_RENDERER *Renderer);
    HRESULT (ANXAPI *LoadFromFile)(ITuiTheme *This, CONST CHAR8 *FilePath);
    HRESULT (ANXAPI *SaveToFile)(ITuiTheme *This, CONST CHAR8 *FilePath);
    HRESULT (ANXAPI *SetName)(ITuiTheme *This, CONST CHAR8 *Name);
    HRESULT (ANXAPI *GetName)(ITuiTheme *This, CHAR8 *Buffer, UINTN BufferSize);
} ITuiTheme_Vtbl;

struct _ITuiTheme {
    CONST ITuiTheme_Vtbl *Vtbl;
};

#ifdef COBJMACROS
#define ITuiTheme_QueryInterface(This,riid,ppvObject) (This)->Vtbl->QueryInterface(This,riid,ppvObject)
#define ITuiTheme_AddRef(This) (This)->Vtbl->AddRef(This)
#define ITuiTheme_Release(This) (This)->Vtbl->Release(This)
#define ITuiTheme_SetColors(This,Component,Colors) (This)->Vtbl->SetColors(This,Component,Colors)
#define ITuiTheme_GetColors(This,Component,Colors) (This)->Vtbl->GetColors(This,Component,Colors)
#define ITuiTheme_SetBorderStyle(This,Style) (This)->Vtbl->SetBorderStyle(This,Style)
#define ITuiTheme_GetBorderStyle(This,Style) (This)->Vtbl->GetBorderStyle(This,Style)
#define ITuiTheme_SetWindowShadow(This,Enabled,ShadowChar) (This)->Vtbl->SetWindowShadow(This,Enabled,ShadowChar)
#define ITuiTheme_GetWindowShadow(This,Enabled,ShadowChar) (This)->Vtbl->GetWindowShadow(This,Enabled,ShadowChar)
#define ITuiTheme_SetButtonStyle(This,Style) (This)->Vtbl->SetButtonStyle(This,Style)
#define ITuiTheme_GetButtonStyle(This,Style) (This)->Vtbl->GetButtonStyle(This,Style)
#define ITuiTheme_SetUseUnicode(This,UseUnicode) (This)->Vtbl->SetUseUnicode(This,UseUnicode)
#define ITuiTheme_GetUseUnicode(This,UseUnicode) (This)->Vtbl->GetUseUnicode(This,UseUnicode)
#define ITuiTheme_SetButtonRenderer(This,Renderer,Sizer) (This)->Vtbl->SetButtonRenderer(This,Renderer,Sizer)
#define ITuiTheme_GetButtonRenderer(This,Renderer,Sizer) (This)->Vtbl->GetButtonRenderer(This,Renderer,Sizer)
#define ITuiTheme_SetCheckboxRenderer(This,Renderer,Sizer) (This)->Vtbl->SetCheckboxRenderer(This,Renderer,Sizer)
#define ITuiTheme_GetCheckboxRenderer(This,Renderer,Sizer) (This)->Vtbl->GetCheckboxRenderer(This,Renderer,Sizer)
#define ITuiTheme_SetInputRenderer(This,Renderer) (This)->Vtbl->SetInputRenderer(This,Renderer)
#define ITuiTheme_GetInputRenderer(This,Renderer) (This)->Vtbl->GetInputRenderer(This,Renderer)
#define ITuiTheme_SetWindowRenderer(This,Renderer) (This)->Vtbl->SetWindowRenderer(This,Renderer)
#define ITuiTheme_GetWindowRenderer(This,Renderer) (This)->Vtbl->GetWindowRenderer(This,Renderer)
#define ITuiTheme_LoadFromFile(This,FilePath) (This)->Vtbl->LoadFromFile(This,FilePath)
#define ITuiTheme_SaveToFile(This,FilePath) (This)->Vtbl->SaveToFile(This,FilePath)
#define ITuiTheme_SetName(This,Name) (This)->Vtbl->SetName(This,Name)
#define ITuiTheme_GetName(This,Buffer,BufferSize) (This)->Vtbl->GetName(This,Buffer,BufferSize)
#endif /* COBJMACROS */

// {D6E7F8A9-B0C1-4D2E-3F4A-5B6C7D8E9F0A}
DEFINE_GUID(IID_ITuiTabControl,
    0xD6E7F8A9, 0xB0C1, 0x4D2E, 0x3F, 0x4A, 0x5B, 0x6C, 0x7D, 0x8E, 0x9F, 0x0A);

/**
  ITuiTabControl Interface

  Tabbed pages container.
**/
typedef struct _ITuiTabControl_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiTabControl *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiTabControl *This);
    UINTN (ANXAPI *Release)(ITuiTabControl *This);

    /**
      Add a new tab page.
    **/
    HRESULT (ANXAPI *AddTab)(
        ITuiTabControl *This,
        CONST CHAR8 *Title,
        ITuiWindow *Content,
        VOID *UserData
    );

    /**
      Remove a tab by index.
    **/
    HRESULT (ANXAPI *RemoveTab)(
        ITuiTabControl *This,
        UINT32 Index
    );

    /**
      Get number of tabs.
    **/
    HRESULT (ANXAPI *GetTabCount)(
        ITuiTabControl *This,
        UINT32 *Count
    );

    /**
      Get/set active tab index.
    **/
    HRESULT (ANXAPI *GetActiveTab)(
        ITuiTabControl *This,
        INT32 *Index
    );

    HRESULT (ANXAPI *SetActiveTab)(
        ITuiTabControl *This,
        INT32 Index
    );

    /**
      Get tab title.
    **/
    HRESULT (ANXAPI *GetTabTitle)(
        ITuiTabControl *This,
        UINT32 Index,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Set tab title.
    **/
    HRESULT (ANXAPI *SetTabTitle)(
        ITuiTabControl *This,
        UINT32 Index,
        CONST CHAR8 *Title
    );

    /**
      Enable/disable a tab.
    **/
    HRESULT (ANXAPI *SetTabEnabled)(
        ITuiTabControl *This,
        UINT32 Index,
        BOOLEAN Enabled
    );

    /**
      Render tab control.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiTabControl *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y,
        UINT32 Width,
        UINT32 Height
    );

    /**
      Handle input events.
    **/
    HRESULT (ANXAPI *HandleInput)(
        ITuiTabControl *This,
        CONST TUI_INPUT_EVENT *Event,
        BOOLEAN *Handled
    );

} ITuiTabControl_Vtbl;

struct _ITuiTabControl {
    CONST ITuiTabControl_Vtbl *Vtbl;
};

/**
  Progress Bar Style
**/
typedef enum _TUI_PROGRESS_STYLE {
    TuiProgressBlocks,      /* ████████░░ */
    TuiProgressDots,        /* ●●●●●○○○○○ */
    TuiProgressArrows,      /* >>>>>>>--- */
    TuiProgressPercent,     /* [50%] */
    TuiProgressSpinner      /* Spinning animation */
} TUI_PROGRESS_STYLE;

// {9D0E1F2A-3B4C-5D6E-7F8A-9B0C1D2E3F4A}
DEFINE_GUID(IID_ITuiThemedProgressBar,
    0x9D0E1F2A, 0x3B4C, 0x5D6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A);

/**
  ITuiThemedProgressBar Interface

  Progress bar theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedProgressBar_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedProgressBar *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedProgressBar *This);
    UINTN (ANXAPI *Release)(ITuiThemedProgressBar *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedProgressBar *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedProgressBar *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedProgressBar *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedProgressBar *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedProgressBar *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedProgressBar *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedProgressBar *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedProgressBar *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedProgressBar *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedProgressBar *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedProgressBar *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedProgressBar *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedProgressBar *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedProgressBar *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedProgressBar *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedProgressBar *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedProgressBar *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedProgressBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedProgressBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedProgressBar *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedProgressBar *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedProgressBar *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedProgressBar *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedProgressBar *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedProgressBar *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedProgressBar *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedProgressBar methods
    /**
      Get filled/empty colors for progress bar.
    **/
    HRESULT (ANXAPI *GetProgressColors)(
        ITuiThemedProgressBar *This,
        TUI_COLOR *FilledColor,
        TUI_COLOR *EmptyColor
    );

    /**
      Set filled/empty colors for progress bar.
    **/
    HRESULT (ANXAPI *SetProgressColors)(
        ITuiThemedProgressBar *This,
        TUI_COLOR FilledColor,
        TUI_COLOR EmptyColor
    );
} ITuiThemedProgressBar_Vtbl;

struct _ITuiThemedProgressBar {
    CONST ITuiThemedProgressBar_Vtbl *Vtbl;
};

// {E7F8A9B0-C1D2-4E3F-4A5B-6C7D8E9F0A1B}
DEFINE_GUID(IID_ITuiProgressBar,
    0xE7F8A9B0, 0xC1D2, 0x4E3F, 0x4A, 0x5B, 0x6C, 0x7D, 0x8E, 0x9F, 0x0A, 0x1B);

/**
  ITuiProgressBar Interface

  Progress indicator widget with multiple styles. Inherits from ITuiThemedProgressBar.
**/
typedef struct _ITuiProgressBar_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiProgressBar *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiProgressBar *This);
    UINTN (ANXAPI *Release)(ITuiProgressBar *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiProgressBar *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiProgressBar *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiProgressBar *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiProgressBar *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiProgressBar *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiProgressBar *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiProgressBar *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiProgressBar *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiProgressBar *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiProgressBar *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiProgressBar *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiProgressBar *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiProgressBar *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiProgressBar *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiProgressBar *This);
    HRESULT (ANXAPI *SetParent)(ITuiProgressBar *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiProgressBar *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiProgressBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiProgressBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiProgressBar *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiProgressBar *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiProgressBar *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiProgressBar *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiProgressBar *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiProgressBar *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiProgressBar *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedProgressBar methods
    HRESULT (ANXAPI *GetProgressColors)(ITuiProgressBar *This, TUI_COLOR *FilledColor, TUI_COLOR *EmptyColor);
    HRESULT (ANXAPI *SetProgressColors)(ITuiProgressBar *This, TUI_COLOR FilledColor, TUI_COLOR EmptyColor);

    // ITuiProgressBar methods
    /**
      Set progress value (0-100).
    **/
    HRESULT (ANXAPI *SetValue)(
        ITuiProgressBar *This,
        UINT32 Value
    );

    /**
      Get current progress value.
    **/
    HRESULT (ANXAPI *GetValue)(
        ITuiProgressBar *This,
        UINT32 *Value
    );

    /**
      Get progress bar style.
    **/
    TUI_PROGRESS_STYLE (ANXAPI *GetStyle)(
        ITuiProgressBar *This
    );

    /**
      Set progress bar style.
    **/
    HRESULT (ANXAPI *SetStyle)(
        ITuiProgressBar *This,
        TUI_PROGRESS_STYLE Style
    );

    /**
      Set custom label text.
    **/
    HRESULT (ANXAPI *SetLabel)(
        ITuiProgressBar *This,
        CONST CHAR8 *Label
    );

    /**
      Get custom label text.
    **/
    HRESULT (ANXAPI *GetLabel)(
        ITuiProgressBar *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Check if indeterminate mode is enabled.
    **/
    BOOLEAN (ANXAPI *IsIndeterminate)(
        ITuiProgressBar *This
    );

    /**
      Set indeterminate mode (for unknown duration).
    **/
    HRESULT (ANXAPI *SetIndeterminate)(
        ITuiProgressBar *This,
        BOOLEAN Indeterminate
    );
} ITuiProgressBar_Vtbl;

struct _ITuiProgressBar {
    CONST ITuiProgressBar_Vtbl *Vtbl;
};

// {F8A9B0C1-D2E3-4F4A-5B6C-7D8E9F0A1B2C}
DEFINE_GUID(IID_ITuiColorPicker,
    0xF8A9B0C1, 0xD2E3, 0x4F4A, 0x5B, 0x6C, 0x7D, 0x8E, 0x9F, 0x0A, 0x1B, 0x2C);

/**
  Color Picker Mode
**/
typedef enum _TUI_COLOR_PICKER_MODE {
    TuiColorPickerBasic,       /* 8/16 color palette */
    TuiColorPicker256,         /* 256 color palette */
    TuiColorPickerRGB,         /* RGB sliders (if supported) */
    TuiColorPickerHSV          /* HSV color wheel (if supported) */
} TUI_COLOR_PICKER_MODE;

/**
  ITuiColorPicker Interface

  Interactive color selection widget.
**/
typedef struct _ITuiColorPicker_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiColorPicker *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiColorPicker *This);
    UINTN (ANXAPI *Release)(ITuiColorPicker *This);

    /**
      Set picker mode.
    **/
    HRESULT (ANXAPI *SetMode)(
        ITuiColorPicker *This,
        TUI_COLOR_PICKER_MODE Mode
    );

    /**
      Get selected color.
    **/
    HRESULT (ANXAPI *GetColor)(
        ITuiColorPicker *This,
        TUI_COLOR *Color
    );

    /**
      Set selected color.
    **/
    HRESULT (ANXAPI *SetColor)(
        ITuiColorPicker *This,
        TUI_COLOR Color
    );

    /**
      Get RGB values (0-255 each).
    **/
    HRESULT (ANXAPI *GetRGB)(
        ITuiColorPicker *This,
        UINT8 *Red,
        UINT8 *Green,
        UINT8 *Blue
    );

    /**
      Set RGB values.
    **/
    HRESULT (ANXAPI *SetRGB)(
        ITuiColorPicker *This,
        UINT8 Red,
        UINT8 Green,
        UINT8 Blue
    );

    /**
      Show picker dialog.
    **/
    HRESULT (ANXAPI *Show)(
        ITuiColorPicker *This,
        ITuiScreen *Screen,
        TUI_COLOR *SelectedColor
    );

    /**
      Render color picker inline.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiColorPicker *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y,
        UINT32 Width,
        UINT32 Height
    );

    /**
      Handle input events.
    **/
    HRESULT (ANXAPI *HandleInput)(
        ITuiColorPicker *This,
        CONST TUI_INPUT_EVENT *Event,
        BOOLEAN *Handled
    );

} ITuiColorPicker_Vtbl;

struct _ITuiColorPicker {
    CONST ITuiColorPicker_Vtbl *Vtbl;
};

// {1F2A3B4C-5D6E-7F8A-9B0C-1D2E3F4A5B6C}
DEFINE_GUID(IID_ITuiThemedGroupBox,
    0x1F2A3B4C, 0x5D6E, 0x7F8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B, 0x6C);

/**
  ITuiThemedGroupBox Interface

  GroupBox theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedGroupBox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedGroupBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedGroupBox *This);
    UINTN (ANXAPI *Release)(ITuiThemedGroupBox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedGroupBox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedGroupBox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedGroupBox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedGroupBox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedGroupBox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedGroupBox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedGroupBox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedGroupBox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedGroupBox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedGroupBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedGroupBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedGroupBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedGroupBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedGroupBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedGroupBox *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedGroupBox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedGroupBox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedGroupBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedGroupBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedGroupBox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedGroupBox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedGroupBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedGroupBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedGroupBox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedGroupBox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedGroupBox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedGroupBox methods
    /**
      Get border style.
    **/
    TUI_BORDER_STYLE (ANXAPI *GetBorderStyle)(
        ITuiThemedGroupBox *This
    );

    /**
      Set border style.
    **/
    HRESULT (ANXAPI *SetBorderStyle)(
        ITuiThemedGroupBox *This,
        TUI_BORDER_STYLE Style
    );

    /**
      Get title color.
    **/
    HRESULT (ANXAPI *GetTitleColor)(
        ITuiThemedGroupBox *This,
        TUI_COLOR *TitleColor
    );

    /**
      Set title color.
    **/
    HRESULT (ANXAPI *SetTitleColor)(
        ITuiThemedGroupBox *This,
        TUI_COLOR TitleColor
    );
} ITuiThemedGroupBox_Vtbl;

struct _ITuiThemedGroupBox {
    CONST ITuiThemedGroupBox_Vtbl *Vtbl;
};

// {A9B0C1D2-E3F4-4A5B-6C7D-8E9F0A1B2C3D}
DEFINE_GUID(IID_ITuiGroupBox,
    0xA9B0C1D2, 0xE3F4, 0x4A5B, 0x6C, 0x7D, 0x8E, 0x9F, 0x0A, 0x1B, 0x2C, 0x3D);

/**
  ITuiGroupBox Interface

  Container widget for grouping related controls. Inherits from ITuiThemedGroupBox.
**/
typedef struct _ITuiGroupBox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiGroupBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiGroupBox *This);
    UINTN (ANXAPI *Release)(ITuiGroupBox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiGroupBox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiGroupBox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiGroupBox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiGroupBox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiGroupBox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiGroupBox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiGroupBox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiGroupBox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiGroupBox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiGroupBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiGroupBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiGroupBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiGroupBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiGroupBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiGroupBox *This);
    HRESULT (ANXAPI *SetParent)(ITuiGroupBox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiGroupBox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiGroupBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiGroupBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiGroupBox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiGroupBox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiGroupBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiGroupBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiGroupBox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiGroupBox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiGroupBox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedGroupBox methods
    TUI_BORDER_STYLE (ANXAPI *GetBorderStyle)(ITuiGroupBox *This);
    HRESULT (ANXAPI *SetBorderStyle)(ITuiGroupBox *This, TUI_BORDER_STYLE Style);
    HRESULT (ANXAPI *GetTitleColor)(ITuiGroupBox *This, TUI_COLOR *TitleColor);
    HRESULT (ANXAPI *SetTitleColor)(ITuiGroupBox *This, TUI_COLOR TitleColor);

    // ITuiGroupBox methods
    /**
      Set group box title.
    **/
    HRESULT (ANXAPI *SetTitle)(
        ITuiGroupBox *This,
        CONST CHAR8 *Title
    );

    /**
      Get group box title.
    **/
    HRESULT (ANXAPI *GetTitle)(
        ITuiGroupBox *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Add a child widget at specific position.
    **/
    HRESULT (ANXAPI *AddChildAt)(
        ITuiGroupBox *This,
        ITuiWidget *Widget,
        INT32 X,
        INT32 Y
    );

    /**
      Clear all children.
    **/
    HRESULT (ANXAPI *ClearChildren)(
        ITuiGroupBox *This
    );

    /**
      Get content padding.
    **/
    HRESULT (ANXAPI *GetPadding)(
        ITuiGroupBox *This,
        UINT32 *Top,
        UINT32 *Right,
        UINT32 *Bottom,
        UINT32 *Left
    );

    /**
      Set content padding.
    **/
    HRESULT (ANXAPI *SetPadding)(
        ITuiGroupBox *This,
        UINT32 Top,
        UINT32 Right,
        UINT32 Bottom,
        UINT32 Left
    );

    /**
      Get child count.
    **/
    UINT32 (ANXAPI *GetChildCount)(
        ITuiGroupBox *This
    );
} ITuiGroupBox_Vtbl;

struct _ITuiGroupBox {
    CONST ITuiGroupBox_Vtbl *Vtbl;
};

// {B0C1D2E3-F4A5-4B6C-7D8E-9F0A1B2C3D4E}
DEFINE_GUID(IID_ITuiFocusManager,
    0xB0C1D2E3, 0xF4A5, 0x4B6C, 0x7D, 0x8E, 0x9F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E);

/**
  ITuiFocusManager Interface

  Manages focus and tab navigation between widgets.
**/
typedef struct _ITuiFocusManager_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiFocusManager *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiFocusManager *This);
    UINTN (ANXAPI *Release)(ITuiFocusManager *This);

    /**
      Register a focusable widget.
    **/
    HRESULT (ANXAPI *RegisterWidget)(
        ITuiFocusManager *This,
        VOID *Widget,
        UINT32 TabOrder
    );

    /**
      Unregister a widget.
    **/
    HRESULT (ANXAPI *UnregisterWidget)(
        ITuiFocusManager *This,
        VOID *Widget
    );

    /**
      Set focus to a specific widget.
    **/
    HRESULT (ANXAPI *SetFocus)(
        ITuiFocusManager *This,
        VOID *Widget
    );

    /**
      Get currently focused widget.
    **/
    HRESULT (ANXAPI *GetFocus)(
        ITuiFocusManager *This,
        VOID **Widget
    );

    /**
      Move focus to next widget (Tab key).
    **/
    HRESULT (ANXAPI *FocusNext)(
        ITuiFocusManager *This
    );

    /**
      Move focus to previous widget (Shift+Tab).
    **/
    HRESULT (ANXAPI *FocusPrevious)(
        ITuiFocusManager *This
    );

    /**
      Set widget tab order.
    **/
    HRESULT (ANXAPI *SetTabOrder)(
        ITuiFocusManager *This,
        VOID *Widget,
        UINT32 TabOrder
    );

    /**
      Get widget tab order.
    **/
    HRESULT (ANXAPI *GetTabOrder)(
        ITuiFocusManager *This,
        VOID *Widget,
        UINT32 *TabOrder
    );

    /**
      Enable/disable tab navigation.
    **/
    HRESULT (ANXAPI *SetTabNavigationEnabled)(
        ITuiFocusManager *This,
        BOOLEAN Enabled
    );

    /**
      Handle keyboard input for focus management.
    **/
    HRESULT (ANXAPI *HandleKey)(
        ITuiFocusManager *This,
        TUI_KEY Key,
        BOOLEAN *Handled
    );

} ITuiFocusManager_Vtbl;

struct _ITuiFocusManager {
    CONST ITuiFocusManager_Vtbl *Vtbl;
};

// {5F6A7B8C-9D0E-1F2A-3B4C-5D6E7F8A9B0C}
DEFINE_GUID(IID_ITuiThemedLabel,
    0x5F6A7B8C, 0x9D0E, 0x1F2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C);

/**
  ITuiThemedLabel Interface

  Label theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedLabel_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedLabel *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedLabel *This);
    UINTN (ANXAPI *Release)(ITuiThemedLabel *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedLabel *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedLabel *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedLabel *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedLabel *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedLabel *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedLabel *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedLabel *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedLabel *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedLabel *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedLabel *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedLabel *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedLabel *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedLabel *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedLabel *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedLabel *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedLabel *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedLabel *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedLabel *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedLabel *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedLabel *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedLabel *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedLabel *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedLabel *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedLabel *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedLabel *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedLabel *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedLabel methods
    /**
      Get hotkey color for theming.
    **/
    HRESULT (ANXAPI *GetHotkeyColor)(
        ITuiThemedLabel *This,
        TUI_COLOR *HotkeyColor
    );

    /**
      Set hotkey color for theming.
    **/
    HRESULT (ANXAPI *SetHotkeyColor)(
        ITuiThemedLabel *This,
        TUI_COLOR HotkeyColor
    );
} ITuiThemedLabel_Vtbl;

struct _ITuiThemedLabel {
    CONST ITuiThemedLabel_Vtbl *Vtbl;
};

// {C1D2E3F4-A5B6-4C7D-8E9F-0A1B2C3D4E5F}
DEFINE_GUID(IID_ITuiLabel,
    0xC1D2E3F4, 0xA5B6, 0x4C7D, 0x8E, 0x9F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F);

/**
  ITuiLabel Interface

  Static text label with hotkey support. Inherits from ITuiThemedLabel.
**/
typedef struct _ITuiLabel_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiLabel *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiLabel *This);
    UINTN (ANXAPI *Release)(ITuiLabel *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiLabel *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiLabel *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiLabel *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiLabel *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiLabel *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiLabel *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiLabel *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiLabel *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiLabel *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiLabel *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiLabel *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiLabel *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiLabel *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiLabel *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiLabel *This);
    HRESULT (ANXAPI *SetParent)(ITuiLabel *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiLabel *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiLabel *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiLabel *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiLabel *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiLabel *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiLabel *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiLabel *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiLabel *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiLabel *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiLabel *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedLabel methods
    HRESULT (ANXAPI *GetHotkeyColor)(ITuiLabel *This, TUI_COLOR *HotkeyColor);
    HRESULT (ANXAPI *SetHotkeyColor)(ITuiLabel *This, TUI_COLOR HotkeyColor);

    // ITuiLabel methods
    /**
      Set label text.
    **/
    HRESULT (ANXAPI *SetText)(
        ITuiLabel *This,
        CONST CHAR8 *Text
    );

    /**
      Get label text.
    **/
    HRESULT (ANXAPI *GetText)(
        ITuiLabel *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Set hotkey character (will be underlined).
    **/
    HRESULT (ANXAPI *SetHotkey)(
        ITuiLabel *This,
        CHAR8 Hotkey
    );

    /**
      Set text direction.
    **/
    HRESULT (ANXAPI *SetTextDirection)(
        ITuiLabel *This,
        TUI_TEXT_DIRECTION Direction
    );

    /**
      Set text alignment (0=left, 1=center, 2=right).
    **/
    HRESULT (ANXAPI *SetAlignment)(
        ITuiLabel *This,
        INT32 Alignment
    );

    /**
      Link this label to another widget for focus control.
      When the hotkey is pressed or label is clicked, the linked widget receives focus.
    **/
    HRESULT (ANXAPI *SetLinkedWidget)(
        ITuiLabel *This,
        ITuiWidget *LinkedWidget
    );
} ITuiLabel_Vtbl;

struct _ITuiLabel {
    CONST ITuiLabel_Vtbl *Vtbl;
};

// {D2E3F4A5-B6C7-4D8E-9F0A-1B2C3D4E5F6A}
DEFINE_GUID(IID_ITuiTextEditor,
    0xD2E3F4A5, 0xB6C7, 0x4D8E, 0x9F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0x6A);

/**
  ITuiTextEditor Interface

  Multi-line text editor with scrolling and syntax highlighting.
**/
typedef struct _ITuiTextEditor_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiTextEditor *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiTextEditor *This);
    UINTN (ANXAPI *Release)(ITuiTextEditor *This);

    HRESULT (ANXAPI *SetText)(ITuiTextEditor *This, CONST CHAR8 *Text);
    HRESULT (ANXAPI *GetText)(ITuiTextEditor *This, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *LoadFile)(ITuiTextEditor *This, CONST CHAR8 *FilePath);
    HRESULT (ANXAPI *SaveFile)(ITuiTextEditor *This, CONST CHAR8 *FilePath);
    HRESULT (ANXAPI *SetReadOnly)(ITuiTextEditor *This, BOOLEAN ReadOnly);
    HRESULT (ANXAPI *SetWordWrap)(ITuiTextEditor *This, BOOLEAN WordWrap);
    HRESULT (ANXAPI *SetTabSize)(ITuiTextEditor *This, UINT32 TabSize);
    HRESULT (ANXAPI *SetSyntaxHighlighting)(ITuiTextEditor *This, CONST CHAR8 *Language);
    HRESULT (ANXAPI *GetCursorPosition)(ITuiTextEditor *This, UINT32 *Line, UINT32 *Column);
    HRESULT (ANXAPI *SetCursorPosition)(ITuiTextEditor *This, UINT32 Line, UINT32 Column);
    HRESULT (ANXAPI *GetLineCount)(ITuiTextEditor *This, UINT32 *Count);
    HRESULT (ANXAPI *GetLine)(ITuiTextEditor *This, UINT32 LineNumber, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *InsertText)(ITuiTextEditor *This, CONST CHAR8 *Text);
    HRESULT (ANXAPI *DeleteSelection)(ITuiTextEditor *This);
    HRESULT (ANXAPI *SelectAll)(ITuiTextEditor *This);
    HRESULT (ANXAPI *Undo)(ITuiTextEditor *This);
    HRESULT (ANXAPI *Redo)(ITuiTextEditor *This);
    HRESULT (ANXAPI *Cut)(ITuiTextEditor *This);
    HRESULT (ANXAPI *Copy)(ITuiTextEditor *This);
    HRESULT (ANXAPI *Paste)(ITuiTextEditor *This);
    HRESULT (ANXAPI *Find)(ITuiTextEditor *This, CONST CHAR8 *SearchText, BOOLEAN CaseSensitive);
    HRESULT (ANXAPI *Replace)(ITuiTextEditor *This, CONST CHAR8 *SearchText, CONST CHAR8 *ReplaceText);
    HRESULT (ANXAPI *SetTextDirection)(ITuiTextEditor *This, TUI_TEXT_DIRECTION Direction);
    HRESULT (ANXAPI *Render)(ITuiTextEditor *This, ITuiScreen *Screen, INT32 X, INT32 Y, UINT32 Width, UINT32 Height);
    HRESULT (ANXAPI *HandleInput)(ITuiTextEditor *This, CONST TUI_INPUT_EVENT *Event, BOOLEAN *Handled);
} ITuiTextEditor_Vtbl;

struct _ITuiTextEditor {
    CONST ITuiTextEditor_Vtbl *Vtbl;
};

// {E3F4A5B6-C7D8-4E9F-0A1B-2C3D4E5F6A7B}
DEFINE_GUID(IID_ITuiScrollView,
    0xE3F4A5B6, 0xC7D8, 0x4E9F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0x6A, 0x7B);

/**
  ITuiScrollView Interface

  Scrollable container for widgets or content.
**/
typedef struct _ITuiScrollView_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiScrollView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiScrollView *This);
    UINTN (ANXAPI *Release)(ITuiScrollView *This);

    HRESULT (ANXAPI *SetContentSize)(ITuiScrollView *This, UINT32 Width, UINT32 Height);
    HRESULT (ANXAPI *GetContentSize)(ITuiScrollView *This, UINT32 *Width, UINT32 *Height);
    HRESULT (ANXAPI *SetScrollPosition)(ITuiScrollView *This, INT32 X, INT32 Y);
    HRESULT (ANXAPI *GetScrollPosition)(ITuiScrollView *This, INT32 *X, INT32 *Y);
    HRESULT (ANXAPI *ScrollBy)(ITuiScrollView *This, INT32 DeltaX, INT32 DeltaY);
    HRESULT (ANXAPI *SetShowScrollbars)(ITuiScrollView *This, BOOLEAN Horizontal, BOOLEAN Vertical);
    HRESULT (ANXAPI *AddChild)(ITuiScrollView *This, VOID *Widget, INT32 X, INT32 Y);
    HRESULT (ANXAPI *RemoveChild)(ITuiScrollView *This, VOID *Widget);
    HRESULT (ANXAPI *Render)(ITuiScrollView *This, ITuiScreen *Screen, INT32 X, INT32 Y, UINT32 Width, UINT32 Height);
    HRESULT (ANXAPI *HandleInput)(ITuiScrollView *This, CONST TUI_INPUT_EVENT *Event, BOOLEAN *Handled);
} ITuiScrollView_Vtbl;

struct _ITuiScrollView {
    CONST ITuiScrollView_Vtbl *Vtbl;
};

// {3F4E5D6C-7B8A-9C0D-1E2F-3A4B5C6D7E8F}
DEFINE_GUID(IID_ITuiLongOpDialog,
    0x3F4E5D6C, 0x7B8A, 0x9C0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F);

/**
  ITuiLongOpDialog Interface

  Modal dialog for long-running operations with progress tracking,
  time estimation, and cancel support.
**/
typedef struct _ITuiLongOpDialog_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiLongOpDialog *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiLongOpDialog *This);
    UINTN (ANXAPI *Release)(ITuiLongOpDialog *This);

    /**
      Render the dialog.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiLongOpDialog *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y
    );

    /**
      Handle keyboard input.
    **/
    HRESULT (ANXAPI *HandleKey)(
        ITuiLongOpDialog *This,
        TUI_KEY Key
    );

    /**
      Standard widget methods.
    **/
    HRESULT (ANXAPI *SetBounds)(ITuiLongOpDialog *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiLongOpDialog *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiLongOpDialog *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiLongOpDialog *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiLongOpDialog *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiLongOpDialog *This);

    /**
      Set dialog title.
    **/
    HRESULT (ANXAPI *SetTitle)(
        ITuiLongOpDialog *This,
        CONST CHAR8 *Title
    );

    /**
      Update progress (0-100 percent) and status text.
    **/
    HRESULT (ANXAPI *UpdateProgress)(
        ITuiLongOpDialog *This,
        UINT32 Percent,
        CONST CHAR8 *StatusText
    );

    /**
      Set indeterminate mode (for operations with unknown duration).
    **/
    HRESULT (ANXAPI *SetIndeterminate)(
        ITuiLongOpDialog *This,
        BOOLEAN Indeterminate
    );

    /**
      Check if user cancelled the operation.
    **/
    BOOLEAN (ANXAPI *IsCancelled)(ITuiLongOpDialog *This);

    /**
      Set callback for cancel button.
    **/
    HRESULT (ANXAPI *SetCancelCallback)(
        ITuiLongOpDialog *This,
        HRESULT (*Callback)(VOID *UserData),
        VOID *UserData
    );

    /**
      Start the operation (resets timers).
    **/
    HRESULT (ANXAPI *Start)(ITuiLongOpDialog *This);

    /**
      Mark operation as complete.
    **/
    HRESULT (ANXAPI *Complete)(ITuiLongOpDialog *This);

} ITuiLongOpDialog_Vtbl;

struct _ITuiLongOpDialog {
    CONST ITuiLongOpDialog_Vtbl *Vtbl;
};

// {4A5B6C7D-8E9F-0A1B-2C3D-4E5F6A7B8C9D}
DEFINE_GUID(IID_ITuiDirectoryDialog,
    0x4A5B6C7D, 0x8E9F, 0x0A1B, 0x2C, 0x3D, 0x4E, 0x5F, 0x6A, 0x7B, 0x8C, 0x9D);

/**
  ITuiDirectoryDialog Interface

  Modal dialog for selecting directories with tree navigation.
**/
typedef struct _ITuiDirectoryDialog_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiDirectoryDialog *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiDirectoryDialog *This);
    UINTN (ANXAPI *Release)(ITuiDirectoryDialog *This);

    /**
      Render the dialog.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiDirectoryDialog *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y
    );

    /**
      Handle keyboard input.
    **/
    HRESULT (ANXAPI *HandleKey)(
        ITuiDirectoryDialog *This,
        TUI_KEY Key
    );

    /**
      Standard widget methods.
    **/
    HRESULT (ANXAPI *SetBounds)(ITuiDirectoryDialog *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiDirectoryDialog *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiDirectoryDialog *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiDirectoryDialog *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiDirectoryDialog *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiDirectoryDialog *This);

    /**
      Set initial directory to display.
    **/
    HRESULT (ANXAPI *SetInitialDirectory)(
        ITuiDirectoryDialog *This,
        CONST CHAR8 *Path
    );

    /**
      Get selected directory path.
    **/
    HRESULT (ANXAPI *GetSelectedDirectory)(
        ITuiDirectoryDialog *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Show dialog modally and return result.
    **/
    HRESULT (ANXAPI *Show)(
        ITuiDirectoryDialog *This,
        ITuiScreen *Screen,
        BOOLEAN *Result
    );

} ITuiDirectoryDialog_Vtbl;

struct _ITuiDirectoryDialog {
    CONST ITuiDirectoryDialog_Vtbl *Vtbl;
};

// {5B6C7D8E-9F0A-1B2C-3D4E-5F6A7B8C9D0E}
DEFINE_GUID(IID_ITuiTerminal,
    0x5B6C7D8E, 0x9F0A, 0x1B2C, 0x3D, 0x4E, 0x5F, 0x6A, 0x7B, 0x8C, 0x9D, 0x0E);

/**
  Terminal Render Callback

  Custom callback for rendering terminal cells.

  @param[in] UserData  User-provided context data.
  @param[in] Screen    Screen interface for rendering.
  @param[in] X         X coordinate to render at.
  @param[in] Y         Y coordinate to render at.
  @param[in] Cell      Terminal cell data (opaque structure).

  @retval S_OK  Rendering successful.
**/
typedef HRESULT (*TerminalRenderCallback)(
    VOID *UserData,
    ITuiScreen *Screen,
    INT32 X,
    INT32 Y,
    CONST VOID *Cell
);

/**
  Terminal Parser Callback

  Custom callback for parsing escape sequences.

  @param[in] UserData  User-provided context data.
  @param[in] Sequence  Escape sequence string.
  @param[in] Length    Length of sequence.

  @retval S_OK  Parsing successful.
**/
typedef HRESULT (*TerminalParserCallback)(
    VOID *UserData,
    CONST CHAR8 *Sequence,
    UINTN Length
);

/**
  ITuiTerminal Interface

  Terminal emulator widget with customizable renderer and parser
  for ANSI/VT100 escape sequences.
**/
typedef struct _ITuiTerminal_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiTerminal *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiTerminal *This);
    UINTN (ANXAPI *Release)(ITuiTerminal *This);

    /**
      Render the terminal.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiTerminal *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y,
        BOOLEAN Focused
    );

    /**
      Handle keyboard input.
    **/
    HRESULT (ANXAPI *HandleKey)(
        ITuiTerminal *This,
        TUI_KEY Key
    );

    /**
      Standard widget methods.
    **/
    HRESULT (ANXAPI *SetBounds)(ITuiTerminal *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiTerminal *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiTerminal *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiTerminal *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiTerminal *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiTerminal *This);

    /**
      Write text to terminal (will be parsed for escape sequences).
    **/
    HRESULT (ANXAPI *WriteText)(
        ITuiTerminal *This,
        CONST CHAR8 *Text,
        UINTN Length
    );

    /**
      Clear terminal screen.
    **/
    HRESULT (ANXAPI *Clear)(ITuiTerminal *This);

    /**
      Set terminal size (columns and rows).
    **/
    HRESULT (ANXAPI *SetSize)(
        ITuiTerminal *This,
        UINT32 Cols,
        UINT32 Rows
    );

    /**
      Set custom renderer callback.
    **/
    HRESULT (ANXAPI *SetRenderer)(
        ITuiTerminal *This,
        TerminalRenderCallback Renderer,
        VOID *UserData
    );

    /**
      Set custom parser callback.
    **/
    HRESULT (ANXAPI *SetParser)(
        ITuiTerminal *This,
        TerminalParserCallback Parser,
        VOID *UserData
    );

    /**
      Set input callback (for handling user input).
    **/
    HRESULT (ANXAPI *SetInputCallback)(
        ITuiTerminal *This,
        HRESULT (*Callback)(VOID *UserData, CONST CHAR8 *Input, UINTN Length),
        VOID *UserData
    );

} ITuiTerminal_Vtbl;

struct _ITuiTerminal {
    CONST ITuiTerminal_Vtbl *Vtbl;
};

// {6C7D8E9F-0A1B-2C3D-4E5F-6A7B8C9D0E1F}
DEFINE_GUID(IID_ITuiTreeView,
    0x6C7D8E9F, 0x0A1B, 0x2C3D, 0x4E, 0x5F, 0x6A, 0x7B, 0x8C, 0x9D, 0x0E, 0x1F);

/**
  ITuiTreeView Interface

  Hierarchical tree control with expand/collapse, checkboxes,
  inline editing, and keyboard navigation.
**/
typedef struct _ITuiTreeView_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiTreeView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiTreeView *This);
    UINTN (ANXAPI *Release)(ITuiTreeView *This);

    /**
      Render the tree view.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiTreeView *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y,
        UINT32 Width,
        UINT32 Height
    );

    /**
      Handle keyboard input.
    **/
    HRESULT (ANXAPI *HandleKey)(
        ITuiTreeView *This,
        TUI_KEY Key
    );

    /**
      Standard widget methods.
    **/
    HRESULT (ANXAPI *SetBounds)(ITuiTreeView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiTreeView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiTreeView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiTreeView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiTreeView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiTreeView *This);

    /**
      Add a root node.
    **/
    HRESULT (ANXAPI *AddNode)(
        ITuiTreeView *This,
        CONST CHAR8 *Text,
        VOID *UserData,
        VOID **OutHandle
    );

    /**
      Add a child node.
    **/
    HRESULT (ANXAPI *AddChildNode)(
        ITuiTreeView *This,
        VOID *ParentHandle,
        CONST CHAR8 *Text,
        VOID *UserData,
        VOID **OutHandle
    );

    /**
      Set node checkbox state.
    **/
    HRESULT (ANXAPI *SetNodeCheckbox)(
        ITuiTreeView *This,
        VOID *NodeHandle,
        BOOLEAN HasCheckbox,
        UINT8 CheckState
    );

    /**
      Set node icon.
    **/
    HRESULT (ANXAPI *SetNodeIcon)(
        ITuiTreeView *This,
        VOID *NodeHandle,
        UINT32 Icon
    );

    /**
      Expand or collapse node.
    **/
    HRESULT (ANXAPI *ExpandNode)(
        ITuiTreeView *This,
        VOID *NodeHandle,
        BOOLEAN Expand
    );

    /**
      Clear all nodes.
    **/
    HRESULT (ANXAPI *Clear)(ITuiTreeView *This);

    /**
      Enable virtual mode for handling millions of items.
      When enabled, tree data is fetched via callback on-demand.
    **/
    HRESULT (ANXAPI *SetVirtualMode)(
        ITuiTreeView *This,
        BOOLEAN Enable,
        UINT32 ItemCount,
        HRESULT (*Callback)(VOID *UserData, UINT32 Index, VOID *OutData),
        VOID *UserData
    );

} ITuiTreeView_Vtbl;

struct _ITuiTreeView {
    CONST ITuiTreeView_Vtbl *Vtbl;
};

// {7D8E9F0A-1B2C-3D4E-5F6A-7B8C9D0E1F2A}
DEFINE_GUID(IID_ITuiListView,
    0x7D8E9F0A, 0x1B2C, 0x3D4E, 0x5F, 0x6A, 0x7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A);

/**
  ITuiListView Interface

  Multi-column list control with resizable columns, different view modes,
  alternating rows, checkboxes, and inline editing.
**/
typedef struct _ITuiListView_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiListView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiListView *This);
    UINTN (ANXAPI *Release)(ITuiListView *This);

    /**
      Render the list view.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiListView *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y,
        UINT32 Width,
        UINT32 Height
    );

    /**
      Handle keyboard input.
    **/
    HRESULT (ANXAPI *HandleKey)(
        ITuiListView *This,
        TUI_KEY Key
    );

    /**
      Standard widget methods.
    **/
    HRESULT (ANXAPI *SetBounds)(ITuiListView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiListView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiListView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiListView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiListView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiListView *This);

    /**
      Add a column.
    **/
    HRESULT (ANXAPI *AddColumn)(
        ITuiListView *This,
        CONST CHAR8 *Header,
        UINT32 Width
    );

    /**
      Add an item with cell data.
    **/
    HRESULT (ANXAPI *AddItem)(
        ITuiListView *This,
        CONST CHAR8 **Cells,
        UINT32 CellCount,
        VOID *UserData,
        UINT32 *OutIndex
    );

    /**
      Clear all items.
    **/
    HRESULT (ANXAPI *Clear)(ITuiListView *This);

    /**
      Set view mode (list, details, icons, column browse).
    **/
    HRESULT (ANXAPI *SetMode)(
        ITuiListView *This,
        UINT32 Mode
    );

    /**
      Set column width.
    **/
    HRESULT (ANXAPI *SetColumnWidth)(
        ITuiListView *This,
        UINT32 ColumnIndex,
        UINT32 Width
    );

    /**
      Enable virtual mode for handling millions of items.
      When enabled, list data is fetched via callback on-demand.
    **/
    HRESULT (ANXAPI *SetVirtualMode)(
        ITuiListView *This,
        BOOLEAN Enable,
        UINT32 ItemCount,
        HRESULT (*Callback)(VOID *UserData, UINT32 Index, CHAR8 **OutCells, UINT32 *OutCellCount, BOOLEAN *OutChecked),
        VOID *UserData
    );

} ITuiListView_Vtbl;

struct _ITuiListView {
    CONST ITuiListView_Vtbl *Vtbl;
};

// {8E9F0A1B-2C3D-4E5F-6A7B-8C9D0E1F2A3B}
DEFINE_GUID(IID_ITuiWizard,
    0x8E9F0A1B, 0x2C3D, 0x4E5F, 0x6A, 0x7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B);

/**
  ITuiWizard Interface

  Multi-step workflow dialog with Back/Next/Finish/Cancel buttons,
  progress indicator, and page validation.
**/
typedef struct _ITuiWizard_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiWizard *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiWizard *This);
    UINTN (ANXAPI *Release)(ITuiWizard *This);

    /**
      Render the wizard.
    **/
    HRESULT (ANXAPI *Render)(
        ITuiWizard *This,
        ITuiScreen *Screen,
        INT32 X,
        INT32 Y
    );

    /**
      Handle keyboard input.
    **/
    HRESULT (ANXAPI *HandleKey)(
        ITuiWizard *This,
        TUI_KEY Key
    );

    /**
      Standard widget methods.
    **/
    HRESULT (ANXAPI *SetBounds)(ITuiWizard *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiWizard *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiWizard *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiWizard *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiWizard *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiWizard *This);

    /**
      Add a page to the wizard.

      @param[in] Title        Page title.
      @param[in] Description  Page description.
      @param[in] PageWidget   Page content widget (can be NULL).
      @param[in] OnEnter      Callback when entering page (can be NULL).
      @param[in] OnLeave      Callback when leaving page (can be NULL).
      @param[in] OnValidate   Callback to validate page (can be NULL).
      @param[in] UserData     User data for callbacks.
    **/
    HRESULT (ANXAPI *AddPage)(
        ITuiWizard *This,
        CONST CHAR8 *Title,
        CONST CHAR8 *Description,
        VOID *PageWidget,
        HRESULT (*OnEnter)(VOID *PageWidget, VOID *UserData),
        HRESULT (*OnLeave)(VOID *PageWidget, VOID *UserData, BOOLEAN *AllowLeave),
        HRESULT (*OnValidate)(VOID *PageWidget, VOID *UserData, BOOLEAN *IsValid),
        VOID *UserData
    );

    /**
      Go to next page.
    **/
    HRESULT (ANXAPI *GoNext)(ITuiWizard *This);

    /**
      Go to previous page.
    **/
    HRESULT (ANXAPI *GoBack)(ITuiWizard *This);

    /**
      Finish the wizard.
    **/
    HRESULT (ANXAPI *Finish)(ITuiWizard *This);

    /**
      Reset wizard to first page.
    **/
    HRESULT (ANXAPI *Reset)(ITuiWizard *This);

    /**
      Set finish callback.
    **/
    HRESULT (ANXAPI *SetFinishCallback)(
        ITuiWizard *This,
        HRESULT (*Callback)(VOID *UserData),
        VOID *UserData
    );

} ITuiWizard_Vtbl;

struct _ITuiWizard {
    CONST ITuiWizard_Vtbl *Vtbl;
};

// {9F0A1B2C-3D4E-5F6A-7B8C-9D0E1F2A3B4C}
DEFINE_GUID(IID_ITuiFlexContainer,
    0x9F0A1B2C, 0x3D4E, 0x5F6A, 0x7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C);

/**
  ITuiFlexContainer Interface

  Flexbox-like layout container with support for row/column direction,
  flex-grow/shrink, alignment, justify content, wrapping, and gaps.
**/
typedef struct _ITuiFlexContainer_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiFlexContainer *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiFlexContainer *This);
    UINTN (ANXAPI *Release)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *Render)(ITuiFlexContainer *This, ITuiScreen *Screen, INT32 X, INT32 Y);
    HRESULT (ANXAPI *SetBounds)(ITuiFlexContainer *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiFlexContainer *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiFlexContainer *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiFlexContainer *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *AddChild)(ITuiFlexContainer *This, VOID *Widget, UINT32 FlexGrow, UINT32 FlexShrink, INT32 FlexBasis);
    HRESULT (ANXAPI *RemoveChild)(ITuiFlexContainer *This, VOID *Widget);
    HRESULT (ANXAPI *SetDirection)(ITuiFlexContainer *This, UINT32 Direction);
    HRESULT (ANXAPI *SetJustifyContent)(ITuiFlexContainer *This, UINT32 Justify);
    HRESULT (ANXAPI *SetAlignItems)(ITuiFlexContainer *This, UINT32 Align);
    HRESULT (ANXAPI *SetGap)(ITuiFlexContainer *This, UINT32 Gap);
    HRESULT (ANXAPI *SetPadding)(ITuiFlexContainer *This, UINT32 Top, UINT32 Right, UINT32 Bottom, UINT32 Left);
} ITuiFlexContainer_Vtbl;

struct _ITuiFlexContainer {
    CONST ITuiFlexContainer_Vtbl *Vtbl;
};

// {A0B1C2D3-4E5F-6A7B-8C9D-0E1F2A3B4C5D}
DEFINE_GUID(IID_ITuiVBox,
    0xA0B1C2D3, 0x4E5F, 0x6A7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D);

/**
  ITuiVBox Interface

  Vertical box container for simple top-to-bottom stacking.
**/
typedef struct _ITuiVBox_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiVBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiVBox *This);
    UINTN (ANXAPI *Release)(ITuiVBox *This);
    HRESULT (ANXAPI *Render)(ITuiVBox *This, ITuiScreen *Screen, INT32 X, INT32 Y);
    HRESULT (ANXAPI *SetBounds)(ITuiVBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiVBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiVBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiVBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiVBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiVBox *This);
    HRESULT (ANXAPI *PackStart)(ITuiVBox *This, VOID *Widget, BOOLEAN Expand, BOOLEAN Fill, UINT32 Padding);
    HRESULT (ANXAPI *SetSpacing)(ITuiVBox *This, UINT32 Spacing);
    HRESULT (ANXAPI *SetHomogeneous)(ITuiVBox *This, BOOLEAN Homogeneous);
} ITuiVBox_Vtbl;

struct _ITuiVBox {
    CONST ITuiVBox_Vtbl *Vtbl;
};

// {B1C2D3E4-5F6A-7B8C-9D0E-1F2A3B4C5D6E}
DEFINE_GUID(IID_ITuiHBox,
    0xB1C2D3E4, 0x5F6A, 0x7B8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E);

/**
  ITuiHBox Interface

  Horizontal box container for simple left-to-right stacking.
**/
typedef struct _ITuiHBox_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiHBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiHBox *This);
    UINTN (ANXAPI *Release)(ITuiHBox *This);
    HRESULT (ANXAPI *Render)(ITuiHBox *This, ITuiScreen *Screen, INT32 X, INT32 Y);
    HRESULT (ANXAPI *SetBounds)(ITuiHBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiHBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiHBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiHBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiHBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiHBox *This);
    HRESULT (ANXAPI *PackStart)(ITuiHBox *This, VOID *Widget, BOOLEAN Expand, BOOLEAN Fill, UINT32 Padding);
    HRESULT (ANXAPI *SetSpacing)(ITuiHBox *This, UINT32 Spacing);
    HRESULT (ANXAPI *SetHomogeneous)(ITuiHBox *This, BOOLEAN Homogeneous);
} ITuiHBox_Vtbl;

struct _ITuiHBox {
    CONST ITuiHBox_Vtbl *Vtbl;
};

// {C2D3E4F5-6A7B-8C9D-0E1F-2A3B4C5D6E7F}
DEFINE_GUID(IID_ITuiGrid,
    0xC2D3E4F5, 0x6A7B, 0x8C9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F);

/**
  ITuiGrid Interface

  Grid layout container that arranges children in rows and columns
  with support for spanning, padding, and alignment.
**/
typedef struct _ITuiGrid_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiGrid *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiGrid *This);
    UINTN (ANXAPI *Release)(ITuiGrid *This);
    HRESULT (ANXAPI *Render)(ITuiGrid *This, ITuiScreen *Screen, INT32 X, INT32 Y);
    HRESULT (ANXAPI *SetBounds)(ITuiGrid *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiGrid *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiGrid *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiGrid *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiGrid *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiGrid *This);
    HRESULT (ANXAPI *Attach)(ITuiGrid *This, VOID *Widget, UINT32 Column, UINT32 Row, UINT32 ColumnSpan, UINT32 RowSpan);
    HRESULT (ANXAPI *SetSpacing)(ITuiGrid *This, UINT32 RowSpacing, UINT32 ColumnSpacing);
    HRESULT (ANXAPI *SetRowHeight)(ITuiGrid *This, UINT32 Row, UINT32 Height);
    HRESULT (ANXAPI *SetColumnWidth)(ITuiGrid *This, UINT32 Column, UINT32 Width);
} ITuiGrid_Vtbl;

struct _ITuiGrid {
    CONST ITuiGrid_Vtbl *Vtbl;
};

// {D3E4F5A6-7B8C-9D0E-1F2A-3B4C5D6E7F8A}
DEFINE_GUID(IID_ITuiSplitView,
    0xD3E4F5A6, 0x7B8C, 0x9D0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A);

/**
  ITuiSplitView Interface

  Resizable two-pane container with draggable divider.
  Supports horizontal and vertical orientation.
**/
typedef struct _ITuiSplitView_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiSplitView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiSplitView *This);
    UINTN (ANXAPI *Release)(ITuiSplitView *This);
    HRESULT (ANXAPI *Render)(ITuiSplitView *This, ITuiScreen *Screen, INT32 X, INT32 Y);
    HRESULT (ANXAPI *HandleMouse)(ITuiSplitView *This, CONST TUI_MOUSE_EVENT *Event);
    HRESULT (ANXAPI *SetBounds)(ITuiSplitView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiSplitView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiSplitView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiSplitView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiSplitView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiSplitView *This);
    HRESULT (ANXAPI *SetPane1)(ITuiSplitView *This, VOID *Widget);
    HRESULT (ANXAPI *SetPane2)(ITuiSplitView *This, VOID *Widget);
    HRESULT (ANXAPI *SetSplitPosition)(ITuiSplitView *This, UINT32 Position);
    HRESULT (ANXAPI *SetOrientation)(ITuiSplitView *This, UINT32 Orientation);
} ITuiSplitView_Vtbl;

struct _ITuiSplitView {
    CONST ITuiSplitView_Vtbl *Vtbl;
};

// {E4F5A6B7-8C9D-0E1F-2A3B-4C5D6E7F8A9B}
DEFINE_GUID(IID_ITuiSurface,
    0xE4F5A6B7, 0x8C9D, 0x0E1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B);

/**
  ITuiSurface Interface

  Drawing surface abstraction providing clipping, fill/stroke operations,
  and buffering capabilities.
**/
typedef struct _ITuiSurface_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiSurface *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiSurface *This);
    UINTN (ANXAPI *Release)(ITuiSurface *This);
    HRESULT (ANXAPI *SetClipRect)(ITuiSurface *This, CONST TUI_RECT *Rect);
    HRESULT (ANXAPI *GetClipRect)(ITuiSurface *This, TUI_RECT *Rect);
    HRESULT (ANXAPI *SetChar)(ITuiSurface *This, INT32 X, INT32 Y, CHAR16 Character, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetChar)(ITuiSurface *This, INT32 X, INT32 Y, CHAR16 *Character, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetAttributes)(ITuiSurface *This, INT32 X, INT32 Y, UINT8 Attributes);
    HRESULT (ANXAPI *FillRect)(ITuiSurface *This, CONST TUI_RECT *Rect, CHAR16 Character, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *StrokeRect)(ITuiSurface *This, CONST TUI_RECT *Rect, TUI_BORDER_STYLE Style, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *DrawLine)(ITuiSurface *This, INT32 X1, INT32 Y1, INT32 X2, INT32 Y2, CHAR16 Character, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *WriteText)(ITuiSurface *This, INT32 X, INT32 Y, CONST CHAR8 *Text, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *Clear)(ITuiSurface *This, TUI_COLOR Background);
    HRESULT (ANXAPI *Flush)(ITuiSurface *This, INT32 OffsetX, INT32 OffsetY);
} ITuiSurface_Vtbl;

struct _ITuiSurface {
    CONST ITuiSurface_Vtbl *Vtbl;
};

// {F5A6B7C8-9D0E-1F2A-3B4C-5D6E7F8A9B0C}
DEFINE_GUID(IID_ITuiPropertySheet,
    0xF5A6B7C8, 0x9D0E, 0x1F2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C);

/**
  ITuiPropertySheet Interface

  Tabbed dialog with multiple property pages, OK/Cancel/Apply buttons,
  validation, and modified state tracking.
**/
typedef struct _ITuiPropertySheet_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiPropertySheet *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiPropertySheet *This);
    UINTN (ANXAPI *Release)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *Render)(ITuiPropertySheet *This, ITuiScreen *Screen, INT32 X, INT32 Y);
    HRESULT (ANXAPI *HandleKey)(ITuiPropertySheet *This, TUI_KEY Key);
    HRESULT (ANXAPI *SetBounds)(ITuiPropertySheet *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiPropertySheet *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiPropertySheet *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiPropertySheet *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *AddPage)(ITuiPropertySheet *This, CONST CHAR8 *Title, CONST CHAR8 *Description, VOID *PageWidget, HRESULT (*OnActivate)(VOID*, VOID*), HRESULT (*OnDeactivate)(VOID*, VOID*), HRESULT (*OnApply)(VOID*, VOID*, BOOLEAN*), HRESULT (*OnValidate)(VOID*, VOID*, BOOLEAN*), HRESULT (*OnReset)(VOID*, VOID*), VOID *UserData);
    HRESULT (ANXAPI *SetActivePage)(ITuiPropertySheet *This, INT32 PageIndex);
    HRESULT (ANXAPI *SetPageModified)(ITuiPropertySheet *This, INT32 PageIndex, BOOLEAN Modified);
    HRESULT (ANXAPI *Apply)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *OK)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *Cancel)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *Reset)(ITuiPropertySheet *This);
} ITuiPropertySheet_Vtbl;

struct _ITuiPropertySheet {
    CONST ITuiPropertySheet_Vtbl *Vtbl;
};

// {A6B7C8D9-0E1F-2A3B-4C5D-6E7F8A9B0C1D}
DEFINE_GUID(IID_ITuiSpreadsheet,
    0xA6B7C8D9, 0x0E1F, 0x2A3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D);

/**
  ITuiSpreadsheet Interface

  Excel-like spreadsheet control with virtual storage, formula evaluation,
  cell editing, selection, freeze panes, and formatting.
**/
typedef struct _ITuiSpreadsheet_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiSpreadsheet *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiSpreadsheet *This);
    UINTN (ANXAPI *Release)(ITuiSpreadsheet *This);
    HRESULT (ANXAPI *Render)(ITuiSpreadsheet *This, ITuiScreen *Screen, INT32 X, INT32 Y);
    HRESULT (ANXAPI *HandleKey)(ITuiSpreadsheet *This, TUI_KEY Key);
    HRESULT (ANXAPI *SetBounds)(ITuiSpreadsheet *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiSpreadsheet *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiSpreadsheet *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiSpreadsheet *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiSpreadsheet *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiSpreadsheet *This);
    HRESULT (ANXAPI *SetCellValue)(ITuiSpreadsheet *This, UINT32 Row, UINT32 Col, CONST CHAR8 *Value);
    HRESULT (ANXAPI *GetCellValue)(ITuiSpreadsheet *This, UINT32 Row, UINT32 Col, CHAR8 *Value, UINTN ValueSize);
    HRESULT (ANXAPI *SetColumnWidth)(ITuiSpreadsheet *This, UINT32 Column, UINT32 Width);
    HRESULT (ANXAPI *SetVirtualMode)(ITuiSpreadsheet *This, BOOLEAN Enable, HRESULT (*OnGetCell)(VOID*, UINT32, UINT32, VOID*), HRESULT (*OnSetCell)(VOID*, UINT32, UINT32, CONST VOID*), VOID *UserData);
} ITuiSpreadsheet_Vtbl;

struct _ITuiSpreadsheet {
    CONST ITuiSpreadsheet_Vtbl *Vtbl;
};

// {B7C8D9E0-1F2A-3B4C-5D6E-7F8A9B0C1D2E}
DEFINE_GUID(IID_ITuiRuler,
    0xB7C8D9E0, 0x1F2A, 0x3B4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E);

/**
  ITuiRuler Interface

  Horizontal/vertical ruler for text editors showing column/line numbers,
  tab stops, margins, and cursor position.
**/
typedef struct _ITuiRuler_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiRuler *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiRuler *This);
    UINTN (ANXAPI *Release)(ITuiRuler *This);
    HRESULT (ANXAPI *Render)(ITuiRuler *This, ITuiScreen *Screen, INT32 X, INT32 Y);
    HRESULT (ANXAPI *SetBounds)(ITuiRuler *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiRuler *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiRuler *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiRuler *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiRuler *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiRuler *This);
    HRESULT (ANXAPI *SetMargins)(ITuiRuler *This, UINT32 LeftMargin, UINT32 RightMargin, UINT32 FirstLineIndent);
    HRESULT (ANXAPI *AddTabStop)(ITuiRuler *This, UINT32 Position, UINT32 Type);
    HRESULT (ANXAPI *ClearTabStops)(ITuiRuler *This);
    HRESULT (ANXAPI *SetCurrentPosition)(ITuiRuler *This, UINT32 Position);
} ITuiRuler_Vtbl;

struct _ITuiRuler {
    CONST ITuiRuler_Vtbl *Vtbl;
};

// {C8D9E0F1-2A3B-4C5D-6E7F-8A9B0C1D2E3F}
DEFINE_GUID(IID_ITuiRichTextEditor,
    0xC8D9E0F1, 0x2A3B, 0x4C5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F);

/**
  ITuiRichTextEditor Interface

  Full-featured rich text editor similar to Word for DOS/WordPerfect/WordStar
  with formatting, search/replace, block operations, and reveal codes.
**/
typedef struct _ITuiRichTextEditor_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiRichTextEditor *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiRichTextEditor *This);
    UINTN (ANXAPI *Release)(ITuiRichTextEditor *This);
    HRESULT (ANXAPI *Render)(ITuiRichTextEditor *This, ITuiScreen *Screen, INT32 X, INT32 Y);
    HRESULT (ANXAPI *HandleKey)(ITuiRichTextEditor *This, TUI_KEY Key);
    HRESULT (ANXAPI *SetBounds)(ITuiRichTextEditor *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiRichTextEditor *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiRichTextEditor *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiRichTextEditor *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiRichTextEditor *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiRichTextEditor *This);
    HRESULT (ANXAPI *SetText)(ITuiRichTextEditor *This, CONST CHAR8 *Text);
    HRESULT (ANXAPI *ToggleFormat)(ITuiRichTextEditor *This, UINT32 Attribute);
    HRESULT (ANXAPI *SetRuler)(ITuiRichTextEditor *This, ITuiRuler *Ruler);
} ITuiRichTextEditor_Vtbl;

struct _ITuiRichTextEditor {
    CONST ITuiRichTextEditor_Vtbl *Vtbl;
};

// {E5F6A7B8-C9D0-4E1F-2A3B-4C5D6E7F8A9B}
DEFINE_GUID(IID_ITuiHeaderView,
    0xE5F6A7B8, 0xC9D0, 0x4E1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B);

/**
  ITuiHeaderView Interface

  Reusable column header control for list views, tree views, and spreadsheets.
  Features sortable, resizable, and reorderable columns.
**/
typedef struct _ITuiHeaderView_Vtbl {
    HRESULT (ANXAPI *QueryInterface)(ITuiHeaderView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiHeaderView *This);
    UINTN (ANXAPI *Release)(ITuiHeaderView *This);
    HRESULT (ANXAPI *Render)(ITuiHeaderView *This, ITuiScreen *Screen, INT32 X, INT32 Y);
    HRESULT (ANXAPI *HandleMouse)(ITuiHeaderView *This, CONST TUI_MOUSE_EVENT *Event);
    HRESULT (ANXAPI *SetBounds)(ITuiHeaderView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiHeaderView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiHeaderView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiHeaderView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiHeaderView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiHeaderView *This);
    HRESULT (ANXAPI *AddSection)(ITuiHeaderView *This, CONST CHAR8 *Title, UINT32 Width);
    HRESULT (ANXAPI *SetSortIndicator)(ITuiHeaderView *This, UINT32 SectionIndex, UINT32 SortOrder);
    HRESULT (ANXAPI *GetSectionWidth)(ITuiHeaderView *This, UINT32 SectionIndex, UINT32 *OutWidth);
    HRESULT (ANXAPI *SetCallbacks)(ITuiHeaderView *This, HRESULT (*OnSectionClicked)(VOID*, UINT32), HRESULT (*OnSectionResized)(VOID*, UINT32, UINT32), HRESULT (*OnSectionMoved)(VOID*, UINT32, UINT32), VOID *UserData);
} ITuiHeaderView_Vtbl;

struct _ITuiHeaderView {
    CONST ITuiHeaderView_Vtbl *Vtbl;
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

/**
  Create a TUI Tab Control instance.

  @param[out] TabControl  Pointer to receive the tab control interface.

  @retval S_OK        Tab control created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateTabControl(
    OUT ITuiTabControl **TabControl
);

/**
  Create a TUI Progress Bar instance.

  @param[out] ProgressBar  Pointer to receive the progress bar interface.

  @retval S_OK        Progress bar created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateProgressBar(
    OUT ITuiProgressBar **ProgressBar
);

/**
  Create a TUI Color Picker instance.

  @param[out] ColorPicker  Pointer to receive the color picker interface.

  @retval S_OK        Color picker created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateColorPicker(
    OUT ITuiColorPicker **ColorPicker
);

/**
  Create a TUI Group Box instance.

  @param[in]  Title     Group box title.
  @param[out] GroupBox  Pointer to receive the group box interface.

  @retval S_OK        Group box created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateGroupBox(
    IN  CONST CHAR8 *Title,
    OUT ITuiGroupBox **GroupBox
);

/**
  Create a TUI Focus Manager instance.

  @param[out] FocusManager  Pointer to receive the focus manager interface.

  @retval S_OK        Focus manager created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateFocusManager(
    OUT ITuiFocusManager **FocusManager
);

/**
  Create a TUI Label instance.

  @param[in]  Text   Label text.
  @param[out] Label  Pointer to receive the label interface.

  @retval S_OK        Label created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateLabel(
    IN  CONST CHAR8 *Text,
    OUT ITuiLabel **Label
);

/**
  Create a TUI Text Editor instance.

  @param[out] Editor  Pointer to receive the text editor interface.

  @retval S_OK        Text editor created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateTextEditor(
    OUT ITuiTextEditor **Editor
);

/**
  Create a TUI Scroll View instance.

  @param[out] ScrollView  Pointer to receive the scroll view interface.

  @retval S_OK        Scroll view created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateScrollView(
    OUT ITuiScrollView **ScrollView
);

/**
  Create a TUI Long Operation Dialog instance.

  @param[in]  Title       Dialog title.
  @param[out] OutDialog   Pointer to receive the dialog interface.

  @retval S_OK        Dialog created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateLongOpDialog(
    IN  CONST CHAR8 *Title,
    OUT ITuiLongOpDialog **OutDialog
);

/**
  Create a TUI Directory Dialog instance.

  @param[in]  Title       Dialog title.
  @param[out] OutDialog   Pointer to receive the dialog interface.

  @retval S_OK        Dialog created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateDirectoryDialog(
    IN  CONST CHAR8 *Title,
    OUT ITuiDirectoryDialog **OutDialog
);

/**
  Create a TUI Terminal instance.

  @param[in]  Cols         Number of columns.
  @param[in]  Rows         Number of rows.
  @param[out] OutTerminal  Pointer to receive the terminal interface.

  @retval S_OK        Terminal created successfully.
  @retval E_INVALIDARG  Invalid dimensions.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateTerminal(
    IN  UINT32 Cols,
    IN  UINT32 Rows,
    OUT ITuiTerminal **OutTerminal
);

/**
  Create a TUI Tree View instance.

  @param[out] OutTreeView  Pointer to receive the tree view interface.

  @retval S_OK        Tree view created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateTreeView(
    OUT ITuiTreeView **OutTreeView
);

/**
  Create a TUI List View instance.

  @param[out] OutListView  Pointer to receive the list view interface.

  @retval S_OK        List view created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateListView(
    OUT ITuiListView **OutListView
);

/**
  Create a TUI Wizard instance.

  @param[out] OutWizard  Pointer to receive the wizard interface.

  @retval S_OK        Wizard created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateWizard(
    OUT ITuiWizard **OutWizard
);

/**
  Create a TUI Flex Container instance.

  @param[out] OutContainer  Pointer to receive the flex container interface.

  @retval S_OK        Flex container created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateFlexContainer(
    OUT ITuiFlexContainer **OutContainer
);

/**
  Create a TUI VBox instance.

  @param[in]  Homogeneous  TRUE for equal-height children.
  @param[in]  Spacing      Spacing between children.
  @param[out] OutVBox      Pointer to receive the VBox interface.

  @retval S_OK        VBox created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateVBox(
    IN  BOOLEAN Homogeneous,
    IN  UINT32 Spacing,
    OUT ITuiVBox **OutVBox
);

/**
  Create a TUI HBox instance.

  @param[in]  Homogeneous  TRUE for equal-width children.
  @param[in]  Spacing      Spacing between children.
  @param[out] OutHBox      Pointer to receive the HBox interface.

  @retval S_OK        HBox created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateHBox(
    IN  BOOLEAN Homogeneous,
    IN  UINT32 Spacing,
    OUT ITuiHBox **OutHBox
);

/**
  Create a TUI Grid instance.

  @param[in]  Rows      Number of rows.
  @param[in]  Columns   Number of columns.
  @param[out] OutGrid   Pointer to receive the grid interface.

  @retval S_OK        Grid created successfully.
  @retval E_INVALIDARG  Invalid dimensions.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateGrid(
    IN  UINT32 Rows,
    IN  UINT32 Columns,
    OUT ITuiGrid **OutGrid
);

/**
  Create a TUI Split View instance.

  @param[in]  Orientation     0=Horizontal, 1=Vertical.
  @param[in]  InitialPosition Initial divider position.
  @param[out] OutSplitView    Pointer to receive the split view interface.

  @retval S_OK        Split view created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateSplitView(
    IN  UINT32 Orientation,
    IN  UINT32 InitialPosition,
    OUT ITuiSplitView **OutSplitView
);

/**
  Create a TUI Drawing Surface instance.

  @param[in]  Width      Surface width.
  @param[in]  Height     Surface height.
  @param[in]  Screen     Optional screen for flushing (can be NULL).
  @param[out] OutSurface Pointer to receive the surface interface.

  @retval S_OK        Surface created successfully.
  @retval E_INVALIDARG  Invalid dimensions.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateSurface(
    IN  UINT32 Width,
    IN  UINT32 Height,
    IN  ITuiScreen *Screen,
    OUT ITuiSurface **OutSurface
);

/**
  Create a TUI Property Sheet instance.

  @param[out] OutPropertySheet  Pointer to receive the property sheet interface.

  @retval S_OK        Property sheet created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreatePropertySheet(
    OUT ITuiPropertySheet **OutPropertySheet
);

/**
  Create a TUI Spreadsheet instance.

  @param[in]  Rows            Number of rows.
  @param[in]  Columns         Number of columns.
  @param[out] OutSpreadsheet  Pointer to receive the spreadsheet interface.

  @retval S_OK        Spreadsheet created successfully.
  @retval E_INVALIDARG  Invalid dimensions.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateSpreadsheet(
    IN  UINT32 Rows,
    IN  UINT32 Columns,
    OUT ITuiSpreadsheet **OutSpreadsheet
);

/**
  Create a TUI Ruler instance.

  @param[in]  Orientation  0=Horizontal, 1=Vertical.
  @param[in]  Length       Ruler length in units.
  @param[out] OutRuler     Pointer to receive the ruler interface.

  @retval S_OK        Ruler created successfully.
  @retval E_INVALIDARG  Invalid parameters.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateRuler(
    IN  UINT32 Orientation,
    IN  UINT32 Length,
    OUT ITuiRuler **OutRuler
);

/**
  Create a TUI Rich Text Editor instance.

  @param[out] OutEditor  Pointer to receive the editor interface.

  @retval S_OK        Editor created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateRichTextEditor(
    OUT ITuiRichTextEditor **OutEditor
);

/**
  Create a TUI Header View instance.

  @param[out] OutHeaderView  Pointer to receive the header view interface.

  @retval S_OK        Header view created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateHeaderView(
    OUT ITuiHeaderView **OutHeaderView
);

/**
  Create a TUI Window Manager instance.

  @param[out] OutWindowManager  Pointer to receive the window manager interface.

  @retval S_OK        Window manager created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateWindowManager(
    OUT ITuiWindowManager **OutWindowManager
);

/**
  Create a TUI Composer instance.

  @param[out] OutComposer  Pointer to receive the composer interface.

  @retval S_OK        Composer created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxTuiCreateComposer(
    OUT ITuiComposer **OutComposer
);

/**
  Create a YAML Serializer instance.

  @param[out] OutSerializer  Pointer to receive the serializer interface.

  @retval S_OK        Serializer created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxAmxCreateSerializer(
    OUT IAmxSerializer **OutSerializer
);

/**
  Create a Resource Manager instance.

  @param[out] OutManager  Pointer to receive the resource manager interface.

  @retval S_OK        Resource manager created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxAmxCreateResourceManager(
    OUT IAmxResourceManager **OutManager
);

#ifdef __cplusplus
}
#endif

#endif /* __ANANKE_TUI_H__ */
