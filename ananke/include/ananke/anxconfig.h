/** @file
  ANANKE Configuration System (ANXCONFIG)

  Portable configuration system similar to Linux kconfig/menuconfig.
  Uses YAML files for configuration definitions and generates CMake cache files.

  Copyright (C) 2025 A•NUX Project
  SPDX-License-Identifier: BSD-2-Clause
**/

#ifndef __ANANKE_ANXCONFIG_H__
#define __ANANKE_ANXCONFIG_H__

#include <ananke/base.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Forward declarations
//
typedef struct _IConfigDatabase IConfigDatabase;
typedef struct _IConfigItem IConfigItem;
typedef struct _IConfigGenerator IConfigGenerator;

//
// Configuration Item Types
//
typedef enum _CONFIG_ITEM_TYPE {
    ConfigItemTypeUnknown = 0,
    ConfigItemTypeMenu,       // Submenu
    ConfigItemTypeBoolean,    // ON/OFF option
    ConfigItemTypeTristate,   // YES/NO/MODULE
    ConfigItemTypeChoice,     // Multiple choice (radio buttons)
    ConfigItemTypeString,     // String value
    ConfigItemTypeInteger,    // Integer value
    ConfigItemTypeHex,        // Hexadecimal value
    ConfigItemTypeSeparator,  // Visual separator
    ConfigItemTypeComment     // Information text
} CONFIG_ITEM_TYPE;

//
// Configuration Value
//
typedef union _CONFIG_VALUE {
    BOOLEAN Boolean;
    INT32 Integer;
    UINT32 Hex;
    CHAR8 String[256];
    enum {
        TristateNo,
        TristateModule,
        TristateYes
    } Tristate;
} CONFIG_VALUE;

// {D4E5F6A7-B8C9-4D0E-1F2A-3B4C5D6E7F8A}
DEFINE_GUID(IID_IConfigItem,
    0xD4E5F6A7, 0xB8C9, 0x4D0E, 0x1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A);

/**
  IConfigItem Interface

  Represents a single configuration item.
**/
typedef struct _IConfigItem_Vtbl {
    //
    // IUnknown methods
    //
    HRESULT (ANXAPI *QueryInterface)(
        IConfigItem *This,
        REFIID riid,
        VOID **ppvObject
    );

    UINTN (ANXAPI *AddRef)(
        IConfigItem *This
    );

    UINTN (ANXAPI *Release)(
        IConfigItem *This
    );

    //
    // IConfigItem methods
    //

    /**
      Get item type.
    **/
    HRESULT (ANXAPI *GetType)(
        IConfigItem *This,
        CONFIG_ITEM_TYPE *Type
    );

    /**
      Get item name (symbol name like "CONFIG_FOO").
    **/
    HRESULT (ANXAPI *GetName)(
        IConfigItem *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Get item prompt (display text).
    **/
    HRESULT (ANXAPI *GetPrompt)(
        IConfigItem *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Get item help text.
    **/
    HRESULT (ANXAPI *GetHelp)(
        IConfigItem *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Get current value.
    **/
    HRESULT (ANXAPI *GetValue)(
        IConfigItem *This,
        CONFIG_VALUE *Value
    );

    /**
      Set value.
    **/
    HRESULT (ANXAPI *SetValue)(
        IConfigItem *This,
        CONST CONFIG_VALUE *Value
    );

    /**
      Get default value.
    **/
    HRESULT (ANXAPI *GetDefault)(
        IConfigItem *This,
        CONFIG_VALUE *Value
    );

    /**
      Check if item is visible (depends evaluation).
    **/
    HRESULT (ANXAPI *IsVisible)(
        IConfigItem *This,
        BOOLEAN *Visible
    );

    /**
      Get dependency expression.
    **/
    HRESULT (ANXAPI *GetDependency)(
        IConfigItem *This,
        CHAR8 *Buffer,
        UINTN BufferSize
    );

    /**
      Get number of child items (for menus/choices).
    **/
    HRESULT (ANXAPI *GetChildCount)(
        IConfigItem *This,
        UINTN *Count
    );

    /**
      Get child item by index.
    **/
    HRESULT (ANXAPI *GetChild)(
        IConfigItem *This,
        UINTN Index,
        IConfigItem **Child
    );

} IConfigItem_Vtbl;

struct _IConfigItem {
    CONST IConfigItem_Vtbl *Vtbl;
};

// {E5F6A7B8-C9D0-4E1F-2A3B-4C5D6E7F8A9B}
DEFINE_GUID(IID_IConfigDatabase,
    0xE5F6A7B8, 0xC9D0, 0x4E1F, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B);

/**
  IConfigDatabase Interface

  Manages the entire configuration database.
**/
typedef struct _IConfigDatabase_Vtbl {
    //
    // IUnknown methods
    //
    HRESULT (ANXAPI *QueryInterface)(
        IConfigDatabase *This,
        REFIID riid,
        VOID **ppvObject
    );

    UINTN (ANXAPI *AddRef)(
        IConfigDatabase *This
    );

    UINTN (ANXAPI *Release)(
        IConfigDatabase *This
    );

    //
    // IConfigDatabase methods
    //

    /**
      Load configuration from YAML file.
    **/
    HRESULT (ANXAPI *LoadFromFile)(
        IConfigDatabase *This,
        CONST CHAR8 *FilePath
    );

    /**
      Load configuration values from file (.config).
    **/
    HRESULT (ANXAPI *LoadValues)(
        IConfigDatabase *This,
        CONST CHAR8 *FilePath
    );

    /**
      Save configuration values to file.
    **/
    HRESULT (ANXAPI *SaveValues)(
        IConfigDatabase *This,
        CONST CHAR8 *FilePath
    );

    /**
      Get root menu item.
    **/
    HRESULT (ANXAPI *GetRootItem)(
        IConfigDatabase *This,
        IConfigItem **RootItem
    );

    /**
      Find item by name (symbol).
    **/
    HRESULT (ANXAPI *FindItem)(
        IConfigDatabase *This,
        CONST CHAR8 *Name,
        IConfigItem **Item
    );

    /**
      Evaluate all dependencies.
    **/
    HRESULT (ANXAPI *EvaluateDependencies)(
        IConfigDatabase *This
    );

    /**
      Get total number of items.
    **/
    HRESULT (ANXAPI *GetItemCount)(
        IConfigDatabase *This,
        UINTN *Count
    );

} IConfigDatabase_Vtbl;

struct _IConfigDatabase {
    CONST IConfigDatabase_Vtbl *Vtbl;
};

// {F6A7B8C9-D0E1-4F2A-3B4C-5D6E7F8A9B0C}
DEFINE_GUID(IID_IConfigGenerator,
    0xF6A7B8C9, 0xD0E1, 0x4F2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F, 0x8A, 0x9B, 0x0C);

/**
  IConfigGenerator Interface

  Generates output files from configuration.
**/
typedef struct _IConfigGenerator_Vtbl {
    //
    // IUnknown methods
    //
    HRESULT (ANXAPI *QueryInterface)(
        IConfigGenerator *This,
        REFIID riid,
        VOID **ppvObject
    );

    UINTN (ANXAPI *AddRef)(
        IConfigGenerator *This
    );

    UINTN (ANXAPI *Release)(
        IConfigGenerator *This
    );

    //
    // IConfigGenerator methods
    //

    /**
      Generate CMake cache file.
    **/
    HRESULT (ANXAPI *GenerateCMakeCache)(
        IConfigGenerator *This,
        IConfigDatabase *Database,
        CONST CHAR8 *OutputPath
    );

    /**
      Generate C header file.
    **/
    HRESULT (ANXAPI *GenerateCHeader)(
        IConfigGenerator *This,
        IConfigDatabase *Database,
        CONST CHAR8 *OutputPath
    );

    /**
      Generate Makefile fragment.
    **/
    HRESULT (ANXAPI *GenerateMakefile)(
        IConfigGenerator *This,
        IConfigDatabase *Database,
        CONST CHAR8 *OutputPath
    );

    /**
      Generate autoconf fragment.
    **/
    HRESULT (ANXAPI *GenerateAutoconf)(
        IConfigGenerator *This,
        IConfigDatabase *Database,
        CONST CHAR8 *OutputPath
    );

} IConfigGenerator_Vtbl;

struct _IConfigGenerator {
    CONST IConfigGenerator_Vtbl *Vtbl;
};

//
// Factory functions
//

/**
  Create a configuration database.

  @param[out] Database  Pointer to receive the database interface.

  @retval S_OK        Database created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxConfigCreateDatabase(
    OUT IConfigDatabase **Database
);

/**
  Create a configuration generator.

  @param[out] Generator  Pointer to receive the generator interface.

  @retval S_OK        Generator created successfully.
  @retval E_OUTOFMEMORY  Memory allocation failed.
**/
HRESULT
ANXAPI
AnxConfigCreateGenerator(
    OUT IConfigGenerator **Generator
);

/**
  Run interactive configuration menu (menuconfig-like).

  @param[in] Database  Configuration database.
  @param[in] Title     Menu title.

  @retval S_OK        Menu completed successfully.
  @retval E_ABORT     User cancelled.
**/
HRESULT
ANXAPI
AnxConfigRunMenu(
    IN IConfigDatabase *Database,
    IN CONST CHAR8 *Title
);

#ifdef __cplusplus
}
#endif

#endif /* __ANANKE_ANXCONFIG_H__ */
