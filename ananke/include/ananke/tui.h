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

// {5F6A7B8C-9D0E-1F2A-3B4C-5D6E7F8A9B0C}
DEFINE_GUID(IID_ITuiThemedMenu,
    0x5F6A7B8C, 0x9D0E, 0x1F2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C);

/**
  ITuiThemedMenu Interface

  Menu theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedMenu_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedMenu *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedMenu *This);
    UINTN (ANXAPI *Release)(ITuiThemedMenu *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedMenu *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedMenu *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedMenu *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedMenu *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedMenu *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedMenu *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedMenu *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedMenu *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedMenu *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedMenu *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedMenu *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedMenu *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedMenu *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedMenu *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedMenu *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedMenu *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedMenu *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedMenu *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedMenu *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedMenu *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedMenu *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedMenu *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedMenu *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedMenu *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedMenu *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedMenu *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedMenu methods
    HRESULT (ANXAPI *GetHotkeyColor)(ITuiThemedMenu *This, TUI_COLOR *HotkeyColor);
    HRESULT (ANXAPI *SetHotkeyColor)(ITuiThemedMenu *This, TUI_COLOR HotkeyColor);
    HRESULT (ANXAPI *GetAcceleratorColor)(ITuiThemedMenu *This, TUI_COLOR *AcceleratorColor);
    HRESULT (ANXAPI *SetAcceleratorColor)(ITuiThemedMenu *This, TUI_COLOR AcceleratorColor);
    HRESULT (ANXAPI *GetSelectedColors)(ITuiThemedMenu *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectedColors)(ITuiThemedMenu *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetDisabledColor)(ITuiThemedMenu *This, TUI_COLOR *DisabledColor);
    HRESULT (ANXAPI *SetDisabledColor)(ITuiThemedMenu *This, TUI_COLOR DisabledColor);
    CHAR8 (ANXAPI *GetSeparatorChar)(ITuiThemedMenu *This);
    HRESULT (ANXAPI *SetSeparatorChar)(ITuiThemedMenu *This, CHAR8 SeparatorChar);
} ITuiThemedMenu_Vtbl;

struct _ITuiThemedMenu {
    CONST ITuiThemedMenu_Vtbl *Vtbl;
};

/**
  ITuiMenu Interface

  Represents an interactive menu. Inherits from ITuiThemedMenu.
**/
typedef struct _ITuiMenu_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiMenu *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiMenu *This);
    UINTN (ANXAPI *Release)(ITuiMenu *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiMenu *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiMenu *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiMenu *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiMenu *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiMenu *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiMenu *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiMenu *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiMenu *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiMenu *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiMenu *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiMenu *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiMenu *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiMenu *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiMenu *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiMenu *This);
    HRESULT (ANXAPI *SetParent)(ITuiMenu *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiMenu *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiMenu *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiMenu *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiMenu *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiMenu *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiMenu *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiMenu *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiMenu *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiMenu *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiMenu *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedMenu methods
    HRESULT (ANXAPI *GetHotkeyColor)(ITuiMenu *This, TUI_COLOR *HotkeyColor);
    HRESULT (ANXAPI *SetHotkeyColor)(ITuiMenu *This, TUI_COLOR HotkeyColor);
    HRESULT (ANXAPI *GetAcceleratorColor)(ITuiMenu *This, TUI_COLOR *AcceleratorColor);
    HRESULT (ANXAPI *SetAcceleratorColor)(ITuiMenu *This, TUI_COLOR AcceleratorColor);
    HRESULT (ANXAPI *GetSelectedColors)(ITuiMenu *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectedColors)(ITuiMenu *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetDisabledColor)(ITuiMenu *This, TUI_COLOR *DisabledColor);
    HRESULT (ANXAPI *SetDisabledColor)(ITuiMenu *This, TUI_COLOR DisabledColor);
    CHAR8 (ANXAPI *GetSeparatorChar)(ITuiMenu *This);
    HRESULT (ANXAPI *SetSeparatorChar)(ITuiMenu *This, CHAR8 SeparatorChar);

    // ITuiMenu methods
    HRESULT (ANXAPI *AddItem)(ITuiMenu *This, TUI_MENU_ITEM_TYPE Type, CONST CHAR8 *Label, CONST CHAR8 *Help, VOID *UserData);
    HRESULT (ANXAPI *Run)(ITuiMenu *This, INT32 *SelectedIndex);
    HRESULT (ANXAPI *GetItemValue)(ITuiMenu *This, INT32 Index, VOID *Value, UINTN ValueSize);
    HRESULT (ANXAPI *SetItemValue)(ITuiMenu *This, INT32 Index, CONST VOID *Value, UINTN ValueSize);
    HRESULT (ANXAPI *SetItemHotkey)(ITuiMenu *This, INT32 Index, CHAR8 Hotkey);
    HRESULT (ANXAPI *SetItemAccelerator)(ITuiMenu *This, INT32 Index, TUI_KEY Key, BOOLEAN Ctrl, BOOLEAN Alt, BOOLEAN Shift);
    HRESULT (ANXAPI *GetItemHotkey)(ITuiMenu *This, INT32 Index, CHAR8 *Hotkey);
    HRESULT (ANXAPI *SetItemEnabled)(ITuiMenu *This, INT32 Index, BOOLEAN Enabled);
    HRESULT (ANXAPI *SetItemChecked)(ITuiMenu *This, INT32 Index, BOOLEAN Checked);
    UINT32 (ANXAPI *GetItemCount)(ITuiMenu *This);
    HRESULT (ANXAPI *GetItemLabel)(ITuiMenu *This, INT32 Index, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *RemoveItem)(ITuiMenu *This, INT32 Index);
    HRESULT (ANXAPI *ClearItems)(ITuiMenu *This);
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

// {8D9E0F1A-2B3C-4D5E-6F7A-8B9C0D1E2F3A}
DEFINE_GUID(IID_ITuiThemedRadioGroup,
    0x8D9E0F1A, 0x2B3C, 0x4D5E, 0x6F, 0x7A, 0x8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x3A);

/**
  ITuiThemedRadioGroup Interface

  Radio group theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedRadioGroup_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedRadioGroup *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedRadioGroup *This);
    UINTN (ANXAPI *Release)(ITuiThemedRadioGroup *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedRadioGroup *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedRadioGroup *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedRadioGroup *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedRadioGroup *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedRadioGroup *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedRadioGroup *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedRadioGroup *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedRadioGroup *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedRadioGroup *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedRadioGroup *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedRadioGroup *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedRadioGroup *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedRadioGroup *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedRadioGroup *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedRadioGroup *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedRadioGroup *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedRadioGroup *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedRadioGroup *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedRadioGroup *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedRadioGroup *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedRadioGroup *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedRadioGroup *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedRadioGroup *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedRadioGroup *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedRadioGroup *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedRadioGroup *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedRadioGroup methods
    HRESULT (ANXAPI *GetCheckedChar)(ITuiThemedRadioGroup *This, CHAR8 *CheckedChar);
    HRESULT (ANXAPI *SetCheckedChar)(ITuiThemedRadioGroup *This, CHAR8 CheckedChar);
    HRESULT (ANXAPI *GetUncheckedChar)(ITuiThemedRadioGroup *This, CHAR8 *UncheckedChar);
    HRESULT (ANXAPI *SetUncheckedChar)(ITuiThemedRadioGroup *This, CHAR8 UncheckedChar);
    HRESULT (ANXAPI *GetSelectedColors)(ITuiThemedRadioGroup *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectedColors)(ITuiThemedRadioGroup *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedRadioGroup_Vtbl;

struct _ITuiThemedRadioGroup {
    CONST ITuiThemedRadioGroup_Vtbl *Vtbl;
};

// {F6A7B8C9-D0E1-4F2A-3B4C-5D6E7F8A9B0C}
DEFINE_GUID(IID_ITuiRadioGroup,
    0xF6A7B8C9, 0xD0E1, 0x4F2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C);

/**
  ITuiRadioGroup Interface

  Represents a radio button group (choice selection). Inherits from ITuiThemedRadioGroup.
**/
typedef struct _ITuiRadioGroup_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiRadioGroup *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiRadioGroup *This);
    UINTN (ANXAPI *Release)(ITuiRadioGroup *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiRadioGroup *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiRadioGroup *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiRadioGroup *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiRadioGroup *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiRadioGroup *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiRadioGroup *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiRadioGroup *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiRadioGroup *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiRadioGroup *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiRadioGroup *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiRadioGroup *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiRadioGroup *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiRadioGroup *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiRadioGroup *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiRadioGroup *This);
    HRESULT (ANXAPI *SetParent)(ITuiRadioGroup *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiRadioGroup *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiRadioGroup *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiRadioGroup *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiRadioGroup *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiRadioGroup *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiRadioGroup *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiRadioGroup *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiRadioGroup *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiRadioGroup *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiRadioGroup *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedRadioGroup methods
    HRESULT (ANXAPI *GetCheckedChar)(ITuiRadioGroup *This, CHAR8 *CheckedChar);
    HRESULT (ANXAPI *SetCheckedChar)(ITuiRadioGroup *This, CHAR8 CheckedChar);
    HRESULT (ANXAPI *GetUncheckedChar)(ITuiRadioGroup *This, CHAR8 *UncheckedChar);
    HRESULT (ANXAPI *SetUncheckedChar)(ITuiRadioGroup *This, CHAR8 UncheckedChar);
    HRESULT (ANXAPI *GetSelectedColors)(ITuiRadioGroup *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectedColors)(ITuiRadioGroup *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiRadioGroup methods
    HRESULT (ANXAPI *SetLabel)(ITuiRadioGroup *This, CONST CHAR8 *Label);
    HRESULT (ANXAPI *GetLabel)(ITuiRadioGroup *This, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *AddChoice)(ITuiRadioGroup *This, CONST CHAR8 *Label, CONST CHAR8 *Value);
    HRESULT (ANXAPI *GetSelectedIndex)(ITuiRadioGroup *This, UINT32 *Index);
    HRESULT (ANXAPI *SetSelectedIndex)(ITuiRadioGroup *This, UINT32 Index);
    HRESULT (ANXAPI *GetSelectedValue)(ITuiRadioGroup *This, CHAR8 *Buffer, UINTN BufferSize);
    UINT32 (ANXAPI *GetChoiceCount)(ITuiRadioGroup *This);
    HRESULT (ANXAPI *GetChoiceLabel)(ITuiRadioGroup *This, UINT32 Index, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *RemoveChoice)(ITuiRadioGroup *This, UINT32 Index);
    HRESULT (ANXAPI *ClearChoices)(ITuiRadioGroup *This);
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

// {4B5C6D7E-8F9A-0B1C-2D3E-4F5A6B7C8D9E}
DEFINE_GUID(IID_ITuiThemedComboBox,
    0x4B5C6D7E, 0x8F9A, 0x0B1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B, 0x7C, 0x8D, 0x9E);

/**
  ITuiThemedComboBox Interface

  ComboBox theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedComboBox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedComboBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedComboBox *This);
    UINTN (ANXAPI *Release)(ITuiThemedComboBox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedComboBox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedComboBox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedComboBox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedComboBox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedComboBox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedComboBox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedComboBox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedComboBox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedComboBox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedComboBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedComboBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedComboBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedComboBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedComboBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedComboBox *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedComboBox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedComboBox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedComboBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedComboBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedComboBox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedComboBox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedComboBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedComboBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedComboBox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedComboBox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedComboBox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedComboBox methods
    CHAR8 (ANXAPI *GetDropdownChar)(ITuiThemedComboBox *This);
    HRESULT (ANXAPI *SetDropdownChar)(ITuiThemedComboBox *This, CHAR8 DropdownChar);
    HRESULT (ANXAPI *GetDropdownColors)(ITuiThemedComboBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetDropdownColors)(ITuiThemedComboBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetListColors)(ITuiThemedComboBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetListColors)(ITuiThemedComboBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedComboBox_Vtbl;

struct _ITuiThemedComboBox {
    CONST ITuiThemedComboBox_Vtbl *Vtbl;
};

// {D0E1F2A3-B4C5-4D6E-7F8A-9B0C1D2E3F4A}
DEFINE_GUID(IID_ITuiComboBox,
    0xD0E1F2A3, 0xB4C5, 0x4D6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A);

/**
  ITuiComboBox Interface

  Editable dropdown with autocomplete. Inherits from ITuiThemedComboBox.
**/
typedef struct _ITuiComboBox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiComboBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiComboBox *This);
    UINTN (ANXAPI *Release)(ITuiComboBox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiComboBox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiComboBox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiComboBox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiComboBox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiComboBox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiComboBox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiComboBox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiComboBox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiComboBox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiComboBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiComboBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiComboBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiComboBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiComboBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiComboBox *This);
    HRESULT (ANXAPI *SetParent)(ITuiComboBox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiComboBox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiComboBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiComboBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiComboBox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiComboBox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiComboBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiComboBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiComboBox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiComboBox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiComboBox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedComboBox methods
    CHAR8 (ANXAPI *GetDropdownChar)(ITuiComboBox *This);
    HRESULT (ANXAPI *SetDropdownChar)(ITuiComboBox *This, CHAR8 DropdownChar);
    HRESULT (ANXAPI *GetDropdownColors)(ITuiComboBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetDropdownColors)(ITuiComboBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetListColors)(ITuiComboBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetListColors)(ITuiComboBox *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiComboBox methods
    HRESULT (ANXAPI *AddItem)(ITuiComboBox *This, CONST CHAR8 *Text);
    HRESULT (ANXAPI *GetText)(ITuiComboBox *This, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *SetText)(ITuiComboBox *This, CONST CHAR8 *Text);
    HRESULT (ANXAPI *SetEditable)(ITuiComboBox *This, BOOLEAN Editable);
    BOOLEAN (ANXAPI *IsEditable)(ITuiComboBox *This);
    HRESULT (ANXAPI *SetAutocomplete)(ITuiComboBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsAutocomplete)(ITuiComboBox *This);
    UINT32 (ANXAPI *GetItemCount)(ITuiComboBox *This);
    HRESULT (ANXAPI *GetItemText)(ITuiComboBox *This, UINT32 Index, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *RemoveItem)(ITuiComboBox *This, UINT32 Index);
    HRESULT (ANXAPI *ClearItems)(ITuiComboBox *This);
    HRESULT (ANXAPI *GetSelectedIndex)(ITuiComboBox *This, INT32 *Index);
    HRESULT (ANXAPI *SetSelectedIndex)(ITuiComboBox *This, INT32 Index);
} ITuiComboBox_Vtbl;

struct _ITuiComboBox {
    CONST ITuiComboBox_Vtbl *Vtbl;
};

// {7C8D9E0F-1A2B-3C4D-5E6F-7A8B9C0D1E2F}
DEFINE_GUID(IID_ITuiThemedDropDown,
    0x7C8D9E0F, 0x1A2B, 0x3C4D, 0x5E, 0x6F, 0x7A, 0x8B, 0x9C, 0x0D, 0x1E, 0x2F);

/**
  ITuiThemedDropDown Interface

  DropDown theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedDropDown_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedDropDown *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedDropDown *This);
    UINTN (ANXAPI *Release)(ITuiThemedDropDown *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedDropDown *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedDropDown *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedDropDown *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedDropDown *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedDropDown *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedDropDown *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedDropDown *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedDropDown *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedDropDown *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedDropDown *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedDropDown *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedDropDown *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedDropDown *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedDropDown *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedDropDown *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedDropDown *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedDropDown *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedDropDown *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedDropDown *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedDropDown *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedDropDown *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedDropDown *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedDropDown *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedDropDown *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedDropDown *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedDropDown *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedDropDown methods
    CHAR8 (ANXAPI *GetArrowChar)(ITuiThemedDropDown *This);
    HRESULT (ANXAPI *SetArrowChar)(ITuiThemedDropDown *This, CHAR8 ArrowChar);
    HRESULT (ANXAPI *GetListColors)(ITuiThemedDropDown *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetListColors)(ITuiThemedDropDown *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedDropDown_Vtbl;

struct _ITuiThemedDropDown {
    CONST ITuiThemedDropDown_Vtbl *Vtbl;
};

// {E1F2A3B4-C5D6-4E7F-8A9B-0C1D2E3F4A5B}
DEFINE_GUID(IID_ITuiDropDown,
    0xE1F2A3B4, 0xC5D6, 0x4E7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B);

/**
  ITuiDropDown Interface

  Dropdown menu (non-editable selection). Inherits from ITuiThemedDropDown.
**/
typedef struct _ITuiDropDown_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiDropDown *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiDropDown *This);
    UINTN (ANXAPI *Release)(ITuiDropDown *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiDropDown *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiDropDown *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiDropDown *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiDropDown *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiDropDown *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiDropDown *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiDropDown *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiDropDown *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiDropDown *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiDropDown *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiDropDown *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiDropDown *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiDropDown *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiDropDown *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiDropDown *This);
    HRESULT (ANXAPI *SetParent)(ITuiDropDown *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiDropDown *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiDropDown *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiDropDown *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiDropDown *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiDropDown *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiDropDown *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiDropDown *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiDropDown *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiDropDown *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiDropDown *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedDropDown methods
    CHAR8 (ANXAPI *GetArrowChar)(ITuiDropDown *This);
    HRESULT (ANXAPI *SetArrowChar)(ITuiDropDown *This, CHAR8 ArrowChar);
    HRESULT (ANXAPI *GetListColors)(ITuiDropDown *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetListColors)(ITuiDropDown *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiDropDown methods
    HRESULT (ANXAPI *AddItem)(ITuiDropDown *This, CONST CHAR8 *Text, VOID *UserData);
    HRESULT (ANXAPI *GetSelectedIndex)(ITuiDropDown *This, INT32 *Index);
    HRESULT (ANXAPI *SetSelectedIndex)(ITuiDropDown *This, INT32 Index);
    UINT32 (ANXAPI *GetItemCount)(ITuiDropDown *This);
    HRESULT (ANXAPI *GetItemText)(ITuiDropDown *This, UINT32 Index, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *GetItemUserData)(ITuiDropDown *This, UINT32 Index, VOID **UserData);
    HRESULT (ANXAPI *RemoveItem)(ITuiDropDown *This, UINT32 Index);
    HRESULT (ANXAPI *ClearItems)(ITuiDropDown *This);
} ITuiDropDown_Vtbl;

struct _ITuiDropDown {
    CONST ITuiDropDown_Vtbl *Vtbl;
};

// {9E0F1A2B-3C4D-5E6F-7A8B-9C0D1E2F3A4B}
DEFINE_GUID(IID_ITuiThemedMenuBar,
    0x9E0F1A2B, 0x3C4D, 0x5E6F, 0x7A, 0x8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B);

/**
  ITuiThemedMenuBar Interface

  MenuBar theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedMenuBar_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedMenuBar *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedMenuBar *This);
    UINTN (ANXAPI *Release)(ITuiThemedMenuBar *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedMenuBar *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedMenuBar *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedMenuBar *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedMenuBar *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedMenuBar *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedMenuBar *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedMenuBar *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedMenuBar *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedMenuBar *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedMenuBar *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedMenuBar *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedMenuBar *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedMenuBar *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedMenuBar *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedMenuBar *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedMenuBar *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedMenuBar *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedMenuBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedMenuBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedMenuBar *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedMenuBar *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedMenuBar *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedMenuBar *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedMenuBar *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedMenuBar *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedMenuBar *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedMenuBar methods
    HRESULT (ANXAPI *GetHotkeyColor)(ITuiThemedMenuBar *This, TUI_COLOR *HotkeyColor);
    HRESULT (ANXAPI *SetHotkeyColor)(ITuiThemedMenuBar *This, TUI_COLOR HotkeyColor);
    HRESULT (ANXAPI *GetActiveColors)(ITuiThemedMenuBar *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetActiveColors)(ITuiThemedMenuBar *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedMenuBar_Vtbl;

struct _ITuiThemedMenuBar {
    CONST ITuiThemedMenuBar_Vtbl *Vtbl;
};

// {F2A3B4C5-D6E7-4F8A-9B0C-1D2E3F4A5B6C}
DEFINE_GUID(IID_ITuiMenuBar,
    0xF2A3B4C5, 0xD6E7, 0x4F8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B, 0x6C);

/**
  ITuiMenuBar Interface

  Top menu bar with dropdown menus. Inherits from ITuiThemedMenuBar.
**/
typedef struct _ITuiMenuBar_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiMenuBar *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiMenuBar *This);
    UINTN (ANXAPI *Release)(ITuiMenuBar *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiMenuBar *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiMenuBar *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiMenuBar *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiMenuBar *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiMenuBar *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiMenuBar *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiMenuBar *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiMenuBar *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiMenuBar *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiMenuBar *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiMenuBar *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiMenuBar *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiMenuBar *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiMenuBar *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiMenuBar *This);
    HRESULT (ANXAPI *SetParent)(ITuiMenuBar *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiMenuBar *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiMenuBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiMenuBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiMenuBar *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiMenuBar *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiMenuBar *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiMenuBar *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiMenuBar *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiMenuBar *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiMenuBar *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedMenuBar methods
    HRESULT (ANXAPI *GetHotkeyColor)(ITuiMenuBar *This, TUI_COLOR *HotkeyColor);
    HRESULT (ANXAPI *SetHotkeyColor)(ITuiMenuBar *This, TUI_COLOR HotkeyColor);
    HRESULT (ANXAPI *GetActiveColors)(ITuiMenuBar *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetActiveColors)(ITuiMenuBar *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiMenuBar methods
    HRESULT (ANXAPI *AddMenu)(ITuiMenuBar *This, CONST CHAR8 *Title, ITuiMenu *Menu);
    HRESULT (ANXAPI *SetHotkey)(ITuiMenuBar *This, UINT32 MenuIndex, CHAR8 Hotkey);
    HRESULT (ANXAPI *GetHotkey)(ITuiMenuBar *This, UINT32 MenuIndex, CHAR8 *Hotkey);
    UINT32 (ANXAPI *GetMenuCount)(ITuiMenuBar *This);
    HRESULT (ANXAPI *GetMenuTitle)(ITuiMenuBar *This, UINT32 MenuIndex, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *GetMenu)(ITuiMenuBar *This, UINT32 MenuIndex, ITuiMenu **Menu);
    HRESULT (ANXAPI *RemoveMenu)(ITuiMenuBar *This, UINT32 MenuIndex);
    HRESULT (ANXAPI *ClearMenus)(ITuiMenuBar *This);
} ITuiMenuBar_Vtbl;

struct _ITuiMenuBar {
    CONST ITuiMenuBar_Vtbl *Vtbl;
};

// {2A3B4C5D-6E7F-8A9B-0C1D-2E3F4A5B6C7D}
DEFINE_GUID(IID_ITuiThemedStatusBar,
    0x2A3B4C5D, 0x6E7F, 0x8A9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B, 0x6C, 0x7D);

/**
  ITuiThemedStatusBar Interface

  StatusBar theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedStatusBar_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedStatusBar *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedStatusBar *This);
    UINTN (ANXAPI *Release)(ITuiThemedStatusBar *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedStatusBar *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedStatusBar *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedStatusBar *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedStatusBar *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedStatusBar *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedStatusBar *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedStatusBar *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedStatusBar *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedStatusBar *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedStatusBar *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedStatusBar *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedStatusBar *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedStatusBar *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedStatusBar *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedStatusBar *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedStatusBar *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedStatusBar *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedStatusBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedStatusBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedStatusBar *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedStatusBar *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedStatusBar *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedStatusBar *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedStatusBar *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedStatusBar *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedStatusBar *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedStatusBar methods
    /**
      Get panel separator character.
    **/
    CHAR8 (ANXAPI *GetSeparatorChar)(
        ITuiThemedStatusBar *This
    );

    /**
      Set panel separator character.
    **/
    HRESULT (ANXAPI *SetSeparatorChar)(
        ITuiThemedStatusBar *This,
        CHAR8 SeparatorChar
    );
} ITuiThemedStatusBar_Vtbl;

struct _ITuiThemedStatusBar {
    CONST ITuiThemedStatusBar_Vtbl *Vtbl;
};

// {A3B4C5D6-E7F8-4A9B-0C1D-2E3F4A5B6C7D}
DEFINE_GUID(IID_ITuiStatusBar,
    0xA3B4C5D6, 0xE7F8, 0x4A9B, 0x0C, 0x1D, 0x2E, 0x3F, 0x4A, 0x5B, 0x6C, 0x7D);

/**
  ITuiStatusBar Interface

  Multi-panel status bar widget. Inherits from ITuiThemedStatusBar.
**/
typedef struct _ITuiStatusBar_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiStatusBar *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiStatusBar *This);
    UINTN (ANXAPI *Release)(ITuiStatusBar *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiStatusBar *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiStatusBar *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiStatusBar *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiStatusBar *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiStatusBar *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiStatusBar *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiStatusBar *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiStatusBar *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiStatusBar *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiStatusBar *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiStatusBar *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiStatusBar *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiStatusBar *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiStatusBar *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiStatusBar *This);
    HRESULT (ANXAPI *SetParent)(ITuiStatusBar *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiStatusBar *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiStatusBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiStatusBar *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiStatusBar *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiStatusBar *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiStatusBar *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiStatusBar *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiStatusBar *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiStatusBar *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiStatusBar *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedStatusBar methods
    CHAR8 (ANXAPI *GetSeparatorChar)(ITuiStatusBar *This);
    HRESULT (ANXAPI *SetSeparatorChar)(ITuiStatusBar *This, CHAR8 SeparatorChar);

    // ITuiStatusBar methods
    /**
      Add a panel with fixed width.
    **/
    HRESULT (ANXAPI *AddPanel)(
        ITuiStatusBar *This,
        UINT32 Width,
        TUI_TEXT_ALIGNMENT Alignment
    );

    /**
      Add a spring panel (expands to fill available space).
    **/
    HRESULT (ANXAPI *AddSpringPanel)(
        ITuiStatusBar *This,
        TUI_TEXT_ALIGNMENT Alignment
    );

    /**
      Get panel count.
    **/
    UINT32 (ANXAPI *GetPanelCount)(
        ITuiStatusBar *This
    );

    /**
      Set panel count (legacy).
    **/
    HRESULT (ANXAPI *SetPanelCount)(
        ITuiStatusBar *This,
        UINT32 Count
    );

    /**
      Get panel text.
    **/
    HRESULT (ANXAPI *GetText)(
        ITuiStatusBar *This,
        UINT32 PanelIndex,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Set panel text.
    **/
    HRESULT (ANXAPI *SetText)(
        ITuiStatusBar *This,
        UINT32 PanelIndex,
        CONST CHAR8 *Text
    );

    /**
      Get panel width.
    **/
    HRESULT (ANXAPI *GetPanelWidth)(
        ITuiStatusBar *This,
        UINT32 PanelIndex,
        UINT32 *Width
    );

    /**
      Set panel width.
    **/
    HRESULT (ANXAPI *SetPanelWidth)(
        ITuiStatusBar *This,
        UINT32 PanelIndex,
        UINT32 Width
    );

    /**
      Clear all panels.
    **/
    HRESULT (ANXAPI *ClearPanels)(
        ITuiStatusBar *This
    );
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

// {0A1B2C3D-4E5F-6A7B-8C9D-0E1F2A3B4C5D}
DEFINE_GUID(IID_ITuiThemedTabControl,
    0x0A1B2C3D, 0x4E5F, 0x6A7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D);

/**
  ITuiThemedTabControl Interface

  TabControl theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedTabControl_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedTabControl *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedTabControl *This);
    UINTN (ANXAPI *Release)(ITuiThemedTabControl *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedTabControl *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedTabControl *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedTabControl *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedTabControl *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedTabControl *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedTabControl *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedTabControl *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedTabControl *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedTabControl *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedTabControl *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedTabControl *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedTabControl *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedTabControl *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedTabControl *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedTabControl *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedTabControl *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedTabControl *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedTabControl *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedTabControl *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedTabControl *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedTabControl *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedTabControl *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedTabControl *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedTabControl *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedTabControl *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedTabControl *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedTabControl methods
    HRESULT (ANXAPI *GetActiveTabColors)(ITuiThemedTabControl *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetActiveTabColors)(ITuiThemedTabControl *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetInactiveTabColors)(ITuiThemedTabControl *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetInactiveTabColors)(ITuiThemedTabControl *This, TUI_COLOR Foreground, TUI_COLOR Background);
    CHAR8 (ANXAPI *GetSeparatorChar)(ITuiThemedTabControl *This);
    HRESULT (ANXAPI *SetSeparatorChar)(ITuiThemedTabControl *This, CHAR8 SeparatorChar);
} ITuiThemedTabControl_Vtbl;

struct _ITuiThemedTabControl {
    CONST ITuiThemedTabControl_Vtbl *Vtbl;
};

// {D6E7F8A9-B0C1-4D2E-3F4A-5B6C7D8E9F0A}
DEFINE_GUID(IID_ITuiTabControl,
    0xD6E7F8A9, 0xB0C1, 0x4D2E, 0x3F, 0x4A, 0x5B, 0x6C, 0x7D, 0x8E, 0x9F, 0x0A);

/**
  ITuiTabControl Interface

  Tabbed pages container. Inherits from ITuiThemedTabControl.
**/
typedef struct _ITuiTabControl_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiTabControl *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiTabControl *This);
    UINTN (ANXAPI *Release)(ITuiTabControl *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiTabControl *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiTabControl *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiTabControl *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiTabControl *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiTabControl *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiTabControl *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiTabControl *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiTabControl *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiTabControl *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiTabControl *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiTabControl *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiTabControl *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiTabControl *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiTabControl *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiTabControl *This);
    HRESULT (ANXAPI *SetParent)(ITuiTabControl *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiTabControl *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiTabControl *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiTabControl *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiTabControl *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiTabControl *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiTabControl *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiTabControl *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiTabControl *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiTabControl *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiTabControl *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedTabControl methods
    HRESULT (ANXAPI *GetActiveTabColors)(ITuiTabControl *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetActiveTabColors)(ITuiTabControl *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetInactiveTabColors)(ITuiTabControl *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetInactiveTabColors)(ITuiTabControl *This, TUI_COLOR Foreground, TUI_COLOR Background);
    CHAR8 (ANXAPI *GetSeparatorChar)(ITuiTabControl *This);
    HRESULT (ANXAPI *SetSeparatorChar)(ITuiTabControl *This, CHAR8 SeparatorChar);

    // ITuiTabControl methods
    HRESULT (ANXAPI *AddTab)(ITuiTabControl *This, CONST CHAR8 *Title, ITuiWidget *Content, VOID *UserData);
    HRESULT (ANXAPI *RemoveTab)(ITuiTabControl *This, UINT32 Index);
    UINT32 (ANXAPI *GetTabCount)(ITuiTabControl *This);
    HRESULT (ANXAPI *GetActiveTab)(ITuiTabControl *This, INT32 *Index);
    HRESULT (ANXAPI *SetActiveTab)(ITuiTabControl *This, INT32 Index);
    HRESULT (ANXAPI *GetTabTitle)(ITuiTabControl *This, UINT32 Index, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *SetTabTitle)(ITuiTabControl *This, UINT32 Index, CONST CHAR8 *Title);
    HRESULT (ANXAPI *SetTabEnabled)(ITuiTabControl *This, UINT32 Index, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsTabEnabled)(ITuiTabControl *This, UINT32 Index);
    HRESULT (ANXAPI *GetTabUserData)(ITuiTabControl *This, UINT32 Index, VOID **UserData);
    HRESULT (ANXAPI *GetTabContent)(ITuiTabControl *This, UINT32 Index, ITuiWidget **Content);
    HRESULT (ANXAPI *ClearTabs)(ITuiTabControl *This);
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

/**
  Color Picker Mode
**/
typedef enum _TUI_COLOR_PICKER_MODE {
    TuiColorPickerBasic,       /* 8/16 color palette */
    TuiColorPicker256,         /* 256 color palette */
    TuiColorPickerRGB,         /* RGB sliders (if supported) */
    TuiColorPickerHSV          /* HSV color wheel (if supported) */
} TUI_COLOR_PICKER_MODE;

// {6C7D8E9F-0A1B-2C3D-4E5F-6A7B8C9D0E1F}
DEFINE_GUID(IID_ITuiThemedColorPicker,
    0x6C7D8E9F, 0x0A1B, 0x2C3D, 0x4E, 0x5F, 0x6A, 0x7B, 0x8C, 0x9D, 0x0E, 0x1F);

/**
  ITuiThemedColorPicker Interface

  ColorPicker theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedColorPicker_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedColorPicker *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedColorPicker *This);
    UINTN (ANXAPI *Release)(ITuiThemedColorPicker *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedColorPicker *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedColorPicker *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedColorPicker *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedColorPicker *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedColorPicker *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedColorPicker *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedColorPicker *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedColorPicker *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedColorPicker *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedColorPicker *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedColorPicker *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedColorPicker *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedColorPicker *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedColorPicker *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedColorPicker *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedColorPicker *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedColorPicker *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedColorPicker *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedColorPicker *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedColorPicker *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedColorPicker *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedColorPicker *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedColorPicker *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedColorPicker *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedColorPicker *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedColorPicker *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedColorPicker methods
    HRESULT (ANXAPI *GetSelectedCellColors)(ITuiThemedColorPicker *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectedCellColors)(ITuiThemedColorPicker *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetBorderColors)(ITuiThemedColorPicker *This, TUI_COLOR *BorderColor);
    HRESULT (ANXAPI *SetBorderColors)(ITuiThemedColorPicker *This, TUI_COLOR BorderColor);
} ITuiThemedColorPicker_Vtbl;

struct _ITuiThemedColorPicker {
    CONST ITuiThemedColorPicker_Vtbl *Vtbl;
};

// {F8A9B0C1-D2E3-4F4A-5B6C-7D8E9F0A1B2C}
DEFINE_GUID(IID_ITuiColorPicker,
    0xF8A9B0C1, 0xD2E3, 0x4F4A, 0x5B, 0x6C, 0x7D, 0x8E, 0x9F, 0x0A, 0x1B, 0x2C);

/**
  ITuiColorPicker Interface

  Interactive color selection widget. Inherits from ITuiThemedColorPicker.
**/
typedef struct _ITuiColorPicker_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiColorPicker *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiColorPicker *This);
    UINTN (ANXAPI *Release)(ITuiColorPicker *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiColorPicker *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiColorPicker *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiColorPicker *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiColorPicker *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiColorPicker *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiColorPicker *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiColorPicker *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiColorPicker *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiColorPicker *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiColorPicker *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiColorPicker *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiColorPicker *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiColorPicker *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiColorPicker *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiColorPicker *This);
    HRESULT (ANXAPI *SetParent)(ITuiColorPicker *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiColorPicker *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiColorPicker *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiColorPicker *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiColorPicker *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiColorPicker *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiColorPicker *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiColorPicker *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiColorPicker *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiColorPicker *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiColorPicker *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedColorPicker methods
    HRESULT (ANXAPI *GetSelectedCellColors)(ITuiColorPicker *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectedCellColors)(ITuiColorPicker *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetBorderColors)(ITuiColorPicker *This, TUI_COLOR *BorderColor);
    HRESULT (ANXAPI *SetBorderColors)(ITuiColorPicker *This, TUI_COLOR BorderColor);

    // ITuiColorPicker methods
    HRESULT (ANXAPI *SetMode)(ITuiColorPicker *This, TUI_COLOR_PICKER_MODE Mode);
    TUI_COLOR_PICKER_MODE (ANXAPI *GetMode)(ITuiColorPicker *This);
    HRESULT (ANXAPI *GetColor)(ITuiColorPicker *This, TUI_COLOR *Color);
    HRESULT (ANXAPI *SetColor)(ITuiColorPicker *This, TUI_COLOR Color);
    HRESULT (ANXAPI *GetRGB)(ITuiColorPicker *This, UINT8 *Red, UINT8 *Green, UINT8 *Blue);
    HRESULT (ANXAPI *SetRGB)(ITuiColorPicker *This, UINT8 Red, UINT8 Green, UINT8 Blue);
    HRESULT (ANXAPI *Show)(ITuiColorPicker *This, ITuiScreen *Screen, TUI_COLOR *SelectedColor);
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

// {2F3A4B5C-6D7E-8F9A-0B1C-2D3E4F5A6B7C}
DEFINE_GUID(IID_ITuiThemedTextEditor,
    0x2F3A4B5C, 0x6D7E, 0x8F9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B, 0x7C);

/**
  ITuiThemedTextEditor Interface

  TextEditor theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedTextEditor_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedTextEditor *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedTextEditor *This);
    UINTN (ANXAPI *Release)(ITuiThemedTextEditor *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedTextEditor *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedTextEditor *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedTextEditor *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedTextEditor *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedTextEditor *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedTextEditor *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedTextEditor *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedTextEditor *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedTextEditor *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedTextEditor *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedTextEditor *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedTextEditor *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedTextEditor *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedTextEditor *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedTextEditor *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedTextEditor *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedTextEditor *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedTextEditor *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedTextEditor *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedTextEditor *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedTextEditor *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedTextEditor *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedTextEditor *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedTextEditor *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedTextEditor methods
    HRESULT (ANXAPI *GetSelectionColors)(ITuiThemedTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectionColors)(ITuiThemedTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetLineNumberColors)(ITuiThemedTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetLineNumberColors)(ITuiThemedTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetCurrentLineColor)(ITuiThemedTextEditor *This, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetCurrentLineColor)(ITuiThemedTextEditor *This, TUI_COLOR Background);
} ITuiThemedTextEditor_Vtbl;

struct _ITuiThemedTextEditor {
    CONST ITuiThemedTextEditor_Vtbl *Vtbl;
};

/**
  ITuiTextEditor Interface

  Multi-line text editor with scrolling and syntax highlighting.
  Inherits from ITuiThemedTextEditor.
**/
typedef struct _ITuiTextEditor_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiTextEditor *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiTextEditor *This);
    UINTN (ANXAPI *Release)(ITuiTextEditor *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiTextEditor *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiTextEditor *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiTextEditor *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiTextEditor *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiTextEditor *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiTextEditor *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiTextEditor *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiTextEditor *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiTextEditor *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiTextEditor *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiTextEditor *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiTextEditor *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiTextEditor *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiTextEditor *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiTextEditor *This);
    HRESULT (ANXAPI *SetParent)(ITuiTextEditor *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiTextEditor *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiTextEditor *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiTextEditor *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiTextEditor *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiTextEditor *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiTextEditor *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiTextEditor *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiTextEditor *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedTextEditor methods
    HRESULT (ANXAPI *GetSelectionColors)(ITuiTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectionColors)(ITuiTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetLineNumberColors)(ITuiTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetLineNumberColors)(ITuiTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetCurrentLineColor)(ITuiTextEditor *This, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetCurrentLineColor)(ITuiTextEditor *This, TUI_COLOR Background);

    // ITuiTextEditor methods
    HRESULT (ANXAPI *SetText)(ITuiTextEditor *This, CONST CHAR8 *Text);
    HRESULT (ANXAPI *GetText)(ITuiTextEditor *This, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *LoadFile)(ITuiTextEditor *This, CONST CHAR8 *FilePath);
    HRESULT (ANXAPI *SaveFile)(ITuiTextEditor *This, CONST CHAR8 *FilePath);
    HRESULT (ANXAPI *SetReadOnly)(ITuiTextEditor *This, BOOLEAN ReadOnly);
    BOOLEAN (ANXAPI *GetReadOnly)(ITuiTextEditor *This);
    HRESULT (ANXAPI *SetWordWrap)(ITuiTextEditor *This, BOOLEAN WordWrap);
    BOOLEAN (ANXAPI *GetWordWrap)(ITuiTextEditor *This);
    HRESULT (ANXAPI *SetTabSize)(ITuiTextEditor *This, UINT32 TabSize);
    UINT32 (ANXAPI *GetTabSize)(ITuiTextEditor *This);
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
    TUI_TEXT_DIRECTION (ANXAPI *GetTextDirection)(ITuiTextEditor *This);
} ITuiTextEditor_Vtbl;

struct _ITuiTextEditor {
    CONST ITuiTextEditor_Vtbl *Vtbl;
};

// {3A4B5C6D-7E8F-9A0B-1C2D-3E4F5A6B7C8D}
DEFINE_GUID(IID_ITuiThemedScrollView,
    0x3A4B5C6D, 0x7E8F, 0x9A0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B, 0x7C, 0x8D);

/**
  ITuiThemedScrollView Interface

  ScrollView theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedScrollView_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedScrollView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedScrollView *This);
    UINTN (ANXAPI *Release)(ITuiThemedScrollView *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedScrollView *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedScrollView *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedScrollView *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedScrollView *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedScrollView *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedScrollView *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedScrollView *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedScrollView *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedScrollView *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedScrollView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedScrollView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedScrollView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedScrollView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedScrollView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedScrollView *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedScrollView *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedScrollView *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedScrollView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedScrollView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedScrollView *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedScrollView *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedScrollView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedScrollView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedScrollView *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedScrollView *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedScrollView *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedScrollView methods
    HRESULT (ANXAPI *GetScrollbarColors)(ITuiThemedScrollView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetScrollbarColors)(ITuiThemedScrollView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetScrollbarThumbColors)(ITuiThemedScrollView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetScrollbarThumbColors)(ITuiThemedScrollView *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedScrollView_Vtbl;

struct _ITuiThemedScrollView {
    CONST ITuiThemedScrollView_Vtbl *Vtbl;
};

// {E3F4A5B6-C7D8-4E9F-0A1B-2C3D4E5F6A7B}
DEFINE_GUID(IID_ITuiScrollView,
    0xE3F4A5B6, 0xC7D8, 0x4E9F, 0x0A, 0x1B, 0x2C, 0x3D, 0x4E, 0x5F, 0x6A, 0x7B);

/**
  ITuiScrollView Interface

  Scrollable container for widgets or content.
  Inherits from ITuiThemedScrollView.
**/
typedef struct _ITuiScrollView_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiScrollView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiScrollView *This);
    UINTN (ANXAPI *Release)(ITuiScrollView *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiScrollView *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiScrollView *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiScrollView *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiScrollView *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiScrollView *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiScrollView *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiScrollView *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiScrollView *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiScrollView *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiScrollView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiScrollView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiScrollView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiScrollView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiScrollView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiScrollView *This);
    HRESULT (ANXAPI *SetParent)(ITuiScrollView *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiScrollView *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiScrollView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiScrollView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiScrollView *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiScrollView *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiScrollView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiScrollView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiScrollView *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiScrollView *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiScrollView *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedScrollView methods
    HRESULT (ANXAPI *GetScrollbarColors)(ITuiScrollView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetScrollbarColors)(ITuiScrollView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetScrollbarThumbColors)(ITuiScrollView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetScrollbarThumbColors)(ITuiScrollView *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiScrollView methods
    HRESULT (ANXAPI *SetContentSize)(ITuiScrollView *This, UINT32 Width, UINT32 Height);
    HRESULT (ANXAPI *GetContentSize)(ITuiScrollView *This, UINT32 *Width, UINT32 *Height);
    HRESULT (ANXAPI *SetScrollPosition)(ITuiScrollView *This, INT32 X, INT32 Y);
    HRESULT (ANXAPI *GetScrollPosition)(ITuiScrollView *This, INT32 *X, INT32 *Y);
    HRESULT (ANXAPI *ScrollBy)(ITuiScrollView *This, INT32 DeltaX, INT32 DeltaY);
    HRESULT (ANXAPI *SetShowScrollbars)(ITuiScrollView *This, BOOLEAN Horizontal, BOOLEAN Vertical);
    HRESULT (ANXAPI *GetShowScrollbars)(ITuiScrollView *This, BOOLEAN *Horizontal, BOOLEAN *Vertical);
    HRESULT (ANXAPI *AddScrollChild)(ITuiScrollView *This, VOID *Widget, INT32 X, INT32 Y);
    HRESULT (ANXAPI *RemoveScrollChild)(ITuiScrollView *This, VOID *Widget);
} ITuiScrollView_Vtbl;

struct _ITuiScrollView {
    CONST ITuiScrollView_Vtbl *Vtbl;
};

// {4B5C6D7E-8F9A-0B1C-2D3E-4F5A6B7C8D9E}
DEFINE_GUID(IID_ITuiThemedLongOpDialog,
    0x4B5C6D7E, 0x8F9A, 0x0B1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B, 0x7C, 0x8D, 0x9E);

/**
  ITuiThemedLongOpDialog Interface

  LongOpDialog theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedLongOpDialog_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedLongOpDialog *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedLongOpDialog *This);
    UINTN (ANXAPI *Release)(ITuiThemedLongOpDialog *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedLongOpDialog *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedLongOpDialog *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedLongOpDialog *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedLongOpDialog *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedLongOpDialog *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedLongOpDialog *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedLongOpDialog *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedLongOpDialog *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedLongOpDialog *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedLongOpDialog *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedLongOpDialog *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedLongOpDialog *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedLongOpDialog *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedLongOpDialog *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedLongOpDialog *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedLongOpDialog *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedLongOpDialog *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedLongOpDialog *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedLongOpDialog *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedLongOpDialog *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedLongOpDialog *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedLongOpDialog *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedLongOpDialog *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedLongOpDialog *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedLongOpDialog *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedLongOpDialog *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedLongOpDialog methods
    HRESULT (ANXAPI *GetProgressColors)(ITuiThemedLongOpDialog *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetProgressColors)(ITuiThemedLongOpDialog *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetTitleColors)(ITuiThemedLongOpDialog *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetTitleColors)(ITuiThemedLongOpDialog *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetStatusColors)(ITuiThemedLongOpDialog *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetStatusColors)(ITuiThemedLongOpDialog *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedLongOpDialog_Vtbl;

struct _ITuiThemedLongOpDialog {
    CONST ITuiThemedLongOpDialog_Vtbl *Vtbl;
};

// {3F4E5D6C-7B8A-9C0D-1E2F-3A4B5C6D7E8F}
DEFINE_GUID(IID_ITuiLongOpDialog,
    0x3F4E5D6C, 0x7B8A, 0x9C0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F);

/**
  ITuiLongOpDialog Interface

  Modal dialog for long-running operations with progress tracking,
  time estimation, and cancel support. Inherits from ITuiThemedLongOpDialog.
**/
typedef struct _ITuiLongOpDialog_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiLongOpDialog *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiLongOpDialog *This);
    UINTN (ANXAPI *Release)(ITuiLongOpDialog *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiLongOpDialog *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiLongOpDialog *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiLongOpDialog *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiLongOpDialog *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiLongOpDialog *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiLongOpDialog *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiLongOpDialog *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiLongOpDialog *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiLongOpDialog *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiLongOpDialog *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiLongOpDialog *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiLongOpDialog *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiLongOpDialog *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiLongOpDialog *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiLongOpDialog *This);
    HRESULT (ANXAPI *SetParent)(ITuiLongOpDialog *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiLongOpDialog *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiLongOpDialog *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiLongOpDialog *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiLongOpDialog *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiLongOpDialog *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiLongOpDialog *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiLongOpDialog *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiLongOpDialog *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiLongOpDialog *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiLongOpDialog *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedLongOpDialog methods
    HRESULT (ANXAPI *GetProgressColors)(ITuiLongOpDialog *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetProgressColors)(ITuiLongOpDialog *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetTitleColors)(ITuiLongOpDialog *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetTitleColors)(ITuiLongOpDialog *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetStatusColors)(ITuiLongOpDialog *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetStatusColors)(ITuiLongOpDialog *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiLongOpDialog methods
    HRESULT (ANXAPI *SetTitle)(ITuiLongOpDialog *This, CONST CHAR8 *Title);
    HRESULT (ANXAPI *GetTitle)(ITuiLongOpDialog *This, CHAR8 *Buffer, UINTN BufferSize);
    HRESULT (ANXAPI *UpdateProgress)(ITuiLongOpDialog *This, UINT32 Percent, CONST CHAR8 *StatusText);
    UINT32 (ANXAPI *GetProgress)(ITuiLongOpDialog *This);
    HRESULT (ANXAPI *SetIndeterminate)(ITuiLongOpDialog *This, BOOLEAN Indeterminate);
    BOOLEAN (ANXAPI *GetIndeterminate)(ITuiLongOpDialog *This);
    BOOLEAN (ANXAPI *IsCancelled)(ITuiLongOpDialog *This);
    HRESULT (ANXAPI *SetCancelCallback)(ITuiLongOpDialog *This, HRESULT (*Callback)(VOID *UserData), VOID *UserData);
    HRESULT (ANXAPI *Start)(ITuiLongOpDialog *This);
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

// {2F3A4B5C-6D7E-8F9A-0B1C-2D3E4F5A6B7C}
DEFINE_GUID(IID_ITuiThemedTerminal,
    0x2F3A4B5C, 0x6D7E, 0x8F9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B, 0x7C);

/**
  ITuiThemedTerminal Interface

  Terminal theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedTerminal_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedTerminal *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedTerminal *This);
    UINTN (ANXAPI *Release)(ITuiThemedTerminal *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedTerminal *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedTerminal *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedTerminal *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedTerminal *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedTerminal *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedTerminal *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedTerminal *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedTerminal *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedTerminal *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedTerminal *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedTerminal *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedTerminal *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedTerminal *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedTerminal *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedTerminal *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedTerminal *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedTerminal *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedTerminal *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedTerminal *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedTerminal *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedTerminal *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedTerminal *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedTerminal *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedTerminal *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedTerminal *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedTerminal *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedTerminal methods
    HRESULT (ANXAPI *GetCursorColor)(ITuiThemedTerminal *This, TUI_COLOR *CursorColor);
    HRESULT (ANXAPI *SetCursorColor)(ITuiThemedTerminal *This, TUI_COLOR CursorColor);
    HRESULT (ANXAPI *GetSelectionColors)(ITuiThemedTerminal *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectionColors)(ITuiThemedTerminal *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetScrollbarColors)(ITuiThemedTerminal *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetScrollbarColors)(ITuiThemedTerminal *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedTerminal_Vtbl;

struct _ITuiThemedTerminal {
    CONST ITuiThemedTerminal_Vtbl *Vtbl;
};

/**
  ITuiTerminal Interface

  Terminal emulator widget with customizable renderer and parser
  for ANSI/VT100 escape sequences. Inherits from ITuiThemedTerminal.
**/
typedef struct _ITuiTerminal_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiTerminal *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiTerminal *This);
    UINTN (ANXAPI *Release)(ITuiTerminal *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiTerminal *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiTerminal *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiTerminal *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiTerminal *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiTerminal *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiTerminal *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiTerminal *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiTerminal *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiTerminal *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiTerminal *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiTerminal *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiTerminal *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiTerminal *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiTerminal *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiTerminal *This);
    HRESULT (ANXAPI *SetParent)(ITuiTerminal *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiTerminal *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiTerminal *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiTerminal *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiTerminal *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiTerminal *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiTerminal *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiTerminal *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiTerminal *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiTerminal *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiTerminal *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedTerminal methods
    HRESULT (ANXAPI *GetCursorColor)(ITuiTerminal *This, TUI_COLOR *CursorColor);
    HRESULT (ANXAPI *SetCursorColor)(ITuiTerminal *This, TUI_COLOR CursorColor);
    HRESULT (ANXAPI *GetSelectionColors)(ITuiTerminal *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectionColors)(ITuiTerminal *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetScrollbarColors)(ITuiTerminal *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetScrollbarColors)(ITuiTerminal *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiTerminal methods
    HRESULT (ANXAPI *WriteText)(ITuiTerminal *This, CONST CHAR8 *Text, UINTN Length);
    HRESULT (ANXAPI *Clear)(ITuiTerminal *This);
    HRESULT (ANXAPI *SetSize)(ITuiTerminal *This, UINT32 Cols, UINT32 Rows);
    HRESULT (ANXAPI *GetSize)(ITuiTerminal *This, UINT32 *Cols, UINT32 *Rows);
    HRESULT (ANXAPI *SetRenderer)(ITuiTerminal *This, TerminalRenderCallback Renderer, VOID *UserData);
    HRESULT (ANXAPI *SetParser)(ITuiTerminal *This, TerminalParserCallback Parser, VOID *UserData);
    HRESULT (ANXAPI *SetInputCallback)(ITuiTerminal *This, HRESULT (*Callback)(VOID *UserData, CONST CHAR8 *Input, UINTN Length), VOID *UserData);
    HRESULT (ANXAPI *GetCursorPosition)(ITuiTerminal *This, UINT32 *Col, UINT32 *Row);
    HRESULT (ANXAPI *SetCursorPosition)(ITuiTerminal *This, UINT32 Col, UINT32 Row);
    HRESULT (ANXAPI *SetScrollbackLines)(ITuiTerminal *This, UINT32 Lines);
    UINT32 (ANXAPI *GetScrollbackLines)(ITuiTerminal *This);
} ITuiTerminal_Vtbl;

struct _ITuiTerminal {
    CONST ITuiTerminal_Vtbl *Vtbl;
};

// {3A4B5C6D-7E8F-9A0B-1C2D-3E4F5A6B7C8D}
DEFINE_GUID(IID_ITuiThemedTreeView,
    0x3A4B5C6D, 0x7E8F, 0x9A0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B, 0x7C, 0x8D);

/**
  ITuiThemedTreeView Interface

  TreeView theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedTreeView_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedTreeView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedTreeView *This);
    UINTN (ANXAPI *Release)(ITuiThemedTreeView *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedTreeView *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedTreeView *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedTreeView *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedTreeView *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedTreeView *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedTreeView *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedTreeView *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedTreeView *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedTreeView *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedTreeView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedTreeView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedTreeView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedTreeView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedTreeView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedTreeView *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedTreeView *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedTreeView *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedTreeView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedTreeView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedTreeView *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedTreeView *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedTreeView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedTreeView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedTreeView *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedTreeView *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedTreeView *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedTreeView methods
    HRESULT (ANXAPI *GetSelectedColors)(ITuiThemedTreeView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectedColors)(ITuiThemedTreeView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetExpandedChar)(ITuiThemedTreeView *This, CHAR8 *ExpandedChar);
    HRESULT (ANXAPI *SetExpandedChar)(ITuiThemedTreeView *This, CHAR8 ExpandedChar);
    HRESULT (ANXAPI *GetCollapsedChar)(ITuiThemedTreeView *This, CHAR8 *CollapsedChar);
    HRESULT (ANXAPI *SetCollapsedChar)(ITuiThemedTreeView *This, CHAR8 CollapsedChar);
    HRESULT (ANXAPI *GetIndentChar)(ITuiThemedTreeView *This, CHAR8 *IndentChar);
    HRESULT (ANXAPI *SetIndentChar)(ITuiThemedTreeView *This, CHAR8 IndentChar);
} ITuiThemedTreeView_Vtbl;

struct _ITuiThemedTreeView {
    CONST ITuiThemedTreeView_Vtbl *Vtbl;
};

// {6C7D8E9F-0A1B-2C3D-4E5F-6A7B8C9D0E1F}
DEFINE_GUID(IID_ITuiTreeView,
    0x6C7D8E9F, 0x0A1B, 0x2C3D, 0x4E, 0x5F, 0x6A, 0x7B, 0x8C, 0x9D, 0x0E, 0x1F);

/**
  ITuiTreeView Interface

  Hierarchical tree control with expand/collapse, checkboxes,
  inline editing, and keyboard navigation. Inherits from ITuiThemedTreeView.
**/
typedef struct _ITuiTreeView_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiTreeView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiTreeView *This);
    UINTN (ANXAPI *Release)(ITuiTreeView *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiTreeView *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiTreeView *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiTreeView *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiTreeView *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiTreeView *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiTreeView *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiTreeView *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiTreeView *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiTreeView *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiTreeView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiTreeView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiTreeView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiTreeView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiTreeView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiTreeView *This);
    HRESULT (ANXAPI *SetParent)(ITuiTreeView *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiTreeView *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiTreeView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiTreeView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiTreeView *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiTreeView *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiTreeView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiTreeView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiTreeView *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiTreeView *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiTreeView *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedTreeView methods
    HRESULT (ANXAPI *GetSelectedColors)(ITuiTreeView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectedColors)(ITuiTreeView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetExpandedChar)(ITuiTreeView *This, CHAR8 *ExpandedChar);
    HRESULT (ANXAPI *SetExpandedChar)(ITuiTreeView *This, CHAR8 ExpandedChar);
    HRESULT (ANXAPI *GetCollapsedChar)(ITuiTreeView *This, CHAR8 *CollapsedChar);
    HRESULT (ANXAPI *SetCollapsedChar)(ITuiTreeView *This, CHAR8 CollapsedChar);
    HRESULT (ANXAPI *GetIndentChar)(ITuiTreeView *This, CHAR8 *IndentChar);
    HRESULT (ANXAPI *SetIndentChar)(ITuiTreeView *This, CHAR8 IndentChar);

    // ITuiTreeView methods
    HRESULT (ANXAPI *AddNode)(ITuiTreeView *This, CONST CHAR8 *Text, VOID *UserData, VOID **OutHandle);
    HRESULT (ANXAPI *AddChildNode)(ITuiTreeView *This, VOID *ParentHandle, CONST CHAR8 *Text, VOID *UserData, VOID **OutHandle);
    HRESULT (ANXAPI *SetNodeCheckbox)(ITuiTreeView *This, VOID *NodeHandle, BOOLEAN HasCheckbox, UINT8 CheckState);
    HRESULT (ANXAPI *GetNodeCheckbox)(ITuiTreeView *This, VOID *NodeHandle, BOOLEAN *HasCheckbox, UINT8 *CheckState);
    HRESULT (ANXAPI *SetNodeIcon)(ITuiTreeView *This, VOID *NodeHandle, UINT32 Icon);
    HRESULT (ANXAPI *GetNodeIcon)(ITuiTreeView *This, VOID *NodeHandle, UINT32 *Icon);
    HRESULT (ANXAPI *ExpandNode)(ITuiTreeView *This, VOID *NodeHandle, BOOLEAN Expand);
    BOOLEAN (ANXAPI *IsNodeExpanded)(ITuiTreeView *This, VOID *NodeHandle);
    HRESULT (ANXAPI *RemoveNode)(ITuiTreeView *This, VOID *NodeHandle);
    HRESULT (ANXAPI *Clear)(ITuiTreeView *This);
    HRESULT (ANXAPI *SetVirtualMode)(ITuiTreeView *This, BOOLEAN Enable, UINT32 ItemCount, HRESULT (*Callback)(VOID *UserData, UINT32 Index, VOID *OutData), VOID *UserData);
    HRESULT (ANXAPI *GetSelectedNode)(ITuiTreeView *This, VOID **NodeHandle);
    HRESULT (ANXAPI *SetSelectedNode)(ITuiTreeView *This, VOID *NodeHandle);
} ITuiTreeView_Vtbl;

struct _ITuiTreeView {
    CONST ITuiTreeView_Vtbl *Vtbl;
};

// {4B5C6D7E-8F9A-0B1C-2D3E-4F5A6B7C8D9E}
DEFINE_GUID(IID_ITuiThemedListView,
    0x4B5C6D7E, 0x8F9A, 0x0B1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B, 0x7C, 0x8D, 0x9E);

/**
  ITuiThemedListView Interface

  ListView theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedListView_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedListView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedListView *This);
    UINTN (ANXAPI *Release)(ITuiThemedListView *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedListView *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedListView *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedListView *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedListView *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedListView *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedListView *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedListView *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedListView *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedListView *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedListView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedListView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedListView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedListView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedListView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedListView *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedListView *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedListView *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedListView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedListView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedListView *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedListView *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedListView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedListView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedListView *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedListView *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedListView *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedListView methods
    HRESULT (ANXAPI *GetSelectedColors)(ITuiThemedListView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectedColors)(ITuiThemedListView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetAlternateRowColors)(ITuiThemedListView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetAlternateRowColors)(ITuiThemedListView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetHeaderColors)(ITuiThemedListView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetHeaderColors)(ITuiThemedListView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    CHAR8 (ANXAPI *GetColumnSeparator)(ITuiThemedListView *This);
    HRESULT (ANXAPI *SetColumnSeparator)(ITuiThemedListView *This, CHAR8 Separator);
} ITuiThemedListView_Vtbl;

struct _ITuiThemedListView {
    CONST ITuiThemedListView_Vtbl *Vtbl;
};

// {7D8E9F0A-1B2C-3D4E-5F6A-7B8C9D0E1F2A}
DEFINE_GUID(IID_ITuiListView,
    0x7D8E9F0A, 0x1B2C, 0x3D4E, 0x5F, 0x6A, 0x7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A);

/**
  ITuiListView Interface

  Multi-column list control with resizable columns, different view modes,
  alternating rows, checkboxes, and inline editing. Inherits from ITuiThemedListView.
**/
typedef struct _ITuiListView_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiListView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiListView *This);
    UINTN (ANXAPI *Release)(ITuiListView *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiListView *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiListView *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiListView *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiListView *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiListView *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiListView *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiListView *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiListView *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiListView *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiListView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiListView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiListView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiListView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiListView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiListView *This);
    HRESULT (ANXAPI *SetParent)(ITuiListView *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiListView *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiListView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiListView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiListView *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiListView *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiListView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiListView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiListView *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiListView *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiListView *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedListView methods
    HRESULT (ANXAPI *GetSelectedColors)(ITuiListView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectedColors)(ITuiListView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetAlternateRowColors)(ITuiListView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetAlternateRowColors)(ITuiListView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetHeaderColors)(ITuiListView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetHeaderColors)(ITuiListView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    CHAR8 (ANXAPI *GetColumnSeparator)(ITuiListView *This);
    HRESULT (ANXAPI *SetColumnSeparator)(ITuiListView *This, CHAR8 Separator);

    // ITuiListView methods
    HRESULT (ANXAPI *AddColumn)(ITuiListView *This, CONST CHAR8 *Header, UINT32 Width);
    HRESULT (ANXAPI *RemoveColumn)(ITuiListView *This, UINT32 ColumnIndex);
    UINT32 (ANXAPI *GetColumnCount)(ITuiListView *This);
    HRESULT (ANXAPI *SetColumnWidth)(ITuiListView *This, UINT32 ColumnIndex, UINT32 Width);
    HRESULT (ANXAPI *GetColumnWidth)(ITuiListView *This, UINT32 ColumnIndex, UINT32 *Width);
    HRESULT (ANXAPI *AddItem)(ITuiListView *This, CONST CHAR8 **Cells, UINT32 CellCount, VOID *UserData, UINT32 *OutIndex);
    HRESULT (ANXAPI *RemoveItem)(ITuiListView *This, UINT32 ItemIndex);
    UINT32 (ANXAPI *GetItemCount)(ITuiListView *This);
    HRESULT (ANXAPI *Clear)(ITuiListView *This);
    HRESULT (ANXAPI *SetMode)(ITuiListView *This, UINT32 Mode);
    UINT32 (ANXAPI *GetMode)(ITuiListView *This);
    HRESULT (ANXAPI *SetVirtualMode)(ITuiListView *This, BOOLEAN Enable, UINT32 ItemCount, HRESULT (*Callback)(VOID *UserData, UINT32 Index, CHAR8 **OutCells, UINT32 *OutCellCount, BOOLEAN *OutChecked), VOID *UserData);
    HRESULT (ANXAPI *GetSelectedItem)(ITuiListView *This, INT32 *ItemIndex);
    HRESULT (ANXAPI *SetSelectedItem)(ITuiListView *This, INT32 ItemIndex);
    HRESULT (ANXAPI *GetItemUserData)(ITuiListView *This, UINT32 ItemIndex, VOID **UserData);
} ITuiListView_Vtbl;

struct _ITuiListView {
    CONST ITuiListView_Vtbl *Vtbl;
};

// {1A2B3C4D-5E6F-7A8B-9C0D-1E2F3A4B5C6D}
DEFINE_GUID(IID_ITuiThemedWizard,
    0x1A2B3C4D, 0x5E6F, 0x7A8B, 0x9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D);

/**
  ITuiThemedWizard Interface

  Wizard theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedWizard_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedWizard *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedWizard *This);
    UINTN (ANXAPI *Release)(ITuiThemedWizard *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedWizard *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedWizard *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedWizard *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedWizard *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedWizard *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedWizard *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedWizard *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedWizard *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedWizard *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedWizard *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedWizard *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedWizard *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedWizard *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedWizard *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedWizard *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedWizard *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedWizard *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedWizard *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedWizard *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedWizard *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedWizard *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedWizard *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedWizard *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedWizard *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedWizard methods
    HRESULT (ANXAPI *GetButtonColors)(ITuiThemedWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetButtonColors)(ITuiThemedWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetDisabledButtonColors)(ITuiThemedWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetDisabledButtonColors)(ITuiThemedWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetProgressColors)(ITuiThemedWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetProgressColors)(ITuiThemedWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetTitleColors)(ITuiThemedWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetTitleColors)(ITuiThemedWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetDescriptionColors)(ITuiThemedWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetDescriptionColors)(ITuiThemedWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    CHAR8 (ANXAPI *GetProgressChar)(ITuiThemedWizard *This);
    HRESULT (ANXAPI *SetProgressChar)(ITuiThemedWizard *This, CHAR8 ProgressChar);
} ITuiThemedWizard_Vtbl;

struct _ITuiThemedWizard {
    CONST ITuiThemedWizard_Vtbl *Vtbl;
};

// {8E9F0A1B-2C3D-4E5F-6A7B-8C9D0E1F2A3B}
DEFINE_GUID(IID_ITuiWizard,
    0x8E9F0A1B, 0x2C3D, 0x4E5F, 0x6A, 0x7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B);

/**
  ITuiWizard Interface

  Multi-step workflow dialog with Back/Next/Finish/Cancel buttons,
  progress indicator, and page validation. Inherits from ITuiThemedWizard.
**/
typedef struct _ITuiWizard_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiWizard *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiWizard *This);
    UINTN (ANXAPI *Release)(ITuiWizard *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiWizard *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiWizard *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiWizard *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiWizard *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiWizard *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiWizard *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiWizard *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiWizard *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiWizard *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiWizard *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiWizard *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiWizard *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiWizard *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiWizard *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiWizard *This);
    HRESULT (ANXAPI *SetParent)(ITuiWizard *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiWizard *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiWizard *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiWizard *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiWizard *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiWizard *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiWizard *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiWizard *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiWizard *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedWizard methods
    HRESULT (ANXAPI *GetButtonColors)(ITuiWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetButtonColors)(ITuiWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetDisabledButtonColors)(ITuiWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetDisabledButtonColors)(ITuiWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetProgressColors)(ITuiWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetProgressColors)(ITuiWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetTitleColors)(ITuiWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetTitleColors)(ITuiWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetDescriptionColors)(ITuiWizard *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetDescriptionColors)(ITuiWizard *This, TUI_COLOR Foreground, TUI_COLOR Background);
    CHAR8 (ANXAPI *GetProgressChar)(ITuiWizard *This);
    HRESULT (ANXAPI *SetProgressChar)(ITuiWizard *This, CHAR8 ProgressChar);

    // ITuiWizard methods
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
    HRESULT (ANXAPI *GoNext)(ITuiWizard *This);
    HRESULT (ANXAPI *GoBack)(ITuiWizard *This);
    HRESULT (ANXAPI *Finish)(ITuiWizard *This);
    HRESULT (ANXAPI *Reset)(ITuiWizard *This);
    HRESULT (ANXAPI *SetFinishCallback)(
        ITuiWizard *This,
        HRESULT (*Callback)(VOID *UserData),
        VOID *UserData
    );
    UINT32 (ANXAPI *GetCurrentPage)(ITuiWizard *This);
    UINT32 (ANXAPI *GetPageCount)(ITuiWizard *This);
    HRESULT (ANXAPI *SetCurrentPage)(ITuiWizard *This, UINT32 PageIndex);
} ITuiWizard_Vtbl;

struct _ITuiWizard {
    CONST ITuiWizard_Vtbl *Vtbl;
};

// {2B3C4D5E-6F7A-8B9C-0D1E-2F3A4B5C6D7E}
DEFINE_GUID(IID_ITuiThemedFlexContainer,
    0x2B3C4D5E, 0x6F7A, 0x8B9C, 0x0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E);

/**
  ITuiThemedFlexContainer Interface

  FlexContainer theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedFlexContainer_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedFlexContainer *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedFlexContainer *This);
    UINTN (ANXAPI *Release)(ITuiThemedFlexContainer *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedFlexContainer *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedFlexContainer *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedFlexContainer *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedFlexContainer *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedFlexContainer *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedFlexContainer *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedFlexContainer *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedFlexContainer *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedFlexContainer *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedFlexContainer *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedFlexContainer *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedFlexContainer *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedFlexContainer *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedFlexContainer *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedFlexContainer *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedFlexContainer *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedFlexContainer *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedFlexContainer *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedFlexContainer *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedFlexContainer *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedFlexContainer *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedFlexContainer *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedFlexContainer *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedFlexContainer *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedFlexContainer *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedFlexContainer *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedFlexContainer methods (minimal for containers)
    HRESULT (ANXAPI *GetBorderColors)(ITuiThemedFlexContainer *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetBorderColors)(ITuiThemedFlexContainer *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedFlexContainer_Vtbl;

struct _ITuiThemedFlexContainer {
    CONST ITuiThemedFlexContainer_Vtbl *Vtbl;
};

// {9F0A1B2C-3D4E-5F6A-7B8C-9D0E1F2A3B4C}
DEFINE_GUID(IID_ITuiFlexContainer,
    0x9F0A1B2C, 0x3D4E, 0x5F6A, 0x7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C);

/**
  ITuiFlexContainer Interface

  Flexbox-like layout container with support for row/column direction,
  flex-grow/shrink, alignment, justify content, wrapping, and gaps.
  Inherits from ITuiThemedFlexContainer.
**/
typedef struct _ITuiFlexContainer_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiFlexContainer *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiFlexContainer *This);
    UINTN (ANXAPI *Release)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiFlexContainer *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiFlexContainer *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiFlexContainer *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiFlexContainer *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiFlexContainer *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiFlexContainer *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiFlexContainer *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiFlexContainer *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiFlexContainer *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiFlexContainer *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiFlexContainer *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *SetParent)(ITuiFlexContainer *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiFlexContainer *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiFlexContainer *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiFlexContainer *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiFlexContainer *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiFlexContainer *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiFlexContainer *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiFlexContainer *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiFlexContainer *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiFlexContainer *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiFlexContainer *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedFlexContainer methods
    HRESULT (ANXAPI *GetBorderColors)(ITuiFlexContainer *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetBorderColors)(ITuiFlexContainer *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiFlexContainer methods
    HRESULT (ANXAPI *AddFlexChild)(ITuiFlexContainer *This, VOID *Widget, UINT32 FlexGrow, UINT32 FlexShrink, INT32 FlexBasis);
    HRESULT (ANXAPI *RemoveFlexChild)(ITuiFlexContainer *This, VOID *Widget);
    HRESULT (ANXAPI *SetDirection)(ITuiFlexContainer *This, UINT32 Direction);
    UINT32 (ANXAPI *GetDirection)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *SetJustifyContent)(ITuiFlexContainer *This, UINT32 Justify);
    UINT32 (ANXAPI *GetJustifyContent)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *SetAlignItems)(ITuiFlexContainer *This, UINT32 Align);
    UINT32 (ANXAPI *GetAlignItems)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *SetGap)(ITuiFlexContainer *This, UINT32 Gap);
    UINT32 (ANXAPI *GetGap)(ITuiFlexContainer *This);
    HRESULT (ANXAPI *SetPadding)(ITuiFlexContainer *This, UINT32 Top, UINT32 Right, UINT32 Bottom, UINT32 Left);
    HRESULT (ANXAPI *GetPadding)(ITuiFlexContainer *This, UINT32 *Top, UINT32 *Right, UINT32 *Bottom, UINT32 *Left);
} ITuiFlexContainer_Vtbl;

struct _ITuiFlexContainer {
    CONST ITuiFlexContainer_Vtbl *Vtbl;
};

// {3C4D5E6F-7A8B-9C0D-1E2F-3A4B5C6D7E8F}
DEFINE_GUID(IID_ITuiThemedVBox,
    0x3C4D5E6F, 0x7A8B, 0x9C0D, 0x1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F);

/**
  ITuiThemedVBox Interface

  VBox theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedVBox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedVBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedVBox *This);
    UINTN (ANXAPI *Release)(ITuiThemedVBox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedVBox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedVBox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedVBox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedVBox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedVBox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedVBox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedVBox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedVBox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedVBox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedVBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedVBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedVBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedVBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedVBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedVBox *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedVBox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedVBox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedVBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedVBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedVBox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedVBox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedVBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedVBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedVBox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedVBox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedVBox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedVBox methods
    HRESULT (ANXAPI *GetSeparatorColors)(ITuiThemedVBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSeparatorColors)(ITuiThemedVBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedVBox_Vtbl;

struct _ITuiThemedVBox {
    CONST ITuiThemedVBox_Vtbl *Vtbl;
};

// {A0B1C2D3-4E5F-6A7B-8C9D-0E1F2A3B4C5D}
DEFINE_GUID(IID_ITuiVBox,
    0xA0B1C2D3, 0x4E5F, 0x6A7B, 0x8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D);

/**
  ITuiVBox Interface

  Vertical box container for simple top-to-bottom stacking.
  Inherits from ITuiThemedVBox.
**/
typedef struct _ITuiVBox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiVBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiVBox *This);
    UINTN (ANXAPI *Release)(ITuiVBox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiVBox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiVBox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiVBox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiVBox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiVBox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiVBox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiVBox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiVBox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiVBox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiVBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiVBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiVBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiVBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiVBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiVBox *This);
    HRESULT (ANXAPI *SetParent)(ITuiVBox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiVBox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiVBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiVBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiVBox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiVBox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiVBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiVBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiVBox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiVBox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiVBox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedVBox methods
    HRESULT (ANXAPI *GetSeparatorColors)(ITuiVBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSeparatorColors)(ITuiVBox *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiVBox methods
    HRESULT (ANXAPI *PackStart)(ITuiVBox *This, VOID *Widget, BOOLEAN Expand, BOOLEAN Fill, UINT32 Padding);
    HRESULT (ANXAPI *SetSpacing)(ITuiVBox *This, UINT32 Spacing);
    UINT32 (ANXAPI *GetSpacing)(ITuiVBox *This);
    HRESULT (ANXAPI *SetHomogeneous)(ITuiVBox *This, BOOLEAN Homogeneous);
    BOOLEAN (ANXAPI *GetHomogeneous)(ITuiVBox *This);
} ITuiVBox_Vtbl;

struct _ITuiVBox {
    CONST ITuiVBox_Vtbl *Vtbl;
};

// {4D5E6F7A-8B9C-0D1E-2F3A-4B5C6D7E8F9A}
DEFINE_GUID(IID_ITuiThemedHBox,
    0x4D5E6F7A, 0x8B9C, 0x0D1E, 0x2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A);

/**
  ITuiThemedHBox Interface

  HBox theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedHBox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedHBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedHBox *This);
    UINTN (ANXAPI *Release)(ITuiThemedHBox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedHBox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedHBox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedHBox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedHBox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedHBox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedHBox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedHBox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedHBox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedHBox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedHBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedHBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedHBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedHBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedHBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedHBox *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedHBox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedHBox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedHBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedHBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedHBox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedHBox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedHBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedHBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedHBox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedHBox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedHBox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedHBox methods
    HRESULT (ANXAPI *GetSeparatorColors)(ITuiThemedHBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSeparatorColors)(ITuiThemedHBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedHBox_Vtbl;

struct _ITuiThemedHBox {
    CONST ITuiThemedHBox_Vtbl *Vtbl;
};

// {B1C2D3E4-5F6A-7B8C-9D0E-1F2A3B4C5D6E}
DEFINE_GUID(IID_ITuiHBox,
    0xB1C2D3E4, 0x5F6A, 0x7B8C, 0x9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E);

/**
  ITuiHBox Interface

  Horizontal box container for simple left-to-right stacking.
  Inherits from ITuiThemedHBox.
**/
typedef struct _ITuiHBox_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiHBox *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiHBox *This);
    UINTN (ANXAPI *Release)(ITuiHBox *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiHBox *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiHBox *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiHBox *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiHBox *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiHBox *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiHBox *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiHBox *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiHBox *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiHBox *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiHBox *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiHBox *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiHBox *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiHBox *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiHBox *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiHBox *This);
    HRESULT (ANXAPI *SetParent)(ITuiHBox *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiHBox *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiHBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiHBox *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiHBox *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiHBox *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiHBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiHBox *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiHBox *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiHBox *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiHBox *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedHBox methods
    HRESULT (ANXAPI *GetSeparatorColors)(ITuiHBox *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSeparatorColors)(ITuiHBox *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiHBox methods
    HRESULT (ANXAPI *PackStart)(ITuiHBox *This, VOID *Widget, BOOLEAN Expand, BOOLEAN Fill, UINT32 Padding);
    HRESULT (ANXAPI *SetSpacing)(ITuiHBox *This, UINT32 Spacing);
    UINT32 (ANXAPI *GetSpacing)(ITuiHBox *This);
    HRESULT (ANXAPI *SetHomogeneous)(ITuiHBox *This, BOOLEAN Homogeneous);
    BOOLEAN (ANXAPI *GetHomogeneous)(ITuiHBox *This);
} ITuiHBox_Vtbl;

struct _ITuiHBox {
    CONST ITuiHBox_Vtbl *Vtbl;
};

// {5E6F7A8B-9C0D-1E2F-3A4B-5C6D7E8F9A0B}
DEFINE_GUID(IID_ITuiThemedGrid,
    0x5E6F7A8B, 0x9C0D, 0x1E2F, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B);

/**
  ITuiThemedGrid Interface

  Grid theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedGrid_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedGrid *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedGrid *This);
    UINTN (ANXAPI *Release)(ITuiThemedGrid *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedGrid *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedGrid *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedGrid *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedGrid *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedGrid *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedGrid *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedGrid *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedGrid *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedGrid *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedGrid *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedGrid *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedGrid *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedGrid *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedGrid *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedGrid *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedGrid *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedGrid *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedGrid *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedGrid *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedGrid *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedGrid *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedGrid *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedGrid *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedGrid *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedGrid *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedGrid *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedGrid methods
    HRESULT (ANXAPI *GetGridLineColors)(ITuiThemedGrid *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetGridLineColors)(ITuiThemedGrid *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedGrid_Vtbl;

struct _ITuiThemedGrid {
    CONST ITuiThemedGrid_Vtbl *Vtbl;
};

// {C2D3E4F5-6A7B-8C9D-0E1F-2A3B4C5D6E7F}
DEFINE_GUID(IID_ITuiGrid,
    0xC2D3E4F5, 0x6A7B, 0x8C9D, 0x0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F);

/**
  ITuiGrid Interface

  Grid layout container that arranges children in rows and columns
  with support for spanning, padding, and alignment. Inherits from ITuiThemedGrid.
**/
typedef struct _ITuiGrid_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiGrid *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiGrid *This);
    UINTN (ANXAPI *Release)(ITuiGrid *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiGrid *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiGrid *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiGrid *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiGrid *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiGrid *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiGrid *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiGrid *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiGrid *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiGrid *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiGrid *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiGrid *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiGrid *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiGrid *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiGrid *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiGrid *This);
    HRESULT (ANXAPI *SetParent)(ITuiGrid *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiGrid *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiGrid *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiGrid *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiGrid *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiGrid *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiGrid *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiGrid *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiGrid *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiGrid *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiGrid *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedGrid methods
    HRESULT (ANXAPI *GetGridLineColors)(ITuiGrid *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetGridLineColors)(ITuiGrid *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiGrid methods
    HRESULT (ANXAPI *Attach)(ITuiGrid *This, VOID *Widget, UINT32 Column, UINT32 Row, UINT32 ColumnSpan, UINT32 RowSpan);
    HRESULT (ANXAPI *SetSpacing)(ITuiGrid *This, UINT32 RowSpacing, UINT32 ColumnSpacing);
    HRESULT (ANXAPI *GetSpacing)(ITuiGrid *This, UINT32 *RowSpacing, UINT32 *ColumnSpacing);
    HRESULT (ANXAPI *SetRowHeight)(ITuiGrid *This, UINT32 Row, UINT32 Height);
    HRESULT (ANXAPI *GetRowHeight)(ITuiGrid *This, UINT32 Row, UINT32 *Height);
    HRESULT (ANXAPI *SetColumnWidth)(ITuiGrid *This, UINT32 Column, UINT32 Width);
    HRESULT (ANXAPI *GetColumnWidth)(ITuiGrid *This, UINT32 Column, UINT32 *Width);
} ITuiGrid_Vtbl;

struct _ITuiGrid {
    CONST ITuiGrid_Vtbl *Vtbl;
};

// {6F7A8B9C-0D1E-2F3A-4B5C-6D7E8F9A0B1C}
DEFINE_GUID(IID_ITuiThemedSplitView,
    0x6F7A8B9C, 0x0D1E, 0x2F3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C);

/**
  ITuiThemedSplitView Interface

  SplitView theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedSplitView_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedSplitView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedSplitView *This);
    UINTN (ANXAPI *Release)(ITuiThemedSplitView *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedSplitView *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedSplitView *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedSplitView *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedSplitView *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedSplitView *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedSplitView *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedSplitView *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedSplitView *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedSplitView *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedSplitView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedSplitView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedSplitView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedSplitView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedSplitView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedSplitView *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedSplitView *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedSplitView *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedSplitView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedSplitView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedSplitView *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedSplitView *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedSplitView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedSplitView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedSplitView *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedSplitView *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedSplitView *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedSplitView methods
    HRESULT (ANXAPI *GetDividerColors)(ITuiThemedSplitView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetDividerColors)(ITuiThemedSplitView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    CHAR8 (ANXAPI *GetDividerChar)(ITuiThemedSplitView *This);
    HRESULT (ANXAPI *SetDividerChar)(ITuiThemedSplitView *This, CHAR8 DividerChar);
} ITuiThemedSplitView_Vtbl;

struct _ITuiThemedSplitView {
    CONST ITuiThemedSplitView_Vtbl *Vtbl;
};

// {D3E4F5A6-7B8C-9D0E-1F2A-3B4C5D6E7F8A}
DEFINE_GUID(IID_ITuiSplitView,
    0xD3E4F5A6, 0x7B8C, 0x9D0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A);

/**
  ITuiSplitView Interface

  Resizable two-pane container with draggable divider.
  Supports horizontal and vertical orientation. Inherits from ITuiThemedSplitView.
**/
typedef struct _ITuiSplitView_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiSplitView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiSplitView *This);
    UINTN (ANXAPI *Release)(ITuiSplitView *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiSplitView *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiSplitView *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiSplitView *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiSplitView *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiSplitView *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiSplitView *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiSplitView *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiSplitView *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiSplitView *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiSplitView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiSplitView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiSplitView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiSplitView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiSplitView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiSplitView *This);
    HRESULT (ANXAPI *SetParent)(ITuiSplitView *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiSplitView *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiSplitView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiSplitView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiSplitView *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiSplitView *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiSplitView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiSplitView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiSplitView *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiSplitView *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiSplitView *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedSplitView methods
    HRESULT (ANXAPI *GetDividerColors)(ITuiSplitView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetDividerColors)(ITuiSplitView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    CHAR8 (ANXAPI *GetDividerChar)(ITuiSplitView *This);
    HRESULT (ANXAPI *SetDividerChar)(ITuiSplitView *This, CHAR8 DividerChar);

    // ITuiSplitView methods
    HRESULT (ANXAPI *SetPane1)(ITuiSplitView *This, VOID *Widget);
    HRESULT (ANXAPI *GetPane1)(ITuiSplitView *This, VOID **Widget);
    HRESULT (ANXAPI *SetPane2)(ITuiSplitView *This, VOID *Widget);
    HRESULT (ANXAPI *GetPane2)(ITuiSplitView *This, VOID **Widget);
    HRESULT (ANXAPI *SetSplitPosition)(ITuiSplitView *This, UINT32 Position);
    UINT32 (ANXAPI *GetSplitPosition)(ITuiSplitView *This);
    HRESULT (ANXAPI *SetOrientation)(ITuiSplitView *This, UINT32 Orientation);
    UINT32 (ANXAPI *GetOrientation)(ITuiSplitView *This);
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

// {7A8B9C0D-1E2F-3A4B-5C6D-7E8F9A0B1C2D}
DEFINE_GUID(IID_ITuiThemedPropertySheet,
    0x7A8B9C0D, 0x1E2F, 0x3A4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D);

/**
  ITuiThemedPropertySheet Interface

  PropertySheet theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedPropertySheet_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedPropertySheet *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedPropertySheet *This);
    UINTN (ANXAPI *Release)(ITuiThemedPropertySheet *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedPropertySheet *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedPropertySheet *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedPropertySheet *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedPropertySheet *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedPropertySheet *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedPropertySheet *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedPropertySheet *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedPropertySheet *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedPropertySheet *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedPropertySheet *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedPropertySheet *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedPropertySheet *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedPropertySheet *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedPropertySheet *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedPropertySheet *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedPropertySheet *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedPropertySheet *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedPropertySheet *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedPropertySheet *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedPropertySheet *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedPropertySheet *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedPropertySheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedPropertySheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedPropertySheet *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedPropertySheet *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedPropertySheet *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedPropertySheet methods
    HRESULT (ANXAPI *GetTabColors)(ITuiThemedPropertySheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetTabColors)(ITuiThemedPropertySheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetActiveTabColors)(ITuiThemedPropertySheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetActiveTabColors)(ITuiThemedPropertySheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetButtonColors)(ITuiThemedPropertySheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetButtonColors)(ITuiThemedPropertySheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetModifiedIndicatorColor)(ITuiThemedPropertySheet *This, TUI_COLOR *Color);
    HRESULT (ANXAPI *SetModifiedIndicatorColor)(ITuiThemedPropertySheet *This, TUI_COLOR Color);
} ITuiThemedPropertySheet_Vtbl;

struct _ITuiThemedPropertySheet {
    CONST ITuiThemedPropertySheet_Vtbl *Vtbl;
};

// {F5A6B7C8-9D0E-1F2A-3B4C-5D6E7F8A9B0C}
DEFINE_GUID(IID_ITuiPropertySheet,
    0xF5A6B7C8, 0x9D0E, 0x1F2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C);

/**
  ITuiPropertySheet Interface

  Tabbed dialog with multiple property pages, OK/Cancel/Apply buttons,
  validation, and modified state tracking. Inherits from ITuiThemedPropertySheet.
**/
typedef struct _ITuiPropertySheet_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiPropertySheet *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiPropertySheet *This);
    UINTN (ANXAPI *Release)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiPropertySheet *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiPropertySheet *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiPropertySheet *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiPropertySheet *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiPropertySheet *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiPropertySheet *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiPropertySheet *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiPropertySheet *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiPropertySheet *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiPropertySheet *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiPropertySheet *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *SetParent)(ITuiPropertySheet *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiPropertySheet *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiPropertySheet *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiPropertySheet *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiPropertySheet *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiPropertySheet *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiPropertySheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiPropertySheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiPropertySheet *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiPropertySheet *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiPropertySheet *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedPropertySheet methods
    HRESULT (ANXAPI *GetTabColors)(ITuiPropertySheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetTabColors)(ITuiPropertySheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetActiveTabColors)(ITuiPropertySheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetActiveTabColors)(ITuiPropertySheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetButtonColors)(ITuiPropertySheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetButtonColors)(ITuiPropertySheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetModifiedIndicatorColor)(ITuiPropertySheet *This, TUI_COLOR *Color);
    HRESULT (ANXAPI *SetModifiedIndicatorColor)(ITuiPropertySheet *This, TUI_COLOR Color);

    // ITuiPropertySheet methods
    HRESULT (ANXAPI *AddPage)(ITuiPropertySheet *This, CONST CHAR8 *Title, CONST CHAR8 *Description, VOID *PageWidget, HRESULT (*OnActivate)(VOID*, VOID*), HRESULT (*OnDeactivate)(VOID*, VOID*), HRESULT (*OnApply)(VOID*, VOID*, BOOLEAN*), HRESULT (*OnValidate)(VOID*, VOID*, BOOLEAN*), HRESULT (*OnReset)(VOID*, VOID*), VOID *UserData);
    HRESULT (ANXAPI *SetActivePage)(ITuiPropertySheet *This, INT32 PageIndex);
    INT32 (ANXAPI *GetActivePage)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *SetPageModified)(ITuiPropertySheet *This, INT32 PageIndex, BOOLEAN Modified);
    BOOLEAN (ANXAPI *GetPageModified)(ITuiPropertySheet *This, INT32 PageIndex);
    UINT32 (ANXAPI *GetPageCount)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *Apply)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *OK)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *Cancel)(ITuiPropertySheet *This);
    HRESULT (ANXAPI *Reset)(ITuiPropertySheet *This);
} ITuiPropertySheet_Vtbl;

struct _ITuiPropertySheet {
    CONST ITuiPropertySheet_Vtbl *Vtbl;
};

// {8B9C0D1E-2F3A-4B5C-6D7E-8F9A0B1C2D3E}
DEFINE_GUID(IID_ITuiThemedSpreadsheet,
    0x8B9C0D1E, 0x2F3A, 0x4B5C, 0x6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E);

/**
  ITuiThemedSpreadsheet Interface

  Spreadsheet theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedSpreadsheet_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedSpreadsheet *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedSpreadsheet *This);
    UINTN (ANXAPI *Release)(ITuiThemedSpreadsheet *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedSpreadsheet *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedSpreadsheet *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedSpreadsheet *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedSpreadsheet *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedSpreadsheet *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedSpreadsheet *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedSpreadsheet *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedSpreadsheet *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedSpreadsheet *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedSpreadsheet *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedSpreadsheet *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedSpreadsheet *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedSpreadsheet *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedSpreadsheet *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedSpreadsheet *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedSpreadsheet *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedSpreadsheet *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedSpreadsheet *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedSpreadsheet *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedSpreadsheet *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedSpreadsheet *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedSpreadsheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedSpreadsheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedSpreadsheet *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedSpreadsheet *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedSpreadsheet *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedSpreadsheet methods
    HRESULT (ANXAPI *GetGridLineColors)(ITuiThemedSpreadsheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetGridLineColors)(ITuiThemedSpreadsheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetSelectionColors)(ITuiThemedSpreadsheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectionColors)(ITuiThemedSpreadsheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetHeaderColors)(ITuiThemedSpreadsheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetHeaderColors)(ITuiThemedSpreadsheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetEditColors)(ITuiThemedSpreadsheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetEditColors)(ITuiThemedSpreadsheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedSpreadsheet_Vtbl;

struct _ITuiThemedSpreadsheet {
    CONST ITuiThemedSpreadsheet_Vtbl *Vtbl;
};

// {A6B7C8D9-0E1F-2A3B-4C5D-6E7F8A9B0C1D}
DEFINE_GUID(IID_ITuiSpreadsheet,
    0xA6B7C8D9, 0x0E1F, 0x2A3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D);

/**
  ITuiSpreadsheet Interface

  Excel-like spreadsheet control with virtual storage, formula evaluation,
  cell editing, selection, freeze panes, and formatting.
  Inherits from ITuiThemedSpreadsheet.
**/
typedef struct _ITuiSpreadsheet_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiSpreadsheet *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiSpreadsheet *This);
    UINTN (ANXAPI *Release)(ITuiSpreadsheet *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiSpreadsheet *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiSpreadsheet *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiSpreadsheet *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiSpreadsheet *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiSpreadsheet *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiSpreadsheet *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiSpreadsheet *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiSpreadsheet *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiSpreadsheet *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiSpreadsheet *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiSpreadsheet *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiSpreadsheet *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiSpreadsheet *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiSpreadsheet *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiSpreadsheet *This);
    HRESULT (ANXAPI *SetParent)(ITuiSpreadsheet *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiSpreadsheet *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiSpreadsheet *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiSpreadsheet *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiSpreadsheet *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiSpreadsheet *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiSpreadsheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiSpreadsheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiSpreadsheet *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiSpreadsheet *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiSpreadsheet *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedSpreadsheet methods
    HRESULT (ANXAPI *GetGridLineColors)(ITuiSpreadsheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetGridLineColors)(ITuiSpreadsheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetSelectionColors)(ITuiSpreadsheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectionColors)(ITuiSpreadsheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetHeaderColors)(ITuiSpreadsheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetHeaderColors)(ITuiSpreadsheet *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetEditColors)(ITuiSpreadsheet *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetEditColors)(ITuiSpreadsheet *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiSpreadsheet methods
    HRESULT (ANXAPI *SetCellValue)(ITuiSpreadsheet *This, UINT32 Row, UINT32 Col, CONST CHAR8 *Value);
    HRESULT (ANXAPI *GetCellValue)(ITuiSpreadsheet *This, UINT32 Row, UINT32 Col, CHAR8 *Value, UINTN ValueSize);
    HRESULT (ANXAPI *SetColumnWidth)(ITuiSpreadsheet *This, UINT32 Column, UINT32 Width);
    HRESULT (ANXAPI *GetColumnWidth)(ITuiSpreadsheet *This, UINT32 Column, UINT32 *Width);
    HRESULT (ANXAPI *SetVirtualMode)(ITuiSpreadsheet *This, BOOLEAN Enable, HRESULT (*OnGetCell)(VOID*, UINT32, UINT32, VOID*), HRESULT (*OnSetCell)(VOID*, UINT32, UINT32, CONST VOID*), VOID *UserData);
    HRESULT (ANXAPI *GetCurrentCell)(ITuiSpreadsheet *This, UINT32 *Row, UINT32 *Col);
    HRESULT (ANXAPI *SetCurrentCell)(ITuiSpreadsheet *This, UINT32 Row, UINT32 Col);
} ITuiSpreadsheet_Vtbl;

struct _ITuiSpreadsheet {
    CONST ITuiSpreadsheet_Vtbl *Vtbl;
};

// {9C0D1E2F-3A4B-5C6D-7E8F-9A0B1C2D3E4F}
DEFINE_GUID(IID_ITuiThemedRuler,
    0x9C0D1E2F, 0x3A4B, 0x5C6D, 0x7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F);

/**
  ITuiThemedRuler Interface

  Ruler theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedRuler_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedRuler *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedRuler *This);
    UINTN (ANXAPI *Release)(ITuiThemedRuler *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedRuler *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedRuler *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedRuler *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedRuler *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedRuler *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedRuler *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedRuler *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedRuler *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedRuler *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedRuler *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedRuler *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedRuler *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedRuler *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedRuler *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedRuler *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedRuler *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedRuler *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedRuler *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedRuler *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedRuler *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedRuler *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedRuler *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedRuler *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedRuler *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedRuler *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedRuler *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedRuler methods
    HRESULT (ANXAPI *GetMarkerColor)(ITuiThemedRuler *This, TUI_COLOR *Color);
    HRESULT (ANXAPI *SetMarkerColor)(ITuiThemedRuler *This, TUI_COLOR Color);
    HRESULT (ANXAPI *GetTabStopColor)(ITuiThemedRuler *This, TUI_COLOR *Color);
    HRESULT (ANXAPI *SetTabStopColor)(ITuiThemedRuler *This, TUI_COLOR Color);
} ITuiThemedRuler_Vtbl;

struct _ITuiThemedRuler {
    CONST ITuiThemedRuler_Vtbl *Vtbl;
};

// {B7C8D9E0-1F2A-3B4C-5D6E-7F8A9B0C1D2E}
DEFINE_GUID(IID_ITuiRuler,
    0xB7C8D9E0, 0x1F2A, 0x3B4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E);

/**
  ITuiRuler Interface

  Horizontal/vertical ruler for text editors showing column/line numbers,
  tab stops, margins, and cursor position. Inherits from ITuiThemedRuler.
**/
typedef struct _ITuiRuler_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiRuler *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiRuler *This);
    UINTN (ANXAPI *Release)(ITuiRuler *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiRuler *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiRuler *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiRuler *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiRuler *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiRuler *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiRuler *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiRuler *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiRuler *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiRuler *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiRuler *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiRuler *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiRuler *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiRuler *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiRuler *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiRuler *This);
    HRESULT (ANXAPI *SetParent)(ITuiRuler *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiRuler *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiRuler *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiRuler *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiRuler *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiRuler *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiRuler *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiRuler *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiRuler *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiRuler *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiRuler *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedRuler methods
    HRESULT (ANXAPI *GetMarkerColor)(ITuiRuler *This, TUI_COLOR *Color);
    HRESULT (ANXAPI *SetMarkerColor)(ITuiRuler *This, TUI_COLOR Color);
    HRESULT (ANXAPI *GetTabStopColor)(ITuiRuler *This, TUI_COLOR *Color);
    HRESULT (ANXAPI *SetTabStopColor)(ITuiRuler *This, TUI_COLOR Color);

    // ITuiRuler methods
    HRESULT (ANXAPI *SetMargins)(ITuiRuler *This, UINT32 LeftMargin, UINT32 RightMargin, UINT32 FirstLineIndent);
    HRESULT (ANXAPI *GetMargins)(ITuiRuler *This, UINT32 *LeftMargin, UINT32 *RightMargin, UINT32 *FirstLineIndent);
    HRESULT (ANXAPI *AddTabStop)(ITuiRuler *This, UINT32 Position, UINT32 Type);
    HRESULT (ANXAPI *ClearTabStops)(ITuiRuler *This);
    HRESULT (ANXAPI *SetCurrentPosition)(ITuiRuler *This, UINT32 Position);
    UINT32 (ANXAPI *GetCurrentPosition)(ITuiRuler *This);
} ITuiRuler_Vtbl;

struct _ITuiRuler {
    CONST ITuiRuler_Vtbl *Vtbl;
};

// {0D1E2F3A-4B5C-6D7E-8F9A-0B1C2D3E4F5A}
DEFINE_GUID(IID_ITuiThemedRichTextEditor,
    0x0D1E2F3A, 0x4B5C, 0x6D7E, 0x8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A);

/**
  ITuiThemedRichTextEditor Interface

  RichTextEditor theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedRichTextEditor_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedRichTextEditor *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedRichTextEditor *This);
    UINTN (ANXAPI *Release)(ITuiThemedRichTextEditor *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedRichTextEditor *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedRichTextEditor *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedRichTextEditor *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedRichTextEditor *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedRichTextEditor *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedRichTextEditor *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedRichTextEditor *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedRichTextEditor *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedRichTextEditor *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedRichTextEditor *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedRichTextEditor *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedRichTextEditor *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedRichTextEditor *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedRichTextEditor *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedRichTextEditor *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedRichTextEditor *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedRichTextEditor *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedRichTextEditor *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedRichTextEditor *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedRichTextEditor *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedRichTextEditor *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedRichTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedRichTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedRichTextEditor *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedRichTextEditor *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedRichTextEditor *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedRichTextEditor methods
    HRESULT (ANXAPI *GetSelectionColors)(ITuiThemedRichTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectionColors)(ITuiThemedRichTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetCodeColors)(ITuiThemedRichTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetCodeColors)(ITuiThemedRichTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);
} ITuiThemedRichTextEditor_Vtbl;

struct _ITuiThemedRichTextEditor {
    CONST ITuiThemedRichTextEditor_Vtbl *Vtbl;
};

// {C8D9E0F1-2A3B-4C5D-6E7F-8A9B0C1D2E3F}
DEFINE_GUID(IID_ITuiRichTextEditor,
    0xC8D9E0F1, 0x2A3B, 0x4C5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C, 0x1D, 0x2E, 0x3F);

/**
  ITuiRichTextEditor Interface

  Full-featured rich text editor similar to Word for DOS/WordPerfect/WordStar
  with formatting, search/replace, block operations, and reveal codes.
  Inherits from ITuiThemedRichTextEditor.
**/
typedef struct _ITuiRichTextEditor_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiRichTextEditor *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiRichTextEditor *This);
    UINTN (ANXAPI *Release)(ITuiRichTextEditor *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiRichTextEditor *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiRichTextEditor *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiRichTextEditor *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiRichTextEditor *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiRichTextEditor *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiRichTextEditor *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiRichTextEditor *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiRichTextEditor *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiRichTextEditor *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiRichTextEditor *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiRichTextEditor *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiRichTextEditor *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiRichTextEditor *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiRichTextEditor *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiRichTextEditor *This);
    HRESULT (ANXAPI *SetParent)(ITuiRichTextEditor *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiRichTextEditor *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiRichTextEditor *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiRichTextEditor *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiRichTextEditor *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiRichTextEditor *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiRichTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiRichTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiRichTextEditor *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiRichTextEditor *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiRichTextEditor *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedRichTextEditor methods
    HRESULT (ANXAPI *GetSelectionColors)(ITuiRichTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetSelectionColors)(ITuiRichTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *GetCodeColors)(ITuiRichTextEditor *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetCodeColors)(ITuiRichTextEditor *This, TUI_COLOR Foreground, TUI_COLOR Background);

    // ITuiRichTextEditor methods
    HRESULT (ANXAPI *SetText)(ITuiRichTextEditor *This, CONST CHAR8 *Text);
    HRESULT (ANXAPI *GetText)(ITuiRichTextEditor *This, CHAR8 **Text, UINTN *Length);
    HRESULT (ANXAPI *ToggleFormat)(ITuiRichTextEditor *This, UINT32 Attribute);
    HRESULT (ANXAPI *SetRuler)(ITuiRichTextEditor *This, ITuiRuler *Ruler);
    HRESULT (ANXAPI *GetRuler)(ITuiRichTextEditor *This, ITuiRuler **Ruler);
} ITuiRichTextEditor_Vtbl;

struct _ITuiRichTextEditor {
    CONST ITuiRichTextEditor_Vtbl *Vtbl;
};

// {1E2F3A4B-5C6D-7E8F-9A0B-1C2D3E4F5A6B}
DEFINE_GUID(IID_ITuiThemedHeaderView,
    0x1E2F3A4B, 0x5C6D, 0x7E8F, 0x9A, 0x0B, 0x1C, 0x2D, 0x3E, 0x4F, 0x5A, 0x6B);

/**
  ITuiThemedHeaderView Interface

  HeaderView theming interface. Inherits from ITuiThemedWidget.
**/
typedef struct _ITuiThemedHeaderView_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiThemedHeaderView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiThemedHeaderView *This);
    UINTN (ANXAPI *Release)(ITuiThemedHeaderView *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiThemedHeaderView *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiThemedHeaderView *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiThemedHeaderView *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiThemedHeaderView *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiThemedHeaderView *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiThemedHeaderView *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiThemedHeaderView *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiThemedHeaderView *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiThemedHeaderView *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiThemedHeaderView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiThemedHeaderView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiThemedHeaderView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiThemedHeaderView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiThemedHeaderView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiThemedHeaderView *This);
    HRESULT (ANXAPI *SetParent)(ITuiThemedHeaderView *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiThemedHeaderView *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiThemedHeaderView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiThemedHeaderView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiThemedHeaderView *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiThemedHeaderView *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiThemedHeaderView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiThemedHeaderView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiThemedHeaderView *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiThemedHeaderView *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiThemedHeaderView *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedHeaderView methods
    HRESULT (ANXAPI *GetSortIndicatorColor)(ITuiThemedHeaderView *This, TUI_COLOR *Color);
    HRESULT (ANXAPI *SetSortIndicatorColor)(ITuiThemedHeaderView *This, TUI_COLOR Color);
    HRESULT (ANXAPI *GetSeparatorColor)(ITuiThemedHeaderView *This, TUI_COLOR *Color);
    HRESULT (ANXAPI *SetSeparatorColor)(ITuiThemedHeaderView *This, TUI_COLOR Color);
} ITuiThemedHeaderView_Vtbl;

struct _ITuiThemedHeaderView {
    CONST ITuiThemedHeaderView_Vtbl *Vtbl;
};

// {E5F6A7B8-C9D0-4E1F-2A3B-4C5D6E7F8A9B}
DEFINE_GUID(IID_ITuiHeaderView,
    0xE5F6A7B8, 0xC9D0, 0x4E1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B);

/**
  ITuiHeaderView Interface

  Reusable column header control for list views, tree views, and spreadsheets.
  Features sortable, resizable, and reorderable columns. Inherits from ITuiThemedHeaderView.
**/
typedef struct _ITuiHeaderView_Vtbl {
    // ITuiSerializable methods
    HRESULT (ANXAPI *QueryInterface)(ITuiHeaderView *This, REFIID riid, VOID **ppvObject);
    UINTN (ANXAPI *AddRef)(ITuiHeaderView *This);
    UINTN (ANXAPI *Release)(ITuiHeaderView *This);
    HRESULT (ANXAPI *SerializeToYaml)(ITuiHeaderView *This, CHAR8 **OutYaml, UINTN *OutLength);
    HRESULT (ANXAPI *DeserializeFromYaml)(ITuiHeaderView *This, CONST CHAR8 *Yaml, UINTN Length);
    HRESULT (ANXAPI *GetTypeName)(ITuiHeaderView *This, CONST CHAR8 **OutTypeName);
    HRESULT (ANXAPI *Clone)(ITuiHeaderView *This, ITuiSerializable **OutClone);

    // ITuiResponder methods
    HRESULT (ANXAPI *GetNextResponder)(ITuiHeaderView *This, ITuiResponder **NextResponder);
    HRESULT (ANXAPI *SetNextResponder)(ITuiHeaderView *This, ITuiResponder *NextResponder);
    BOOLEAN (ANXAPI *AcceptsFirstResponder)(ITuiHeaderView *This);
    HRESULT (ANXAPI *BecomeFirstResponder)(ITuiHeaderView *This);
    HRESULT (ANXAPI *ResignFirstResponder)(ITuiHeaderView *This);

    // ITuiWidget methods
    HRESULT (ANXAPI *SetBounds)(ITuiHeaderView *This, CONST TUI_RECT *Bounds);
    HRESULT (ANXAPI *GetBounds)(ITuiHeaderView *This, TUI_RECT *Bounds);
    HRESULT (ANXAPI *SetVisible)(ITuiHeaderView *This, BOOLEAN Visible);
    BOOLEAN (ANXAPI *IsVisible)(ITuiHeaderView *This);
    HRESULT (ANXAPI *SetEnabled)(ITuiHeaderView *This, BOOLEAN Enabled);
    BOOLEAN (ANXAPI *IsEnabled)(ITuiHeaderView *This);
    HRESULT (ANXAPI *SetParent)(ITuiHeaderView *This, ITuiWidget *Parent);
    HRESULT (ANXAPI *GetParent)(ITuiHeaderView *This, ITuiWidget **Parent);
    HRESULT (ANXAPI *AddChild)(ITuiHeaderView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *RemoveChild)(ITuiHeaderView *This, ITuiWidget *Child);
    HRESULT (ANXAPI *SetNeedsDisplay)(ITuiHeaderView *This, BOOLEAN Needed);

    // ITuiThemedWidget methods
    HRESULT (ANXAPI *ApplyTheme)(ITuiHeaderView *This, ITuiTheme *Theme);
    HRESULT (ANXAPI *GetColors)(ITuiHeaderView *This, TUI_COLOR *Foreground, TUI_COLOR *Background);
    HRESULT (ANXAPI *SetColors)(ITuiHeaderView *This, TUI_COLOR Foreground, TUI_COLOR Background);
    HRESULT (ANXAPI *OnMouseEvent)(ITuiHeaderView *This, CONST TUI_MOUSE_EVENT *Event, BOOLEAN *Handled);
    HRESULT (ANXAPI *OnKeyEvent)(ITuiHeaderView *This, TUI_KEY Key, UINT32 Modifiers, BOOLEAN *Handled);
    HRESULT (ANXAPI *Draw)(ITuiHeaderView *This, ITuiSurface *Surface, CONST TUI_RECT *DirtyRect);

    // ITuiThemedHeaderView methods
    HRESULT (ANXAPI *GetSortIndicatorColor)(ITuiHeaderView *This, TUI_COLOR *Color);
    HRESULT (ANXAPI *SetSortIndicatorColor)(ITuiHeaderView *This, TUI_COLOR Color);
    HRESULT (ANXAPI *GetSeparatorColor)(ITuiHeaderView *This, TUI_COLOR *Color);
    HRESULT (ANXAPI *SetSeparatorColor)(ITuiHeaderView *This, TUI_COLOR Color);

    // ITuiHeaderView methods
    HRESULT (ANXAPI *AddSection)(ITuiHeaderView *This, CONST CHAR8 *Title, UINT32 Width);
    HRESULT (ANXAPI *SetSortIndicator)(ITuiHeaderView *This, UINT32 SectionIndex, UINT32 SortOrder);
    UINT32 (ANXAPI *GetSortIndicator)(ITuiHeaderView *This, UINT32 SectionIndex);
    HRESULT (ANXAPI *GetSectionWidth)(ITuiHeaderView *This, UINT32 SectionIndex, UINT32 *OutWidth);
    UINT32 (ANXAPI *GetSectionCount)(ITuiHeaderView *This);
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
